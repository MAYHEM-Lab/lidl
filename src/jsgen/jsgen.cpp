#include "generator.hpp"
#include "get_identifier.hpp"
#include "struct_gen.hpp"

#include <codegen.hpp>
#include <emitter.hpp>
#include <lidl/module.hpp>
#include <ostream>

namespace lidl::js {
namespace {
class jsgen {
public:
    explicit jsgen(const module& mod)
        : m_mod{&mod} {
    }

    void generate(std::ostream& str) {
        std::vector<std::string> exports;

        for (auto& s : mod().structs) {
            //            if (is_anonymous(*m_module, &s)) {
            //                continue;
            //            }
            auto sym       = *mod().symbols->definition_lookup(&s);
            auto name      = local_name(sym);
            auto generator =
                struct_gen(mod(), sym, name, get_local_obj_name(mod(), lidl::name{sym}), s);
            m_sections.merge_before(generator.generate());
            exports.emplace_back(name);
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
    std::string get_local_identifier(const module& mod,
                                     const lidl::name& name) const override {
        return js::get_local_obj_name(mod, name);
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