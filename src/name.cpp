#include "lidl/basic.hpp"
#include "lidl/scope.hpp"

#include <cassert>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>


namespace lidl {
bool is_type(const name& n) {
    return get_symbol(n.base)->category() == base::categories::type ||
           get_symbol(n.base)->category() == base::categories::generic_type;
}

bool is_generic(const name& n) {
    return get_symbol(n.base)->is_generic();
}

bool is_view(const name& n) {
    return get_symbol(n.base)->is_view();
}

bool is_service(const name& n) {
    return get_symbol(n.base)->category() == base::categories::service;
}

const base* resolve(const module& mod, const name& n) {
    auto base = get_symbol(n.base);

    if (!base->is_generic()) {
        assert(n.args.empty());
        return base;
    }

    auto* generic_base = dynamic_cast<const basic_generic*>(base);

    if (n.args.empty()) {
        // We're just resolving the generic itself, not an instance.
        return generic_base;
    }

    if (generic_base->declaration.arity() != n.args.size()) {
        std::cerr << fmt::format("mismatched number of args and params for generic: "
                                 "expected {}, got {}\n",
                                 generic_base->declaration.arity(),
                                 n.args.size());
        assert(false);
    }

    return dynamic_cast<const lidl::base*>(&mod.create_or_get_instantiation(n));
}

name get_wire_type_name(const module& mod, const name& n) {
    auto t = resolve(mod, n);
    assert(t);

    auto wire_name = t->get_wire_type_name_impl(mod, n);
    if (wire_name == n) {
        // n's wire type name is the same as n, it itself must be a wire type!
        return n;
    }
    return get_wire_type_name(mod, wire_name);
}

const wire_type* get_wire_type(const module& mod, const name& n) {
    auto wire_name = get_wire_type_name(mod, n);
    auto t = resolve(mod, wire_name);

    if (!t) {
        return nullptr;
    }

    return static_cast<const wire_type*>(t);
}

const type* get_type(const module& mod, const name& n) {
    assert(is_type(n));
    return dynamic_cast<const type*>(resolve(mod, n));
}

bool operator==(const name& left, const name& right) {
    return left.base == right.base && left.args == right.args;
}

const name& deref_ptr(const module& mod, const name& nm) {
    auto ptr_sym = recursive_name_lookup(mod.symbols(), "ptr").value();
    if (nm.base == ptr_sym) {
        return nm.args[0].as_name();
    }
    return nm;
}
} // namespace lidl