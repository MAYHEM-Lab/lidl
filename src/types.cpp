#include <lidl/module.hpp>
#include <lidl/types.hpp>

namespace lidl {
name reference_type::get_wire_type_name_impl(const module& mod, const name& your_name) const {
    auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();
    if (your_name.base == ptr_sym) {
        return your_name;
    }
    return name{ptr_sym, {your_name}};
}

name wire_type::get_wire_type_name_impl(const module& mod, const name& your_name) const {
    if (is_reference_type(mod)) {
        auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();
        if (your_name.base == ptr_sym) {
            return your_name;
        }
        return name{ptr_sym, {your_name}};
    }
    // we're already a wire type
    return your_name;
}
} // namespace lidl