#include <lidl/module.hpp>
#include <lidl/types.hpp>

namespace lidl {
name type::get_wire_type_name(const module& mod, const name& your_name) const {
    if (is_value(mod)) {
        return your_name;
    }

    if (is_reference_type(mod)) {
        auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();
        return name{ptr_sym, {your_name}};
    }

    assert(false && "Must override get_wire_type_name");
}
} // namespace lidl