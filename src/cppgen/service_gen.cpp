#include "cppgen.hpp"
#include "service_stub_gen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <codegen.hpp>
#include <lidl/errors.hpp>
#include <sections.hpp>

namespace lidl::cpp {
using codegen::section_key_t;
namespace {
bool return_of_name_requires_message_builder(const module& mod, const name& nm) {
    return is_view(nm) || (is_type(nm) && get_wire_type(mod, nm)->is_reference_type(mod));
}

bool procedure_needs_message_builder(const module& mod, const procedure& proc) {
    return return_of_name_requires_message_builder(mod, proc.return_types.front());
}

std::string compute_return_type_name(const module& mod, const procedure& proc) {
    if (proc.return_types.empty()) {
        return "void";
    }

    std::string ret_type_name;
    ret_type_name = get_user_identifier(mod, proc.return_types.front());

    if (is_type(proc.return_types.front())) {
        auto ret_type = get_wire_type(mod, proc.return_types.front());
        if (ret_type->is_reference_type(mod)) {
            ret_type_name = fmt::format("const {}&", ret_type_name);
        }
    }

    return ret_type_name;
}

std::string decide_param_type_decoration(const module& mod, const parameter& param) {
    if (is_view(param.type)) {
        return "{} {}";
    } else if (is_type(param.type)) {
        auto param_type = get_wire_type(mod, param.type);

        if (param_type->is_reference_type(mod)) {
            return "const {}& {}";
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
    } else if (is_service(param.type)) {
        return "lidl::service_ptr<{}>";
    }

    assert(false);
    std::terminate();
}

std::pair<std::string, std::vector<section_key_t>>
make_proc_signature(const module& mod,
                    std::string_view proc_name,
                    const procedure& proc,
                    bool async = false) {
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

    auto ret_type_name = compute_return_type_name(mod, proc);

    if (procedure_needs_message_builder(mod, proc)) {
        params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
    }

    if (async) {
        ret_type_name = fmt::format("tos::Task<{}>", ret_type_name);
    }

    constexpr auto decl_format = "{} {}({})";
    auto sig =
        fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));

    return {sig, dependencies};
}

std::vector<section_key_t> generate_procedure(const module& mod,
                                              std::string_view proc_name,
                                              const procedure& proc,
                                              std::ostream& str,
                                              bool async = false) {
    auto [sig, keys] = make_proc_signature(mod, proc_name, proc, async);
    str << "virtual " << sig << " = 0;";
    return keys;
}


codegen::sections
generate_service_descriptor(const module& mod, std::string_view, const service& service) {
    auto serv_handle    = *recursive_definition_lookup(mod.symbols(), &service);
    auto serv_full_name = get_identifier(mod, lidl::name{serv_handle});

    std::ostringstream str;
    section sect;
    sect.add_key({serv_handle, section_type::service_descriptor});
    sect.add_dependency({serv_handle, section_type::definition});

    auto params_union = recursive_definition_lookup(service.get_scope(),
                                                    &service.procedure_params_union(mod))
                            .value();

    auto results_union = recursive_definition_lookup(
                             service.get_scope(), &service.procedure_results_union(mod))
                             .value();

    sect.add_dependency({params_union, section_type::definition});
    sect.add_dependency({results_union, section_type::definition});

    auto all_procs = service.all_procedures();
    std::vector<std::string> tuple_types(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), tuple_types.begin(), [&](auto& proc_pair) {
            auto& [proc_name, proc_ptr] = proc_pair;
            auto param_name =
                get_identifier(mod,
                               name{recursive_definition_lookup(
                                        mod.get_scope(), &proc_ptr->params_struct(mod))
                                        .value()});

            auto res_name =
                get_identifier(mod,
                               name{recursive_definition_lookup(
                                        mod.get_scope(), &proc_ptr->results_struct(mod))
                                        .value()});
            return fmt::format(
                "::lidl::procedure_descriptor<&service_type::sync_server::{1}, "
                "&service_type::async_server::{1}, {2}, {3}>{{\"{1}\"}}",
                serv_full_name,
                proc_name,
                param_name,
                res_name);
        });
    str << fmt::format("template <> class service_descriptor<{}> {{\npublic:\n",
                       serv_full_name);
    str << fmt::format("using service_type = {};\n", serv_full_name);
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
    return codegen::sections{{sect}};
}
} // namespace

