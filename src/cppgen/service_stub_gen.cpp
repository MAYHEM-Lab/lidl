//
// Created by fatih on 6/22/20.
//

#include "service_stub_gen.hpp"

#include "cppgen.hpp"

#include <lidl/basic.hpp>
#include <lidl/view_types.hpp>

namespace lidl::cpp {
sections remote_stub_generator::generate() {
    section sect;

    // We depend on the definition for the service.
    sect.add_dependency(def_key());
    sect.keys.emplace_back(symbol(), section_type::misc);
    sect.name_space = mod().name_space;

    std::vector<std::string> proc_stubs(get().procedures.size());
    std::transform(
        get().procedures.begin(),
        get().procedures.end(),
        proc_stubs.begin(),
        [this](auto& proc) { return make_procedure_stub(proc.first, proc.second); });

    constexpr auto stub_format =
        R"__(template <class ServBase> class remote_{0} : public {0}, private ServBase {{
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
                                                   const lidl::name& param_type_name) {
    auto param_type = get_type(mod(), param_type_name);
    if (param_type->is_reference_type(mod())) {
        throw std::runtime_error("Reference types are not supported in stubs yet!");
        //        return fmt::format(
        //            "lidl::create<{}>(mb, {})",
        //            get_identifier(mod(),
        //            std::get<lidl::name>(param_type_name.args[0])), param_name);
    } else if (auto view_t = dynamic_cast<const view_type*>(param_type)) {
        auto wire_type_name = view_t->get_wire_type();
        if (wire_type_name.base ==
            recursive_full_name_lookup(*mod().symbols, "string").value()) {

            return fmt::format("lidl::create_string(mb, {})", param_name);
        }

        throw std::runtime_error(fmt::format("Unknown view type {} at {}",
                                             get_identifier(mod(), param_type_name),
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
    if (ret_type->is_reference_type(mod())) {
        throw std::runtime_error("Reference types are not supported by stubgen yet!");
    } else if (auto vt = dynamic_cast<const view_type*>(ret_type)) {
        auto wire_type_name = vt->get_wire_type();
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
    constexpr auto def_format = R"__({0} {1}({2}) override {{
        using lidl::data;
        auto req_buf = ServBase::get_buffer();
        lidl::message_builder mb(data(req_buf));
        lidl::create<lidl::service_call_union<{5}>>(mb, {3}({4}));
        auto buf = mb.get_buffer();
        auto resp = ServBase::send_receive(buf);
        auto& res =
            lidl::get_root<lidl::service_return_union<{5}>>(data(resp)).{1}();
        {6}
    }})__";

    std::vector<std::string> params;
    for (auto& [param_name, param] : proc.parameters) {
        auto param_type = get_type(mod(), param);
        if (param_type->is_reference_type(mod())) {
            if (param.args.empty()) {
                throw std::runtime_error(
                    fmt::format("Not a pointer: {}", get_identifier(mod(), param)));
            }

            auto identifier =
                get_identifier(mod(), std::get<lidl::name>(param.args.at(0)));
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        } else if (auto vt = dynamic_cast<const view_type*>(param_type); vt) {
            auto identifier = get_identifier(mod(), param);
            params.emplace_back(fmt::format("{} {}", identifier, param_name));
        } else {
            auto identifier = get_identifier(mod(), param);
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        }
    }

    std::string ret_type_name;
    if (proc.return_types.empty()) {
        ret_type_name = "void";
    } else {
        auto ret_type = get_type(mod(), proc.return_types.at(0));
        ret_type_name = get_user_identifier(mod(), proc.return_types.at(0));
        if (ret_type->is_reference_type(mod())) {
            ret_type_name = fmt::format("const {}&", ret_type_name);
            params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
        } else if (auto vt = dynamic_cast<const view_type*>(ret_type); vt) {
            params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
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

    return fmt::format(def_format,
                       ret_type_name,
                       proc_name,
                       fmt::join(params, ", "),
                       params_struct_identifier,
                       fmt::join(param_names, ", "),
                       name(),
                       copy_and_return(proc));
} catch (std::exception& ex) {
    std::cerr << fmt::format(
        "Stub generator for {} failed while generating code for {}, bailing out.",
        name(),
        proc_name);
    return {};
}
} // namespace lidl::cpp