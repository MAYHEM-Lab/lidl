#include <lidl/types.hpp>

namespace lidl {
std::optional<name> wire_type::get_wire_type(const module& mod) const {
    return type::get_wire_type(mod);
}

const wire_type* get_wire_type(const module& mod, const name& n) {
    auto wire_t = get_type<wire_type>(mod, n);
    if (!wire_t) {
        auto t = get_type(mod, n);
        if (!t) {
            return nullptr;
        }
        auto wire_name = t->get_wire_type(mod);
        if (!wire_name) {
            return nullptr;
        }
        return get_type<wire_type>(mod, *wire_name);
    }
    return wire_t;
}
} // namespace lidl