using codegen::do_generate;
codegen::sections better_service_generator::generate() {
    static constexpr auto service_format = R"__(struct {} {{
    struct wire_types;

    struct sync_server;
    struct async_server;

    template <class>
    struct zerocopy_client;
    template <class>
    struct async_zerocopy_client;

    template <class>
    struct stub_client;
    template <class>
    struct async_stub_client;
}};)__";

    codegen::section def_sec;
    def_sec.add_key(def_key());
    def_sec.definition = fmt::format(service_format, name());

    auto res = codegen::sections{{def_sec}};
    res.merge_before(generate_wire_types());
    res.merge_before(generate_sync_server());
    res.merge_before(generate_async_server());
    res.merge_before(generate_traits());
    res.merge_before(generate_zerocopy_stub());
    res.merge_before(generate_async_zerocopy_stub());
    res.merge_before(generate_stub());
    res.merge_before(generate_async_stub());
    return res;
}

codegen::sections better_service_generator::generate_wire_types() {
    codegen::sections wire_res;

    section_key_t wire_types_key{symbol(), section_type::wire_types};

    std::vector<std::string> wire_type_names;
    for (auto& [name, proc] : get().own_procedures()) {
        {
            auto sym = *get().get_scope().definition_lookup(&proc->params_struct(mod()));
            auto union_name = local_name(sym);
            auto name       = fmt::format("{}::wire_types::{}", this->name(), union_name);
            auto abs_name =
                fmt::format("{}::wire_types::{}", absolute_name(), union_name);
            rename(mod(), sym, fmt::format("wire_types::{}", union_name));

            wire_type_names.emplace_back(union_name);

            auto generator =
                struct_gen(mod(), name, union_name, abs_name, proc->params_struct(mod()));
            auto subres = generator.generate();
            for (auto& x : subres.get_sections()) {
                x.add_dependency(wire_types_key);
            }
            wire_res.merge_before(std::move(subres));
        }

        {
            auto sym = *get().get_scope().definition_lookup(&proc->results_struct(mod()));
            auto union_name = local_name(sym);
            auto name       = fmt::format("{}::wire_types::{}", this->name(), union_name);
            auto abs_name =
                fmt::format("{}::wire_types::{}", absolute_name(), union_name);
            rename(mod(), sym, fmt::format("wire_types::{}", union_name));

            wire_type_names.emplace_back(union_name);

            auto generator = struct_gen(
                mod(), name, union_name, abs_name, proc->results_struct(mod()));
            auto subres = generator.generate();
            for (auto& x : subres.get_sections()) {
                x.add_dependency(wire_types_key);
            }
            wire_res.merge_before(std::move(subres));
        }
    }

    {
        auto sym =
            *get().get_scope().definition_lookup(&get().procedure_params_union(mod()));
        auto union_name = local_name(sym);
        auto name       = fmt::format("{}::wire_types::{}", this->name(), union_name);
        auto abs_name   = fmt::format("{}::wire_types::{}", absolute_name(), union_name);
        wire_type_names.emplace_back(union_name);

        auto generator = union_gen(
            mod(), name, union_name, abs_name, get().procedure_params_union(mod()));
        auto subres = generator.generate();
        for (auto& x : subres.get_sections()) {
            x.add_dependency(wire_types_key);
        }
        wire_res.merge_before(std::move(subres));
    }

    {
        auto sym =
            *get().get_scope().definition_lookup(&get().procedure_results_union(mod()));
        auto union_name = local_name(sym);
        auto name       = fmt::format("{}::wire_types::{}", this->name(), union_name);
        auto abs_name   = fmt::format("{}::wire_types::{}", absolute_name(), union_name);
        wire_type_names.emplace_back(union_name);

        auto generator = union_gen(
            mod(), name, union_name, abs_name, get().procedure_results_union(mod()));
        auto subres = generator.generate();
        for (auto& x : subres.get_sections()) {
            x.add_dependency(wire_types_key);
        }
        wire_res.merge_before(std::move(subres));
    }

    std::vector<std::string> sub_structs(wire_type_names);
    std::transform(wire_type_names.begin(),
                   wire_type_names.end(),
                   sub_structs.begin(),
                   [](auto& name) { return fmt::format("class {};", name); });
    auto wire_fwd_decls = fmt::format("{}", fmt::join(sub_structs, "\n"));

    static constexpr auto wire_fmt = R"__(struct {}::wire_types {{
    {}
}};)__";

    section wire_types_sec;
    wire_types_sec.add_key(wire_types_key);
    wire_types_sec.add_dependency(def_key());
    wire_types_sec.definition = fmt::format(wire_fmt, name(), wire_fwd_decls);

    wire_res.add(wire_types_sec);
    return wire_res;
}

