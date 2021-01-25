//
// Created by fatih on 6/22/20.
//

#include "service_stub_gen.hpp"

#include "cppgen.hpp"

#include <lidl/basic.hpp>
#include <lidl/view_types.hpp>

namespace lidl::cpp {
namespace {
std::string decide_param_type_decoration(const module& mod, const parameter& param) {
    auto param_type = get_type(mod, param.type);
    if (param_type->is_reference_type(mod)) {
        return "const {}& {}";
    } else if (param_type->is_view(mod)) {
        return "{} {}";
    } else {
        if (param.flags == param_flags::in) {
            // Small types are passed by value.
            if (param_type->wire_layout(mod).size() <= 2) {
                return "{} {}";
            }
            return "const {}& {}";
        }

        // This is either an out or an in_out parameter, either way, it has to be an
        // l-value ref.
        return "{}& {}";
    }
}

std::pair<std::string, std::vector<section_key_t>> make_proc_signature(
    const module& mod, std::string_view proc_name, const procedure& proc) {
    std::vector<section_key_t> dependencies;

    std::vector<std::string> params;
    for (auto& [param_name, param] : proc.parameters) {
        auto deps = codegen::def_keys_from_name(mod, param.type);
        for (auto& key : deps) {
            dependencies.push_back(key);
        }

        auto format     = decide_param_type_decoration(mod, param);
        auto identifier = get_user_identifier(mod, param.type);
        params.emplace_back(fmt::format(format, identifier, param_name));
    }

    std::vector<section_key_t> return_deps;

    for (auto& ret : proc.return_types) {
        auto deps = codegen::def_keys_from_name(mod, ret);
        return_deps.insert(return_deps.end(), deps.begin(), deps.end());
    }

    for (auto& key : return_deps) {
        dependencies.push_back(key);
    }

    std::string ret_type_name;
    if (proc.return_types.empty()) {
        ret_type_name = "void";
    } else {
        auto ret_type = get_type(mod, proc.return_types.at(0));
        ret_type_name = get_user_identifier(mod, proc.return_types.at(0));
        if (ret_type->is_reference_type(mod)) {
            ret_type_name = fmt::format("const {}&", ret_type_name);
            params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
        } else if (ret_type->is_view(mod)) {
            params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
        }
    }

    constexpr auto decl_format = "{} {}({})";
    auto sig =
        fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));

    return {sig, dependencies};
}
} // namespace

using codegen::sections;
sections remote_stub_generator::generate() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency(def_key());
    sect.add_dependency(misc_key());
    sect.add_dependency({fmt::format("service_return_union<{0}>", absolute_name()),
                         section_type::definition});
    sect.add_dependency({fmt::format("service_call_union<{0}>", absolute_name()),
                         section_type::definition});
    sect.keys.emplace_back(symbol(), section_type::misc);
    sect.name_space = mod().name_space;

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_procedure_stub(proc.first, *proc.second);
        });

    constexpr auto stub_format =
        R"__(template <class ServBase> class remote_{0} final : public {0}, private ServBase {{
    public:
        template <class... BaseParams>
        explicit remote_{0}(BaseParams&&... params) : ServBase(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}

std::string remote_stub_generator::copy_proc_param(const procedure& proc,
                                                   std::string_view param_name,
                                                   const lidl::parameter& param) {
    auto param_type = get_type(mod(), param.type);
    if (param_type->is_reference_type(mod())) {
        throw std::runtime_error("Reference types are not supported in stubs yet!");
        //        return fmt::format(
        //            "lidl::create<{}>(mb, {})",
        //            get_identifier(mod(),
        //            std::get<lidl::name>(param_type_name.args[0])), param_name);
    } else if (param_type->is_view(mod())) {
        auto wire_type_name = param_type->get_wire_type(mod());
        if (wire_type_name.base ==
            recursive_full_name_lookup(*mod().symbols, "string").value()) {

            return fmt::format("lidl::create_string(mb, {})", param_name);
        }

        throw std::runtime_error(fmt::format("Unknown view type {} at {}",
                                             get_identifier(mod(), param.type),
                                             lidl::to_string(*proc.src_info)));
    } else {
        return fmt::format("{}", param_name);
    }
}

