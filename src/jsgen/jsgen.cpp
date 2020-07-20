#include "generator.hpp"
#include "get_identifier.hpp"
#include "struct_gen.hpp"

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
            auto generator = struct_gen(
                mod(), sym, name /*, get_identifier(mod(), lidl::name{sym})*/, s);
            m_sections.merge_before(generator.generate());
            exports.emplace_back(name);
        }

        str << "const lidl = require(\"@lidldev/rt\");\n";
        str << R"__(const inspect = Symbol.for('nodejs.util.inspect.custom'))__" << '\n';
        for (auto& out : m_sections.sects) {
            str << out.body << '\n';
        }

        str << fmt::format("module.exports = {{ {} }};", fmt::join(exports, ", "));
    }

private:
    const module& mod() const {
        return *m_mod;
    }

    sections m_sections;
    const module* m_mod;
};
} // namespace

void generate(const module& mod, std::ostream& str) {
    jsgen gen(mod);
    gen.generate(str);
}
} // namespace lidl::js