codegen::sections better_service_generator::generate_sync_server() {
    std::vector<std::string> inheritance;
    if (auto& base = get().extends; base) {
        inheritance.emplace_back(
            fmt::format("{}::sync_server", get_identifier(mod(), {*base})));
    } else {
        inheritance.emplace_back(fmt::format("::lidl::service_base", name()));
    }

    std::stringstream str;

    section def_sec;

    str << fmt::format(
        "class {}::sync_server{} {{\npublic:\n",
        name(),
        inheritance.empty()
            ? ""
            : fmt::format(" : public {}", fmt::join(inheritance, ", public ")));

    for (auto& [name, proc] : get().own_procedures()) {
        auto deps = generate_procedure(mod(), name, *proc, str);
        str << '\n';
        def_sec.depends_on.insert(def_sec.depends_on.end(), deps.begin(), deps.end());
    }

    str << fmt::format("virtual ~sync_server() = default;\n", name());

    str << fmt::format("using service_type = {};\n", absolute_name());

    str << "};";

    def_sec.add_dependency(def_key());
    def_sec.add_key({symbol(), section_type::sync_server});
    def_sec.definition = str.str();

    return codegen::sections{{std::move(def_sec)}};
}

codegen::sections better_service_generator::generate_async_server() {
    std::vector<std::string> inheritance;
    if (auto& base = get().extends; base) {
        inheritance.emplace_back(
            fmt::format("{}::async_server", get_identifier(mod(), {*base})));
    } else {
        inheritance.emplace_back(fmt::format("::lidl::service_base", name()));
    }

    std::stringstream str;

    section def_sec;

    str << fmt::format(
        "class {}::async_server{} {{\npublic:\n",
        name(),
        inheritance.empty()
            ? ""
            : fmt::format(" : public {}", fmt::join(inheritance, ", public ")));

    for (auto& [name, proc] : get().own_procedures()) {
        auto deps = generate_procedure(mod(), name, *proc, str, true);
        str << '\n';
        def_sec.depends_on.insert(def_sec.depends_on.end(), deps.begin(), deps.end());
    }

    str << fmt::format("using service_type = {};\n", absolute_name());
    str << fmt::format("virtual ~async_server() = default;\n", name());

    str << "};";

    def_sec.add_dependency(def_key());
    def_sec.add_key({symbol(), section_type::async_server});
    def_sec.definition = str.str();

    return codegen::sections{{std::move(def_sec)}};
}

codegen::sections better_service_generator::generate_traits() {
    return generate_service_descriptor(mod(), name(), get());
}

codegen::sections better_service_generator::generate_zerocopy_stub() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency({symbol(), section_type::sync_server});
    sect.add_key({symbol(), section_type::zerocopy_stub});

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_zerocopy_procedure_stub(proc.first, *proc.second, false);
        });

    constexpr auto stub_format =
        R"__(template <class NextLayer> class {0}::zerocopy_client final : public {0}::sync_server, private NextLayer {{
    public:
        template <class... BaseParams>
        explicit zerocopy_client(BaseParams&&... params) : NextLayer(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}

codegen::sections better_service_generator::generate_async_zerocopy_stub() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency({symbol(), section_type::async_server});
    sect.add_key({symbol(), section_type::async_zerocopy_stub});

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_zerocopy_procedure_stub(proc.first, *proc.second, true);
        });

    constexpr auto stub_format =
        R"__(template <class NextLayer> class {0}::async_zerocopy_client final : public {0}::async_server, private NextLayer {{
    public:
        template <class... BaseParams>
        explicit async_zerocopy_client(BaseParams&&... params) : NextLayer(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}

