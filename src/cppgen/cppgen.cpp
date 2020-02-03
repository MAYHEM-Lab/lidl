#include "cppgen.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>


namespace lidl::cpp {
namespace {
bool is_anonymous(const module& mod, const type& t) {
    return !mod.symbols->definition_lookup(&t);
}

void generate_raw_struct_field(std::string_view name,
                               std::string_view type_name,
                               std::ostream& str) {
    str << fmt::format("{} {};\n", type_name, name);
}

std::string
generate_getter(std::string_view name, std::string_view type_name, bool is_const) {
    constexpr auto format = R"__({}& {}() {{ return m_raw.{}; }})__";
    constexpr auto const_format = R"__(const {}& {}() const {{ return m_raw.{}; }})__";
    return fmt::format(is_const ? const_format : format, type_name, name, name);
}

void generate_struct_field(std::string_view name,
                           std::string_view type_name,
                           std::ostream& str) {
    str << generate_getter(name, type_name, true) << '\n';
    str << generate_getter(name, type_name, false) << '\n';
}

void generate_constructor(bool has_ref,
                          const module& m,
                          std::string_view type_name,
                          const structure& s,
                          std::ostream& str) {
    std::vector<std::string> arg_names;
    if (has_ref) {
        arg_names.push_back("::lidl::message_builder& builder");
    }

    std::vector<std::string> initializer_list;

    for (auto& [member_name, member] : s.members) {
        auto member_type = get_type(member.type_);

        if (member_type->is_reference_type(m)) {
            // must be a pointer instantiation
            auto& base = std::get<name>(member.type_.args[0]);
            auto identifier = get_identifier(m, base);

            if (!member.is_nullable()) {
                arg_names.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
                initializer_list.push_back(
                    fmt::format("{}(p_{})", member_name, member_name));
                continue;
            }
            arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
            initializer_list.push_back(
                fmt::format("{}(p_{} ? decltype({}){{*p_{}}} : decltype({}){{nullptr}})",
                            member_name,
                            member_name,
                            member_name,
                            member_name,
                            member_name));
            continue;
        }

        auto identifier = get_identifier(m, member.type_);
        arg_names.push_back(fmt::format("const {}& p_{}", identifier, member_name));
        initializer_list.push_back(fmt::format("{}(p_{})", member_name, member_name));
    }

    str << fmt::format("{}({}) : {} {{}}",
                       type_name,
                       fmt::join(arg_names, ", "),
                       fmt::join(initializer_list, ", "));
}

void generate_specialization(const module& m,
                             std::string_view templ_name,
                             const structure& s,
                             std::ostream& str) {
    auto attr = s.attributes.get<procedure_params_attribute>("procedure_params");

    std::stringstream pub;

    for (auto& [name, member] : s.members) {
        generate_raw_struct_field(name, get_identifier(m, member.type_), pub);
    }

    std::stringstream ctor;
    generate_constructor(s.is_reference_type(m), m, templ_name, s, ctor);

    constexpr auto format = R"__(template <> struct {}<&{}::{}> {{
            {}
            {}
        }};)__";

    str << fmt::format(format,
                       templ_name,
                       attr->serv_name,
                       attr->proc_name,
                       ctor.str(),
                       pub.str())
        << '\n';
}

void generate_raw_struct(const module& m,
                         std::string_view name,
                         const structure& s,
                         std::ostream& str) {
    std::stringstream pub;

    for (auto& [name, member] : s.members) {
        generate_raw_struct_field(name, get_identifier(m, member.type_), pub);
    }

    std::stringstream ctor;
    generate_constructor(s.is_reference_type(m), m, name, s, ctor);

    constexpr auto format = R"__(struct {} {{
        {}
        {}
    }};)__";

    str << fmt::format(format, name, ctor.str(), pub.str()) << '\n';
}

void generate_static_asserts(const module& mod,
                             std::string_view name,
                             const type& t,
                             std::ostream& str) {
    constexpr auto size_format =
        R"__(static_assert(sizeof({}) == {}, "Size of {} does not match the wire size!");)__";
    constexpr auto align_format =
        R"__(static_assert(alignof({}) == {}, "Alignment of {} does not match the wire alignment!");)__";

    str << fmt::format(size_format, name, t.wire_layout(mod).size(), name) << '\n';
    str << fmt::format(align_format, name, t.wire_layout(mod).alignment(), name) << '\n';
}

