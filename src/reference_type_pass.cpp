//
// Created by fatih on 1/31/20.
//

#include "lidl/basic.hpp"

#include <lidl/module.hpp>

namespace lidl {
namespace {
name pointerify(const symbol_handle& ptr_sym, const name& n) {
    assert(ptr_sym != n.base);
    return name{ptr_sym, {n}};
}
} // namespace

bool add_pointer_to_name_if_needed(const module& mod, name& n) {
    if (!is_type(n)) {
        return false;
    }

    auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();

    if (n.base == ptr_sym) {
        // The name is already a pointer
        return false;
    }

    auto member_type = get_type(mod, n);

    if (!member_type) {
        return false;
    }

    if (!member_type->is_reference_type(mod)) {
        return false;
    }

    if (auto gen = dynamic_cast<const basic_generic_instantiation*>(member_type); gen) {
        if (dynamic_cast<const generic_structure*>(gen->get_generic()) ||
            dynamic_cast<const generic_union*>(gen->get_generic())) {
            /**
             * The type is a user defined generic type. For instance, my_gen<string>.
             * We do not wish to expose the internal pointer implementation to the user,
             * so we don't make the individual arguments pointers, only the entire thing.
             * i.e. we return ptr<my_gen<string>> rather than ptr<my_gen<ptr<string>>>
             */
            n = pointerify(ptr_sym, n);
            return true;
        }
    }

    for (auto& arg : n.args) {
        if (auto sym = std::get_if<name>(&arg); sym) {
            add_pointer_to_name_if_needed(mod, *sym);
        }
    }

    n = pointerify(ptr_sym, n);
    return true;
}
} // namespace lidl