std::string better_service_generator::make_zerocopy_procedure_stub(
    std::string_view proc_name, const procedure& proc, bool async) {
    std::string tmp_ret_type_name;
    if (proc.return_types.empty()) {
        tmp_ret_type_name = "void";
    } else {
        tmp_ret_type_name = get_user_identifier(mod(), proc.return_types.at(0));
        if (is_type(proc.return_types.front())) {
            auto ret_type = get_wire_type(mod(), proc.return_types.at(0));
            if (ret_type->is_reference_type(mod())) {
                // Notice the return type is a pointer here!
                // This is not for the actual return object, but rather for the temporary
                // location that will hold the vale we'll return.
                tmp_ret_type_name = fmt::format("{}*", tmp_ret_type_name);
            }
        }
    }

    std::vector<std::string> param_names(proc.parameters.size());
    std::transform(proc.parameters.begin(),
                   proc.parameters.end(),
                   param_names.begin(),
                   [](auto& param) { return fmt::format("&{}", param.first); });

    if (procedure_needs_message_builder(mod(), proc)) {
        param_names.emplace_back("&response_builder");
    }

    auto tuple_make = fmt::format("auto params_tuple_ = std::make_tuple({0});",
                                  fmt::join(param_names, ", "));

    constexpr auto void_def_format = R"__({0} override {{
        {1}
        auto result_ = NextLayer::execute({2}, &params_tuple_, nullptr);
    }})__";

    constexpr auto async_void_def_format = R"__({0} override {{
        {1}
        auto result_ = co_await NextLayer::execute({2}, &params_tuple_, nullptr);
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
        [[maybe_unused]] auto result_ = NextLayer::execute({2}, &params_tuple_, static_cast<void*>(&return_));
        return {4}*reinterpret_cast<ret_t*>(&return_);
    }})__";

    constexpr auto async_def_format = R"__({0} override {{
        {1}
        using ret_t = {3};
        std::aligned_storage_t<sizeof(ret_t), alignof(ret_t)> return_;
        [[maybe_unused]] auto result_ = co_await NextLayer::execute({2}, &params_tuple_, static_cast<void*>(&return_));
        co_return {4}*reinterpret_cast<ret_t*>(&return_);
    }})__";

    auto [sig, deps] = make_proc_signature(mod(), proc_name, proc, async);

    return fmt::format(
        proc.return_types.empty() ? async ? async_void_def_format : void_def_format
        : async                   ? async_def_format
                                  : def_format,
        sig,
        tuple_make,
        get().proc_index(proc).value(),
        tmp_ret_type_name,
        is_type(proc.return_types.front()) &&
                get_wire_type(mod(), proc.return_types.front())->is_reference_type(mod())
            ? "*"
            : "");
}

codegen::sections better_service_generator::generate_stub() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency({symbol(), section_type::sync_server});
    sect.add_dependency({symbol(), section_type::wire_types});
    sect.add_dependency({symbol(), section_type::service_params_union});
    sect.add_dependency({symbol(), section_type::service_return_union});

    sect.add_key({symbol(), section_type::stub});

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_procedure_stub(proc.first, *proc.second, false);
        });

    constexpr auto stub_format =
        R"__(template <class ServBase> class {0}::stub_client final : public {0}::sync_server, private ServBase {{
    public:
        template <class... BaseParams>
        explicit stub_client(BaseParams&&... params) : ServBase(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}
codegen::sections better_service_generator::generate_async_stub() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency({symbol(), section_type::async_server});
    sect.add_dependency({symbol(), section_type::wire_types});
    sect.add_dependency({symbol(), section_type::service_params_union});
    sect.add_dependency({symbol(), section_type::service_return_union});

    sect.add_key({symbol(), section_type::async_stub});

    auto all_procs = get().all_procedures();
    std::vector<std::string> proc_stubs(all_procs.size());
    std::transform(
        all_procs.begin(), all_procs.end(), proc_stubs.begin(), [this](auto& proc) {
            return make_procedure_stub(proc.first, *proc.second, true);
        });

    constexpr auto stub_format =
        R"__(template <class ServBase> class {0}::async_stub_client final : public {0}::async_server, private ServBase {{
    public:
        template <class... BaseParams>
        explicit async_stub_client(BaseParams&&... params) : ServBase(std::forward<BaseParams>(params)...) {{}}
        {1}
    }};)__";

    sect.definition = fmt::format(stub_format, name(), fmt::join(proc_stubs, "\n"));

    return {{std::move(sect)}};
}

