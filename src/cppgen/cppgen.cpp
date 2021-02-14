#include "cppgen.hpp"

#include "emitter.hpp"
#include "enum_gen.hpp"
#include "generator_base.hpp"
#include "generic_gen.hpp"
#include "lidl/enumeration.hpp"
#include "lidl/union.hpp"
#include "raw_union_gen.hpp"
#include "service_stub_gen.hpp"
#include "struct_bodygen.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <algorithm>
#include <codegen.hpp>
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
using codegen::sections;
namespace {
bool is_anonymous(const module& mod, symbol t) {
    return !recursive_definition_lookup(mod.symbols(), t);
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

            auto name = local_name(*mod().symbols().definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& generic : mod().generic_structs) {
            if (is_anonymous(mod(), &generic)) {
                continue;
            }

            auto name = local_name(*mod().symbols().definition_lookup(&generic));
            declare_template(mod(), name, generic, forward_decls);
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }

            auto name = local_name(*mod().symbols().definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }

        for (auto& s : mod().unions) {
            if (is_anonymous(mod(), &s)) {
                continue;
            }
            auto name = local_name(*mod().symbols().definition_lookup(&s));
            forward_decls << "class " << name << ";\n";
        }
        forward_decls << "}\n";

        for (auto& e : m_module->enums) {
            if (is_anonymous(*m_module, &e)) {
                continue;
            }

            auto sym  = *m_module->symbols().definition_lookup(&e);
            auto name = local_name(sym);
            enum_gen generator(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), e);
            m_sections.merge_before(generator.generate());
        }

        for (auto& ins : mod().instantiations) {
            auto sym = *recursive_definition_lookup(mod().symbols(), &ins.generic_type());
            auto name      = local_name(sym);
            auto generator = generic_gen(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), ins);
            m_sections.merge_before(generator.generate());
        }

        for (auto& u : mod().unions) {
            auto sym  = *mod().symbols().definition_lookup(&u);
            auto name = local_name(sym);
            if (u.raw) {
                auto generator = raw_union_gen(
                    mod(), sym, name, get_identifier(mod(), lidl::name{sym}), u);
                m_sections.merge_before(generator.generate());
            } else {
                auto generator = union_gen(
                    mod(), sym, name, get_identifier(mod(), lidl::name{sym}), u);
                m_sections.merge_before(generator.generate());
            }
        }

        for (auto& s : mod().structs) {
            if (is_anonymous(*m_module, &s)) {
                continue;
            }
            auto sym  = *mod().symbols().definition_lookup(&s);
            auto name = local_name(sym);
            auto generator =
                struct_gen(mod(), sym, name, get_identifier(mod(), lidl::name{sym}), s);
            m_sections.merge_before(generator.generate());
        }

        for (auto& service : mod().services) {
            Expects(!is_anonymous(mod(), &service));
            auto sym = *mod().symbols().definition_lookup(&service);

            auto name      = local_name(sym);
            auto generator = service_generator(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), service);
            auto stub_generator = remote_stub_generator(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), service);
            auto svc_generator = svc_stub_generator(
                mod(), sym, name, get_identifier(mod(), lidl::name{sym}), service);

            m_sections.merge_before(generator.generate());
            m_sections.merge_before(stub_generator.generate());
            m_sections.merge_before(svc_generator.generate());
        }

        // TODO: fix module traits
        //        m_sections.merge_before(generate_module_traits());

        codegen::emitter e(*mod().parent, mod(), m_sections);

        str << "#pragma once\n\n#include <lidlrt/lidl.hpp>\n";
        if (!mod().services.empty()) {
            str << "#include <lidlrt/service.hpp>\n";
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
        for (auto& s : m_sections.get_sections()) {
            // TODO:
            //            symbol_names.emplace_back(s.name.name);
        }

        section sec;
        for (auto& s : mod().symbols().all_handles()) {
            auto sym = get_symbol(s);
            using std::get_if;
            if (!dynamic_cast<const scope*>(sym) && sym != &forward_decl) {
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

class backend : public codegen::backend {
public:
    void generate(const module& mod, std::ostream& str) override {
        codegen::detail::current_backend = this;
        for (auto& [_, child] : mod.children) {
            generate(*child, str);
        }

        cppgen gen(mod);
        gen.generate(str);
    }
    std::string get_user_identifier(const module& mod,
                                    const lidl::name& name) const override {
        return cpp::get_user_identifier(mod, name);
    }
    std::string get_local_identifier(const module& mod,
                                     const lidl::name& name) const override {
        return cpp::get_local_identifier(mod, name);
    }
    std::string get_identifier(const module& mod, const lidl::name& name) const override {
        return cpp::get_identifier(mod, name);
    }
};
} // namespace

std::unique_ptr<codegen::backend> make_backend() {
    return std::make_unique<cpp::backend>();
}
} // namespace lidl::cpp