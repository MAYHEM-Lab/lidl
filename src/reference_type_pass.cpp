//
// Created by fatih on 1/31/20.
//

#include "lidl/basic.hpp"

#include <lidl/module.hpp>



namespace lidl {
namespace {
name pointerify(const symbol_handle& ptr_sym, const name& n) {
    name ptr_to_name;
    ptr_to_name.base = ptr_sym;
    ptr_to_name.args = std::vector<generic_argument>{n};
    return ptr_to_name;
}

void reference_type_pass(const module& mod, name& n) {
    auto ptr_sym = recursive_name_lookup(*mod.symbols, "ptr").value();
    auto member_type = get_type(mod, n);
    if (!member_type->is_reference_type(mod)) {
        return;
    }

    if (auto gen = dynamic_cast<const generic_instantiation*>(member_type); gen) {
        if (auto ptr = dynamic_cast<const pointer_type*>(&gen->generic_type()); ptr) {
            // The member is already a pointer
            return;
        }

        for (auto& arg : n.args) {
            if (auto sym = std::get_if<name>(&arg); sym) {
                reference_type_pass(mod, *sym);
            }
        }
    }

    n = pointerify(ptr_sym, n);
}
} // namespace

void reference_type_pass(module& m) {
    for (auto& s : m.structs) {
        for (auto& [_, member] : s.members) {
            reference_type_pass(m, member.type_);
        }
    }
    for (auto& s : m.unions) {
        for (auto& [_, member] : s.members) {
            reference_type_pass(m, member.type_);
        }
    }
    for (auto& s : m.services) {
        for (auto& [_, proc] : s.procedures) {
            for (auto& ret_type : proc.return_types) {
                reference_type_pass(m, ret_type);
            }
            for (auto& [_, param_type] : proc.parameters) {
                reference_type_pass(m, param_type);
            }
        }
    }
}
} // namespace lidl