std::string remote_stub_generator::copy_and_return(const procedure& proc) {
    if (proc.return_types.empty()) {
        return "return;";
    }

    auto ret_type = get_type(mod(), proc.return_types[0]);
    auto ident    = get_user_identifier(mod(), proc.return_types[0]);
    if (ret_type->is_reference_type(mod())) {
        constexpr auto format =
            R"__(auto [__extent, __pos] = lidl::meta::detail::find_extent_and_position(res.ret0());
auto __res_ptr = response_builder.allocate(__extent.size(), 1);
memcpy(__res_ptr, __extent.data(), __extent.size());
return *(reinterpret_cast<const {0}*>(response_builder.get_buffer().data() + __pos));)__";
        return fmt::format(format, ident);
        // Would just memcpy'ing the extent of the result work?
        throw std::runtime_error("Reference types are not supported by stubgen yet!");
    } else if (ret_type->is_view(mod())) {
        auto wire_type_name = ret_type->get_wire_type(mod());
        if (wire_type_name.base !=
            recursive_full_name_lookup(*mod().symbols, "string").value()) {
            throw std::runtime_error("Stubgen only supports string_views");
        }

        // This is a view type. We need to copy the result from the client response and
        // return a view into our own buffer.
        return R"__(auto& copied = lidl::create_string(response_builder, res.ret0().string_view());
return copied;)__";

    } else {
        // Must be a regular type, just return
        return "return res.ret0();";
    }
}

std::string remote_stub_generator::make_procedure_stub(std::string_view proc_name,
                                                       const procedure& proc) try {
    std::string ret_type_name;
    if (proc.return_types.empty()) {
        ret_type_name = "void";
    } else {
        auto ret_type = get_type(mod(), proc.return_types.at(0));
        ret_type_name = get_user_identifier(mod(), proc.return_types.at(0));
        if (ret_type->is_reference_type(mod())) {
            ret_type_name = fmt::format("const {}&", ret_type_name);
        }
    }

    std::vector<std::string> param_names(proc.parameters.size());
    std::transform(proc.parameters.begin(),
                   proc.parameters.end(),
                   param_names.begin(),
                   [this, &proc](auto& param) {
                       return copy_proc_param(proc, param.first, param.second);
                   });
    if (proc.params_struct->is_reference_type(mod())) {
        param_names.emplace(param_names.begin(), "mb");
    }

    auto params_struct_identifier = get_identifier(
        mod(),
        lidl::name{
            recursive_definition_lookup(*mod().symbols, proc.params_struct).value()});

    if (proc.params_struct->is_reference_type(mod())) {
        params_struct_identifier =
            fmt::format("lidl::create<{}>", params_struct_identifier);
    }

    constexpr auto def_format = R"__({0} override {{
        using lidl::as_span;
        auto req_buf = ServBase::get_buffer();
        lidl::message_builder mb{{as_span(req_buf)}};
        lidl::create<lidl::service_call_union<{4}>>(mb, {2}({3}));
        auto buf = mb.get_buffer();
        auto resp = ServBase::send_receive(buf);
        auto& res =
            lidl::get_root<lidl::service_return_union<{4}>>(tos::span<const uint8_t>(as_span(resp))).{1}();
        {5}
    }})__";

    auto [sig, deps] = make_proc_signature(mod(), proc_name, proc);

    return fmt::format(def_format,
                       sig,
                       proc_name,
                       params_struct_identifier,
                       fmt::join(param_names, ", "),
                       name(),
                       copy_and_return(proc));
} catch (std::exception& ex) {
    std::cerr << fmt::format("Stub generator for {} failed while generating code for {}, "
                             "bailing out. Error: {}\n",
                             name(),
                             proc_name,
                             ex.what());
    return {};
}


std::vector<section_key_t> generate_procedure(const module& mod,
                                              std::string_view proc_name,
                                              const procedure& proc,
                                              std::ostream& str) {
    auto [sig, keys] = make_proc_signature(mod, proc_name, proc);
    str << "virtual " << sig << " = 0;";
    return keys;
}

sections
generate_service_descriptor(const module& mod, std::string_view, const service& service) {
    auto serv_handle    = *recursive_definition_lookup(*mod.symbols, &service);
    auto serv_full_name = get_identifier(mod, {serv_handle});

    std::ostringstream str;
    section sect;
    sect.name_space = "lidl";
    sect.keys.emplace_back(serv_handle, section_type::misc);
    sect.depends_on.emplace_back(serv_handle, section_type::definition);

    auto params_union =
        recursive_definition_lookup(*mod.symbols, service.procedure_params_union).value();

    auto results_union =
        recursive_definition_lookup(*mod.symbols, service.procedure_results_union)
            .value();

    sect.add_dependency({params_union, section_type::definition});
    sect.add_dependency({results_union, section_type::definition});

    auto all_procs = service.all_procedures();
    std::vector<std::string> tuple_types(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), tuple_types.begin(), [&](auto& proc_pair) {
            auto& [proc_name, proc_ptr] = proc_pair;
            auto param_name             = get_identifier(
                mod, name{*mod.symbols->definition_lookup(proc_ptr->params_struct)});
            auto res_name = get_identifier(
                mod, name{*mod.symbols->definition_lookup(proc_ptr->results_struct)});
            return fmt::format(
                "::lidl::procedure_descriptor<&{0}::{1}, {2}, {3}>{{\"{1}\"}}",
                serv_full_name,
                proc_name,
                param_name,
                res_name);
        });
    str << fmt::format("template <> class service_descriptor<{}> {{\npublic:\n",
                       serv_full_name);
    str << fmt::format("static constexpr inline auto procedures = std::make_tuple({});\n",
                       fmt::join(tuple_types, ", "));
    str << fmt::format("static constexpr inline std::string_view name = \"{}\";\n",
                       serv_full_name);
    str << fmt::format("using params_union = {};\n",
                       get_identifier(mod, name{params_union}));
    str << fmt::format("using results_union = {};\n",
                       get_identifier(mod, name{results_union}));
    str << "};";

    sect.definition = str.str();
    return sections{{sect}};
}

