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

bool reference_type_pass(const module& mod, name& n) {
    auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();

    if (n.base == ptr_sym) {
        // The name is already a pointer
        return false;
    }

    auto member_type = get_type(mod, n);
    if (!member_type->is_reference_type(mod)) {
        return false;
    }

    if (auto gen = dynamic_cast<const generic_instantiation*>(member_type); gen) {
        if (dynamic_cast<const generic_structure*>(&gen->generic_type()) ||
            dynamic_cast<const generic_union*>(&gen->generic_type())) {
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
            reference_type_pass(mod, *sym);
        }
    }

    n = pointerify(ptr_sym, n);
    return true;
}
} // namespace

bool reference_type_pass(module& m) {
    bool changed = false;

    for (auto& s : m.structs) {
        for (auto& [_, member] : s.own_members()) {
            changed |= reference_type_pass(m, member.type_);
        }
    }
    for (auto& s : m.unions) {
        for (auto& [_, member] : s->own_members()) {
            changed |= reference_type_pass(m, member.type_);
        }
    }
    for (auto& s : m.services) {
        for (auto& [_, proc] : s->own_procedures()) {
            for (auto& ret_type : proc->return_types) {
                changed |= reference_type_pass(m, ret_type);
            }
            for (auto& [_name, param] : proc->parameters) {
                changed |= reference_type_pass(m, param.type);
            }

            if (proc->params_struct && proc->results_struct) {
                for (auto& [_, member] : proc->params_struct->own_members()) {
                    changed |= reference_type_pass(m, member.type_);
                }
                for (auto& [_, member] : proc->results_struct->own_members()) {
                    changed |= reference_type_pass(m, member.type_);
                }
            }
        }
        if (s->procedure_params_union && s->procedure_results_union) {
            for (auto& [_, member] : s->procedure_params_union->own_members()) {
                changed |= reference_type_pass(m, member.type_);
            }
            for (auto& [_, member] : s->procedure_results_union->own_members()) {
                changed |= reference_type_pass(m, member.type_);
            }
        }
    }

    return changed;
}
} // namespace lidl