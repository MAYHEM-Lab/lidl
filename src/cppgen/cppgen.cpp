#include "cppgen.hpp"

#include "lidl/enumeration.hpp"
#include "lidl/union.hpp"

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

class raw_struct_gen {
public:
    raw_struct_gen(const module& mod, std::string_view name, const structure& str)
        : m_module{&mod}
        , m_name{name}
        , m_struct{&str} {
    }
    void generate(std::ostream& ostr) {
        for (auto& [name, member] : str().members) {
            generate_raw_struct_field(
                name, get_identifier(mod(), member.type_), m_sections.pub);
        }

        generate_raw_constructor();

        constexpr auto format = R"__(struct {} {{
            {}
            {}
        }};)__";

        ostr << fmt::format(format, m_name, m_sections.ctor.str(), m_sections.pub.str())
             << '\n';
    }

private:
    void generate_raw_constructor() {
        std::vector<std::string> arg_names;

        std::vector<std::string> initializer_list;

        for (auto& [member_name, member] : str().members) {
            auto member_type = get_type(member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);

            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
                initializer_list.push_back(fmt::format("{0}(p_{0})", member_name));
                continue;
            }
            arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
            initializer_list.push_back(fmt::format(
                "{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                member_name));
        }

        m_sections.ctor << fmt::format("{}({}) : {} {{}}",
                                       m_name,
                                       fmt::join(arg_names, ", "),
                                       fmt::join(initializer_list, ", "));
    }

    const module& mod() {
        return *m_module;
    }

    const structure& str() {
        return *m_struct;
    }

    struct {
        std::stringstream pub, ctor;
    } m_sections;

    const module* m_module;
    std::string_view m_name;
    const structure* m_struct;
};

class struct_body_gen {
public:
    struct_body_gen(const module& mod, std::string_view name, const structure& str)
        : m_module{&mod}
        , m_name{name}
        , m_struct{&str} {
    }

    void generate(std::ostream& stream) {
        for (auto& [name, member] : str().members) {
            generate_struct_field(name, member);
        }

        raw_struct_gen raw_gen(mod(), "raw_t", str());
        raw_gen.generate(m_sections.priv);

        generate_constructor();

        constexpr auto format = R"__(
        public:
            {}
            {}
        private:
            {}
            raw_t raw;
        )__";

        stream << fmt::format(format,
                              m_sections.ctor.str(),
                              m_sections.pub.str(),
                              m_sections.priv.str())
               << '\n';
    }

private:
    void generate_constructor() {
        std::vector<std::string> arg_names;
        std::vector<std::string> initializer_list;

        for (auto& [member_name, member] : str().members) {
            auto member_type = get_type(member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);
            initializer_list.push_back(fmt::format("p_{}", member_name));

            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names.push_back(
                    fmt::format("const {}& p_{}", identifier, member_name));
                continue;
            }
            arg_names.push_back(fmt::format("const {}* p_{}", identifier, member_name));
        }

        m_sections.ctor << fmt::format("{}({}) : raw{{{}}} {{}}",
                                       m_name,
                                       fmt::join(arg_names, ", "),
                                       fmt::join(initializer_list, ", "));
    }

    void generate_struct_field(std::string_view name, const member& mem) {
        m_sections.pub << generate_getter(name, mem, true) << '\n';
        m_sections.pub << generate_getter(name, mem, false) << '\n';
    }

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const) {
        auto member_type = get_type(mem.type_);
        if (!member_type->is_reference_type(mod())) {
            auto type_name = get_identifier(mod(), mem.type_);
            constexpr auto format = R"__({}& {}() {{ return raw.{}; }})__";
            constexpr auto const_format =
                R"__(const {}& {}() const {{ return raw.{}; }})__";
            return fmt::format(
                is_const ? const_format : format, type_name, member_name, member_name);
        } else {
            // need to dereference before return
            auto& base = std::get<name>(mem.type_.args[0]);
            auto identifier = get_identifier(mod(), base);
            if (!mem.is_nullable()) {
                constexpr auto format =
                    R"__({}& {}() {{ return raw.{}.unsafe().get(); }})__";
                constexpr auto const_format =
                    R"__(const {}& {}() const {{ return raw.{}.unsafe().get(); }})__";
                return fmt::format(is_const ? const_format : format,
                                   identifier,
                                   member_name,
                                   member_name);
            }
        }
        return "";
    }

    const module& mod() {
        return *m_module;
    }

    const structure& str() {
        return *m_struct;
    }

    struct {
        std::stringstream pub, priv, ctor;
        std::stringstream traits;
    } m_sections;

    const module* m_module;
    std::string_view m_name;
    const structure* m_struct;
};

struct cppgen {
    explicit cppgen(const module& mod)
        : m_module(&mod) {
    }

    void generate(std::ostream& str) {
        for (auto& e : m_module->enums) {
            if (is_anonymous(*m_module, e)) {
                continue;
            }

            auto name = nameof(*m_module->symbols->definition_lookup(&e));
            generate(name, e);
        }

        for (auto& u : mod().unions) {
            auto name = nameof(*mod().symbols->definition_lookup(&u));
            generate(name, u);
            // generate_static_asserts(mod(), name, u, str);
            str << '\n';
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(*m_module, s)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&s));
            generate_traits(name, s);
        }

        str << m_sections.enums.str() << '\n';
        str << m_sections.unions.str() << '\n';
        str << "namespace lidl {\n" << traits.str() << "\n}\n";
    }

