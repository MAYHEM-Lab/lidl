#include <lidl/module.hpp>
#include <string_view>

namespace lidl {
void generate(const module& mod, std::ostream& str);
void pybindgen(std::string_view name, module& mod, std::ostream& str) {
    std::stringstream src;
    generate(mod, src);
}
} // namespace lidl
