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

    std::vector<section_key_t> return_deps;

    for (auto& ret : proc.return_types) {
        auto deps = def_keys_from_name(mod, ret);
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
        } else if (auto vt = dynamic_cast<const view_type*>(ret_type); vt) {
            params.emplace_back(fmt::format("::lidl::message_builder& response_builder"));
        }
    }

    str << fmt::format(decl_format, ret_type_name, proc_name, fmt::join(params, ", "));

    return dependencies;
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
                           serv_full_name,
                           std::get<std::string>(proc),
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

sections
generate_service(const module& mod, std::string_view name, const service& service) {
    std::vector<std::string> inheritance;
    for (auto& base : service.extends) {
        inheritance.emplace_back(get_identifier(mod, {base.base}));
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

    auto serv_handle    = *recursive_definition_lookup(*mod.symbols, &service);
    auto serv_full_name = get_identifier(mod, {serv_handle});

    def_sec.key        = {serv_handle, section_type::definition};
    def_sec.definition = str.str();
    def_sec.name_space = mod.name_space;

    auto sects = generate_service_descriptor(mod, name, service);
    sects.add(std::move(def_sec));

    return sects;
}

struct cppgen {
    explicit cppgen(const module& mod)
        : m_module(&mod) {
    }

    void generate(std::ostream& str) {
        std::stringstream forward_decls;
        forward_decls << "namespace " << mod().name_space << " {\n";
        forward_decls << fmt::format("struct {};\n", name());

        for (auto& generic : mod().generic_unions) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = local_name(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& generic : mod().generic_structs) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = local_name(*mod().symbols->definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }

            auto name = local_name(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& s : mod().unions) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }
            auto name = local_name(*mod().symbols->definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }
        forward_decls << "}\n";

        for (auto& e : m_module->enums) {
            if (is_anonymous(*m_module, &e)) {
                continue;
            }

            auto sym  = *m_module->symbols->definition_lookup(&e);
            auto name = local_name(sym);
            enum_gen generator(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), e);
            m_sections.merge_before(generator.generate());
        }

        for (auto& ins : mod().instantiations) {
            auto sym  = *recursive_definition_lookup(*mod().symbols, &ins.generic_type());
            auto name = local_name(sym);
            auto generator = generic_gen(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), ins);
            m_sections.merge_before(generator.generate());
        }

        for (auto& u : mod().unions) {
            auto sym  = *mod().symbols->definition_lookup(&u);
            auto name = local_name(sym);
            auto generator =
                union_gen(mod(), sym, name, get_identifier(mod(), lidl::name{sym}), u);
            m_sections.merge_before(generator.generate());
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(*m_module, &s)) {
                continue;
            }
            auto sym  = *mod().symbols->definition_lookup(&s);
            auto name = local_name(sym);
            auto generator =
                struct_gen(mod(), sym, name, get_identifier(mod(), lidl::name{sym}), s);
            m_sections.merge_before(generator.generate());
        }

        for (auto& service : mod().services) {
            Expects(!is_anonymous(mod(), &service));
            auto name = local_name(*mod().symbols->definition_lookup(&service));
            m_sections.merge_before(generate_service(mod(), name, service));
        }

        // TODO: fix module traits
        //        m_sections.merge_before(generate_module_traits());

        //        str << forward_decls.str() << '\n';

        emitter e(*mod().parent, mod(), m_sections);

        str << "#pragma once\n\n#include <lidl/lidl.hpp>\n";
        if (!mod().services.empty()) {
            str << "#include <lidl/service.hpp>\n";
        }
        str << '\n';

        str << e.emit() << '\n';
    }

private:
    sections generate_module_traits() {
        static constexpr auto format = R"__(struct module_traits {{
                inline static constexpr const char* name = "{0}";
                using symbols = ::lidl::meta::list<{1}>;
            }};)__";

        std::vector<std::string> symbol_names;
        for (auto& s : m_sections.m_sections) {
            // TODO:
            //            symbol_names.emplace_back(s.name.name);
        }

        section sec;
        for (auto& s : mod().symbols->all_handles()) {
            auto sym = get_symbol(s);
            using std::get_if;
            if (get_if<const type*>(&sym) || get_if<const generic*>(&sym) ||
                get_if<const service*>(&sym)) {
                symbol_names.push_back(get_identifier(mod(), {s}));
                sec.add_dependency({s, section_type::definition});
            }
        }

        sec.name_space = mod().name_space;
        sec.definition = fmt::format(format, name(), fmt::join(symbol_names, ", "));
        return sections{{std::move(sec)}};
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
} // namespace lidl::cpp

namespace lidl {
void generate(const module& mod, std::ostream& str) {
    for (auto& [_, child] : mod.children) {
        generate(*child, str);
    }

    using namespace cpp;
    cppgen gen(mod);
    gen.generate(str);
}
} // namespace lidl