private:
    void generate(std::string_view union_name, const union_type& u) {
        std::stringstream pub;
        for (auto& [name, member] : u.members) {
            generate_raw_struct_field(name, get_identifier(mod(), member.type_), pub);
        }

        std::stringstream ctor;
        auto enum_name = fmt::format("{}_alternatives", union_name);
        int member_index = 0;
        for (auto& [member_name, member] : u.members) {
            std::string arg_names;
            std::string initializer_list;
            const auto enum_val = u.get_enum().find_by_value(member_index++)->first;

            auto member_type = get_type(member.type_);
            auto identifier = get_user_identifier(mod(), member.type_);
            if (!member_type->is_reference_type(mod()) || !member.is_nullable()) {
                arg_names = fmt::format("const {}& p_{}", identifier, member_name);
                initializer_list = fmt::format("{0}(p_{0})", member_name);
            } else {
                arg_names = fmt::format("const {}* p_{}", identifier, member_name);
                initializer_list = fmt::format(
                    "{0}(p_{0} ? decltype({0}){{*p_{0}}} : decltype({0}){{nullptr}})",
                    member_name);
            }

            ctor << fmt::format("{}({}) : {}, discriminator{{{}::{}}} {{}}\n",
                                union_name,
                                arg_names,
                                initializer_list,
                                enum_name,
                                enum_val);
        }

        constexpr auto format = R"__(class {0} {{
        public:
            {1} alternative() const noexcept {{
                return discriminator;
            }}

            {2}

        private:
            {1} discriminator;
            union {{
                {3}
            }};
        }};)__";

        generate(enum_name, u.get_enum());

        m_sections.unions << fmt::format(
                                 format, union_name, enum_name, ctor.str(), pub.str())
                          << '\n';
        generate_traits(union_name, u);
    }

    void generate_traits(std::string_view union_name, const union_type& u) {
        std::vector<std::string> names;
        for (auto& [name, member] : u.members) {
            names.emplace_back(get_user_identifier(mod(), member.type_));
        }

        constexpr auto format = R"__(
            template <>
            struct union_traits<{}> {{
                using types = meta::list<{}>;
            }};
        )__";

        traits << fmt::format(format, union_name, fmt::join(names, ", "));
    }

    void generate(std::string_view enum_name, const enumeration& e) {
        std::stringstream pub;
        for (auto& [name, member] : e.members) {
            pub << fmt::format("{} = {},\n", name, member.value);
        }

        constexpr auto format = R"__(enum class {} : {} {{
            {}
        }};)__";

        m_sections.enums << fmt::format(format,
                                        enum_name,
                                        get_identifier(*m_module, e.underlying_type),
                                        pub.str())
                         << '\n';
        generate_traits(enum_name, e);
    }

    void generate_traits(std::string_view enum_name, const enumeration& e) {
        std::vector<std::string> names;
        for (auto& [name, val] : e.members) {
            names.emplace_back(fmt::format("\"{}\"", name));
        }

        constexpr auto format = R"__(
            template <>
            struct enum_traits<{}> {{
                static constexpr inline std::array<const char*, {}> names {{{}}};
            }};
        )__";

        traits << fmt::format(format, enum_name, names.size(), fmt::join(names, ", "));
    }

    void generate_traits(std::string_view struct_name, const structure& str) {
        std::vector<std::string> members;
        for (auto& [name, member] : str.members) {
            members.push_back(
                fmt::format("member_info<&{0}::{1}>{{\"{1}\"}}", struct_name, name));
        }


        constexpr auto format = R"__(
            template <>
            struct struct_traits<{}> {{
                static constexpr auto members = std::make_tuple({});
            }};
        )__";

        traits << fmt::format(format, struct_name, fmt::join(members, ", "));
    }


    struct {
        std::stringstream enums;
        std::stringstream structs;
        std::stringstream unions;
        std::stringstream services;
    } m_sections;

    std::stringstream traits;

    const module& mod() {
        return *m_module;
    }

    const module* m_module;
};
} // namespace

std::string specialization_name(std::string_view tmpl, gsl::span<std::string> args) {
    return fmt::format("{}<{}>", tmpl, fmt::join(args, ", "));
}

std::string generate_specialization(const module& m,
                                    std::string_view templ_name,
                                    const structure& s,
                                    std::ostream& str) {
    constexpr auto format = R"__(template <> struct {} {{
        {}
    }};)__";

    std::stringstream body;

    struct_body_gen(m, templ_name, s).generate(body);
    auto attr = s.attributes.get<procedure_params_attribute>("procedure_params");

    std::array<std::string, 1> arg{
        fmt::format("&{}::{}", attr->serv_name, attr->proc_name)};
    auto nm = specialization_name(templ_name, arg);
    str << fmt::format(format, nm, body.str()) << '\n';
    return nm;
}

void generate_struct(const module& m,
                     std::string_view name,
                     const structure& s,
                     std::ostream& str) {
    constexpr auto format = R"__(class {} {{
        {}
    }};)__";

    std::stringstream body;
    struct_body_gen(m, name, s).generate(body);

    str << fmt::format(format, name, body.str()) << '\n';
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
            generate_struct(mod, name, s, str);
            str << '\n';
            generate_static_asserts(mod, name, s, str);
            str << '\n';
        }
    }

    cppgen gen(mod);
    gen.generate(str);

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
            auto name = generate_specialization(mod, "procedure_params_t", s, str);
            str << '\n';
            generate_static_asserts(mod, name, s, str);
            str << '\n';
            str << "}\n";
        }
    }
}
} // namespace lidl