void generate_procedure(const module& mod,
                        std::string_view proc_name,
                        const procedure& proc,
                        std::ostream& str) {
    constexpr auto decl_format = "virtual ::lidl::status<{}> {}({}) = 0;";

    std::vector<std::string> params;
    for (auto& [param_name, param] : proc.parameters) {
        if (get_type(param)->is_reference_type(mod)) {
            if (param.args.empty()) {
                throw std::runtime_error(
                    fmt::format("Not a pointer: {}", get_identifier(mod, param)));
            }
            auto identifier = get_identifier(mod, std::get<name>(param.args.at(0)));
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        } else {
            auto identifier = get_identifier(mod, param);
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        }
    }

    std::string ret_type_name;
    auto ret_type = get_type(proc.return_types.at(0));
    if (!ret_type->is_reference_type(mod)) {
        // can use a regular return value
        ret_type_name = get_identifier(mod, proc.return_types.at(0));
    } else {
        ret_type_name = fmt::format(
            "const {}&",
            get_identifier(mod, std::get<name>(proc.return_types.at(0).args.at(0))));
        params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
    }

    str << fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));
}

void generate_service_descriptor(const module& mod,
                                 std::string_view service_name,
                                 const service& service,
                                 std::ostream& str) {
    std::vector<std::string> names(service.procedures.size());
    std::vector<std::string> tuple_types(service.procedures.size());
    std::transform(service.procedures.begin(),
                   service.procedures.end(),
                   names.begin(),
                   [&](auto& proc) {
                       return fmt::format("{{\"{}\"}}", std::get<std::string>(proc));
                   });
    std::transform(service.procedures.begin(),
                   service.procedures.end(),
                   tuple_types.begin(),
                   [&](auto& proc) {
                       return fmt::format("::lidl::procedure_descriptor<&{}::{}>",
                                          service_name,
                                          std::get<std::string>(proc));
                   });
    str << fmt::format("template <> class service_descriptor<{}> {{\npublic:\n",
                       service_name);
    str << fmt::format("std::tuple<{}> procedures{{{}}};\n",
                       fmt::join(tuple_types, ", "),
                       fmt::join(names, ", "));
    str << fmt::format("std::string_view name = \"{}\";\n", service_name);
    str << "};";
}

void generate_service(const module& mod,
                      std::string_view name,
                      const service& service,
                      std::ostream& str) {
    str << fmt::format("class {} {{\npublic:\n", name);
    for (auto& [name, proc] : service.procedures) {
        generate_procedure(mod, name, proc, str);
        str << '\n';
    }
    str << fmt::format("virtual ~{}() = default;\n", name);
    str << "};";
    str << '\n';
}

void declare_struct(const module& mod,
                    std::string_view name,
                    const structure& s,
                    std::ostream& str) {
    str << fmt::format("struct {};", name);
}
} // namespace
} // namespace lidl::cpp
namespace lidl {
void generate(const module& mod, std::ostream& str) {
    using namespace cpp;
    str << "#pragma once\n\n#include <lidl/lidl.hpp>\n";
    if (!mod.services.empty()) {
        str << "#include<lidl/service.hpp>\n";
    }
    str << '\n';

    for (auto& s : mod.structs) {
        if (is_anonymous(mod, s)) {
            continue;
        }

        auto name = nameof(*mod.symbols->definition_lookup(&s));
        if (!s.attributes.get_untyped("procedure_params")) {
            generate_raw_struct(mod, name, s, str);
            str << '\n';
            generate_static_asserts(mod, name, s, str);
            str << '\n';
        }
    }

    /*for (auto& [name, s] : mod.generic_structs) {
        std::cerr << "Codegen for generic structs not supported!";
    }*/

    for (auto& [name, service] : mod.services) {
        generate_service(mod, name, service, str);
        str << '\n';
    }

    for (auto& [name, service] : mod.services) {
        str << "namespace lidl {\n";
        generate_service_descriptor(mod, name, service, str);
        str << '\n';
        str << "}\n";
    }

    for (auto& s : mod.structs) {
        if (!is_anonymous(mod, s)) {
            continue;
        }

        if (s.attributes.get_untyped("procedure_params")) {
            str << "namespace lidl {\n";
            generate_specialization(mod, "procedure_params_t", s, str);
            // str << '\n';
            // generate_static_asserts(mod, name, s, str);
            str << '\n';
            str << "}\n";
        }
    }
}
} // namespace lidl