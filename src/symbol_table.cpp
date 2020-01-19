#include <lidl/symbol_table.hpp>

namespace lidl {
const symbol*
symbol_table::lookup(std::string_view symbol_name) const {
    auto it = sym_lookup.find(std::string(symbol_name));
    if (it == sym_lookup.end()) {
        return nullptr;
    }
    return it->second;
}
} // namespace lidl