#include "cppgen.hpp"

#include "emitter.hpp"
#include "enum_gen.hpp"
#include "generator_base.hpp"
#include "generic_gen.hpp"
#include "lidl/enumeration.hpp"
#include "lidl/union.hpp"
#include "struct_bodygen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <fmt/format.h>
#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <lidl/view_types.hpp>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl::cpp {
namespace {
bool is_anonymous(const module& mod, symbol t) {
    return !recursive_definition_lookup(*mod.symbols, t);
}

void declare_template(const module& mod,
                      std::string_view generic_name,
                      const generic& generic,
                      std::ostream& str) {
    std::vector<std::string> params;
    for (auto& [name, param] : generic.declaration) {
        std::string type_name;
        if (dynamic_cast<type_parameter*>(param.get())) {
            type_name = "typename";
        } else {
            type_name = "int32_t";
        }
        params.emplace_back(fmt::format("{} {}", type_name, name));
    }

    str << fmt::format(
        "template <{}>\nclass {};\n", fmt::join(params, ", "), generic_name);
}

std::vector<section_key_t> generate_procedure(const module& mod,
                                              std::string_view proc_name,
                                              const procedure& proc,
                                              std::ostream& str) {
    constexpr auto decl_format = "    virtual {} {}({}) = 0;";

    std::vector<section_key_t> dependencies;

    std::vector<std::string> params;
    for (auto& [param_name, param] : proc.parameters) {
        auto deps = def_keys_from_name(mod, param);
        for (auto& key : deps) {
            dependencies.push_back(key);
        }

        auto param_type = get_type(mod, param);
        if (param_type->is_reference_type(mod)) {
            if (param.args.empty()) {
                throw std::runtime_error(
                    fmt::format("Not a pointer: {}", get_identifier(mod, param)));
            }
            auto identifier = get_identifier(mod, std::get<name>(param.args.at(0)));
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        } else if (auto vt = dynamic_cast<const view_type*>(param_type); vt) {
            auto identifier = get_identifier(mod, param);
            params.emplace_back(fmt::format("{} {}", identifier, param_name));
        } else {
            auto identifier = get_identifier(mod, param);
            params.emplace_back(fmt::format("const {}& {}", identifier, param_name));
        }
    }

    std::string ret_type_name;
    auto deps = def_keys_from_name(mod, proc.return_types.at(0));
    for (auto& key : deps) {
        dependencies.push_back(key);
    }

    auto ret_type = get_type(mod, proc.return_types.at(0));
    ret_type_name = get_user_identifier(mod, proc.return_types.at(0));
    if (ret_type->is_reference_type(mod)) {
        ret_type_name = fmt::format("const {}&", ret_type_name);
        params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
    } else if (auto vt = dynamic_cast<const view_type*>(ret_type); vt) {
        params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
    }

    str << fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));

    return dependencies;
}

sections
generate_service(const module& mod, std::string_view name, const service& service) {
    std::vector<std::string> inheritance;
    for (auto& base : service.extends) {
        inheritance.emplace_back(nameof(base.base));
    }
    inheritance.emplace_back(fmt::format("::lidl::service<{}>", name));

    std::stringstream str;

    section def_sec;

    str << fmt::format(
        "class {}{} {{\npublic:\n",
        name,
        inheritance.empty()
            ? ""
            : fmt::format(" : public {}", fmt::join(inheritance, ", public ")));
    for (auto& [name, proc] : service.procedures) {
        auto deps = generate_procedure(mod, name, proc, str);
        def_sec.depends_on.insert(def_sec.depends_on.end(), deps.begin(), deps.end());
        str << '\n';
    }
    str << fmt::format("    virtual ~{}() = default;\n", name);
    str << "};";
    str << '\n';

    def_sec.key        = {std::string(name), section_type::definition};
    def_sec.definition = str.str();

    return {{std::move(def_sec)}};
}

struct cppgen {
    explicit cppgen(const module& mod)
        : m_module(&mod) {
    }