sections service_generator::generate() {
    std::vector<std::string> inheritance;
    if (auto& base = get().extends; base) {
        inheritance.emplace_back(get_identifier(mod(), {*base}));
    } else {
        inheritance.emplace_back(fmt::format("::lidl::service_base", name()));
    }

    std::stringstream str;

    section def_sec;

    str << fmt::format(
        "class {}{} {{\npublic:\n",
        name(),
        inheritance.empty()
            ? ""
            : fmt::format(" : public {}", fmt::join(inheritance, ", public ")));
    for (auto& [name, proc] : get().procedures) {
        auto deps = generate_procedure(mod(), name, proc, str);
        def_sec.depends_on.insert(def_sec.depends_on.end(), deps.begin(), deps.end());
        str << '\n';
    }
    str << fmt::format("    virtual ~{}() = default;\n", name());
    str << "};";

    auto serv_handle    = *recursive_definition_lookup(*mod().symbols, &get());
    auto serv_full_name = get_identifier(mod(), {serv_handle});

    def_sec.add_key({serv_handle, section_type::definition});
    def_sec.definition = str.str();
    def_sec.name_space = mod().name_space;

    auto res = sections{{std::move(def_sec)}};

    res.merge_before(generate_service_descriptor(mod(), name(), get()));
    return res;
}

codegen::sections svc_stub_generator::generate() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency(def_key());
    sect.add_dependency(misc_key());
    sect.keys.emplace_back(symbol(), section_type::misc);
    sect.name_space = mod().name_space;

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_procedure_stub(proc.first, *proc.second);
        });

    constexpr auto stub_format =
        R"__(template <class NextLayer> class zerocopy_{0} final : public {0}, private NextLayer {{
    public:
        template <class... BaseParams>
        explicit zerocopy_{0}(BaseParams&&... params) : NextLayer(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}

std::string svc_stub_generator::make_procedure_stub(std::string_view proc_name,
                                                    const procedure& proc) {
    std::string ret_type_name;
    if (proc.return_types.empty()) {
        ret_type_name = "void";
    } else {
        auto ret_type = get_type(mod(), proc.return_types.at(0));
        ret_type_name = get_user_identifier(mod(), proc.return_types.at(0));
        if (ret_type->is_reference_type(mod())) {
            // Notice the return type is a pointer here!
            ret_type_name = fmt::format("{}*", ret_type_name);
        }
    }

    std::vector<std::string> param_names(proc.parameters.size());
    std::transform(
        proc.parameters.begin(),
        proc.parameters.end(),
        param_names.begin(),
        [this, &proc](auto& param) { return fmt::format("&{}", param.first); });
    if (proc.results_struct->is_reference_type(mod())) {
        param_names.emplace_back("&response_builder");
    }

    auto tuple_make = fmt::format("auto params_tuple_ = std::make_tuple({0});",
                                  fmt::join(param_names, ", "));

    constexpr auto void_def_format = R"__({0} override {{
        {1}
        auto result_ = NextLayer::execute({2}, &params_tuple_, nullptr);
    }})__";

    // Since we can't use a natural return path, we instead allocate the memory to hold
    // the return object. If the return object is a reference, it's being converted to a
    // pointer above, since references aren't objects.
    // Once we have this, the parameters tuple and the address of the return memory is
    // passed to the zero-copy handler.
    // Once it returns, it will have placed the return value in the buffer provided.
    // We cast it to a pointer to the return type and dereference it to return the right
    // value.
    // If the return type is actually a reference, we need one more dereference to get a
    // reference from our pointer, so the last {4} is * if the return type is a ref type
    // and empty otherwise.
    constexpr auto def_format = R"__({0} override {{
        {1}
        using ret_t = {3};
        std::aligned_storage_t<sizeof(ret_t), alignof(ret_t)> return_;
        auto result_ = NextLayer::execute({2}, &params_tuple_, static_cast<void*>(&return_));
        return {4}*reinterpret_cast<ret_t*>(&return_);
    }})__";

    auto [sig, deps] = make_proc_signature(mod(), proc_name, proc);

    return fmt::format(proc.return_types.empty() ? void_def_format : def_format,
                       sig,
                       tuple_make,
                       get().proc_index(proc).value(),
                       ret_type_name,
                       proc.results_struct->is_reference_type(mod()) ? "*" : "");
}
} // namespace lidl::cpp