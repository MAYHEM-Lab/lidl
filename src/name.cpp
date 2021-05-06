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

const wire_type* get_wire_type(const module& mod, const name& n) {
    auto t = get_type(mod, n);
    if (!t) {
        return nullptr;
    }

    if (auto wt = dynamic_cast<const wire_type*>(t)) {
        return wt;
    }

    auto wire_name = t->get_wire_type_name(mod, n);
    assert(wire_name != n);
    return get_wire_type(mod, wire_name);
}

const type* get_type(const module& mod, const name& n) {
    assert(is_type(n));
    return dynamic_cast<const type*>(resolve(mod, n));
}

bool operator==(const name& left, const name& right) {
    return left.base == right.base && left.args == right.args;
}
} // namespace lidl