    void generate(std::ostream& str) {
        std::stringstream forward_decls;
        forward_decls << fmt::format("struct {};\n", name());

        for (auto& generic : mod().generic_unions) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& generic : mod().generic_structs) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }

            auto name = nameof(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& s : mod().unions) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }
            auto name = nameof(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& e : m_module->enums) {
            if (is_anonymous(*m_module, &e)) {
                continue;
            }

            auto sym  = *m_module->symbols->definition_lookup(&e);
            auto name = nameof(sym);
            enum_gen generator(mod(), sym, name, e);
            m_sections.merge_before(generator.generate());
        }

        for (auto& ins : mod().instantiations) {
            auto sym  = *recursive_definition_lookup(*mod().symbols, &ins.generic_type());
            auto name = nameof(sym);
            auto generator = generic_gen(mod(), sym, name, ins);
            m_sections.merge_before(generator.generate());
        }

        for (auto& u : mod().unions) {
            auto sym       = *mod().symbols->definition_lookup(&u);
            auto name      = nameof(sym);
            auto generator = union_gen(mod(), sym, name, u);
            m_sections.merge_before(generator.generate());
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(*m_module, &s)) {
                continue;
            }
            auto sym       = *mod().symbols->definition_lookup(&s);
            auto name      = nameof(sym);
            auto generator = struct_gen(mod(), sym, name, s);
            m_sections.merge_before(generator.generate());
        }

        for (auto& service : mod().services) {
            Expects(!is_anonymous(mod(), &service));
            auto name = nameof(*mod().symbols->definition_lookup(&service));
            m_sections.merge_before(generate_service(mod(), name, service));
        }

        m_sections.merge_before(generate_module_traits());

        str << forward_decls.str() << '\n';

        emitter e(mod(), m_sections);

        str << e.emit() << '\n';
    }

private:
    sections generate_module_traits() {
        static constexpr auto format = R"__(
            template<>
            struct module_traits<{0}> {{
                static constexpr const char* name = "{0}";
                using symbols = meta::list<{1}>;
            }};
        )__";

        std::vector<std::string> symbol_names;
        for (auto& s : m_sections.m_sections) {
            // TODO:
            //            symbol_names.emplace_back(s.name.name);
        }

        section s;
        s.name_space = "lidl";
        s.definition = fmt::format(format, name(), fmt::join(symbol_names, ", "));
        return sections{{std::move(s)}};
    }

    sections m_sections;

    std::string_view name() {
        return mod().module_name;
    }

    const module& mod() {
        return *m_module;
    }

    const module* m_module;
};
} // namespace

void generate_service_descriptor(const module& mod,
                                 std::string_view service_name,
                                 const service& service,
                                 std::ostream& str) {
    std::vector<std::string> tuple_types(service.procedures.size());
    std::transform(service.procedures.begin(),
                   service.procedures.end(),
                   tuple_types.begin(),
                   [&](auto& proc) {
                       auto param_name =
                           get_identifier(mod,
                                          name{*mod.symbols->definition_lookup(
                                              std::get<procedure>(proc).params_struct)});
                       auto res_name =
                           get_identifier(mod,
                                          name{*mod.symbols->definition_lookup(
                                              std::get<procedure>(proc).results_struct)});
                       return fmt::format(
                           "::lidl::procedure_descriptor<&{0}::{1}, {2}, {3}>{{\"{1}\"}}",
                           service_name,
                           std::get<std::string>(proc),
                           param_name,
                           res_name);
                   });
    str << fmt::format("template <> class service_descriptor<{}> {{\npublic:\n",
                       service_name);
    str << fmt::format("static constexpr inline auto procedures = std::make_tuple({});\n",
                       fmt::join(tuple_types, ", "));
    str << fmt::format("static constexpr inline std::string_view name = \"{}\";\n",
                       service_name);
    str << fmt::format(
        "using params_union = {};\n",
        get_identifier(
            mod, name{*mod.symbols->definition_lookup(service.procedure_params_union)}));
    str << fmt::format(
        "using results_union = {};\n",
        get_identifier(
            mod, name{*mod.symbols->definition_lookup(service.procedure_results_union)}));
    str << "};";
}
} // namespace lidl::cpp

namespace lidl {
void generate(const module& mod, std::ostream& str) {
    for (auto& [_, child] : mod.children) {
        generate(*child, str);
    }

    using namespace cpp;
    str << "#pragma once\n\n#include <lidl/lidl.hpp>\n";
    if (!mod.services.empty()) {
        str << "#include <lidl/service.hpp>\n";
    }
    str << '\n';
    //
    //    if (!mod.name_space.empty()) {
    //        str << fmt::format("namespace {} {{\n", mod.name_space);
    //    }
    //    if (!mod.name_space.empty()) {
    //        str << "}\n";
    //    }

    cppgen gen(mod);
    gen.generate(str);

    for (auto& service : mod.services) {
        auto name = nameof(*mod.symbols->definition_lookup(&service));
        str << "namespace lidl {\n";
        generate_service_descriptor(mod, name, service, str);
        str << '\n';
        str << "}\n";
    }
}
} // namespace lidl