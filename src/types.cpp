#include <lidl/types.hpp>

namespace lidl {
name wire_type::get_wire_type(const module& mod) const {
    return type::get_wire_type(mod);
}
} // namespace lidl