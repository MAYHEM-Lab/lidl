#include "enum_gen.hpp"
#include "generator.hpp"
#include "get_identifier.hpp"
#include "struct_gen.hpp"
#include "union_gen.hpp"

#include <codegen.hpp>
#include <emitter.hpp>
#include <lidl/module.hpp>
#include <ostream>

namespace lidl::js {
namespace {
bool is_anonymous(const module& mod, symbol t) {
    return !recursive_definition_lookup(mod.symbols(), t);
}

class jsgen {
public:
    explicit jsgen(const module& mod)
        : m_mod{&mod} {
    }

    void generate(std::ostream& str) {
        std::vector<std::string> exports;

        for (auto& serv : mod().services) {
            {
                auto call_union = &serv->procedure_params_union(mod());
                auto sym = *serv->get_scope().definition_lookup(call_union);
                auto union_name = local_name(sym);
            }
            {
                auto call_union = &serv->procedure_results_union(mod());
                auto sym = *serv->get_scope().definition_lookup(call_union);
                auto union_name = local_name(sym);
            }
        }

        for (auto& s : mod().structs) {
            m_sections.merge_before(codegen::do_generate<struct_gen>(mod(), s.get()));
        }

        for (auto& e : mod().enums) {
            m_sections.merge_before(codegen::do_generate<enum_gen>(mod(), e.get()));
        }

        for (auto& u : mod().unions) {
            m_sections.merge_before(codegen::do_generate<union_gen>(mod(), u.get()));
        }

        codegen::emitter e(*mod().parent, mod(), m_sections);

        str << "import * as lidl from \"@lidldev/rt\";\n";
        str << '\n';

        str << e.emit() << '\n';
    }

private:
    const module& mod() const {
        return *m_mod;
    }

    codegen::sections m_sections;
    const module* m_mod;
};
class backend : public codegen::backend {
public:
    void generate(const module& mod, std::ostream& str) override {
        codegen::detail::current_backend = this;
        for (auto& [_, child] : mod.children) {
            generate(*child, str);
        }

        jsgen gen(mod);
        gen.generate(str);
    }
    std::string get_user_identifier(const module& mod,
                                    const lidl::name& name) const override {
        return js::get_local_user_obj_name(mod, name);
    }

    std::string get_identifier(const module& mod, const lidl::name& name) const override {
        return js::get_local_obj_name(mod, name);
    }
};
} // namespace

std::unique_ptr<codegen::backend> make_backend() {
    return std::make_unique<js::backend>();
}
} // namespace lidl::js