std::string better_service_generator::make_procedure_stub(std::string_view proc_name,
                                                          const procedure& proc,
                                                          bool async) try {
    auto ret_type_name = compute_return_type_name(mod(), proc);

    std::vector<std::string> param_names(proc.parameters.size());
    std::transform(proc.parameters.begin(),
                   proc.parameters.end(),
                   param_names.begin(),
                   [this, &proc](auto& param) {
                       return copy_proc_param(proc, param.first, param.second);
                   });

    if (proc.params_struct(mod()).is_reference_type(mod())) {
        param_names.emplace(param_names.begin(), "mb");
    }

    auto params_struct_identifier =
        get_identifier(mod(),
                       lidl::name{recursive_definition_lookup(mod().symbols(),
                                                              &proc.params_struct(mod()))
                                      .value()});

    if (proc.params_struct(mod()).is_reference_type(mod())) {
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

    constexpr auto async_def_format = R"__({0} override {{
        using lidl::as_span;
        auto req_buf = ServBase::get_buffer();
        lidl::message_builder mb{{as_span(req_buf)}};
        lidl::create<lidl::service_call_union<{4}>>(mb, {2}({3}));
        auto buf = mb.get_buffer();
        auto resp = co_await ServBase::send_receive(buf);
        auto& res =
            lidl::get_root<lidl::service_return_union<{4}>>(tos::span<const uint8_t>(as_span(resp))).{1}();
        {5}
    }})__";

    auto [sig, deps] = make_proc_signature(mod(), proc_name, proc, async);

    return fmt::format(async ? async_def_format : def_format,
                       sig,
                       proc_name,
                       params_struct_identifier,
                       fmt::join(param_names, ", "),
                       name(),
                       copy_and_return(proc, async),
                       ret_type_name);
} catch (std::exception& ex) {
    std::cerr << fmt::format("Stub generator for {} failed while generating code for {}, "
                             "bailing out. Error: {}\n",
                             name(),
                             proc_name,
                             ex.what());
    return {};
}

std::string better_service_generator::copy_proc_param(const procedure& proc,
                                                      std::string_view param_name,
                                                      const parameter& param) {
    if (is_type(param.type)) {
        auto param_type = get_wire_type(mod(), param.type);
        if (param_type->is_reference_type(mod())) {
            throw std::runtime_error("Reference types are not supported in stubs yet!");
        }
        return fmt::format("{}", param_name);
    } else if (is_view(param.type)) {
        if (param.type.base ==
            recursive_full_name_lookup(mod().symbols(), "string_view").value()) {
            return fmt::format("lidl::create_string(mb, {})", param_name);
        }

        throw unknown_type_error(get_identifier(mod(), param.type), proc.src_info);
    }

    assert(false);
    std::terminate();
}
std::string better_service_generator::copy_and_return(const procedure& proc, bool async) {
    if (proc.return_types.empty()) {
        return async ? "co_return;" : "return;";
    }

    auto ident = get_user_identifier(mod(), proc.return_types.front());

    if (is_type(proc.return_types.front())) {
        auto ret_type = get_wire_type(mod(), proc.return_types.front());

        if (ret_type->is_reference_type(mod())) {
            constexpr auto format =
                R"__(auto [__extent, __pos] = lidl::meta::detail::find_extent_and_position(res.ret0());
auto __res_ptr = response_builder.allocate(__extent.size(), 1);
memcpy(__res_ptr, __extent.data(), __extent.size());
{1} *(reinterpret_cast<const {0}*>(response_builder.get_buffer().data() + __pos));)__";

            return fmt::format(format, ident, async ? "co_return" : "return");
        }

        // Must be a value type, just return
        return fmt::format("{} res.ret0();", async ? "co_return" : "return");
    } else if (is_service(proc.return_types.front())) {
        return fmt::format("{} ServBase::unpack_service(res.ret0());",
                           async ? "co_return" : "return");
    } else if (is_view(proc.return_types.front())) {
        if (proc.return_types.front().base !=
            recursive_full_name_lookup(mod().symbols(), "string_view").value()) {
            throw std::runtime_error("Stubgen only supports string_views");
        }

        // This is a view type. We need to copy the result from the client response
        // and return a view into our own buffer.
        return fmt::format(
            R"__(auto& copied = lidl::create_string(response_builder, res.ret0().string_view());
    {} static_cast<{}>(copied);)__",
            async ? "co_return" : "return",
            ident);
    }

    assert(false && "Don't know what to do");
    std::terminate();
}
} // namespace lidl::cpp