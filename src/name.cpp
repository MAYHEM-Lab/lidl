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

const type* get_type(const module& mod, const name& n) {
    assert(is_type(n));

    auto base = get_symbol(n.base);

    if (auto base_type = dynamic_cast<const type*>(base); base_type) {
        assert(n.args.empty());
        return base_type;
    }

    auto base_type = dynamic_cast<const generic_type*>(base);
    if (!base_type) {
        return nullptr;
    }

    if (base_type->declaration.arity() != n.args.size()) {
        throw std::runtime_error(
            fmt::format("mismatched number of args and params for generic: "
                        "expected {}, got {}",
                        base_type->declaration.arity(),
                        n.args.size()));
    }

    return dynamic_cast<const type*>(&mod.create_or_get_instantiation(n));
}

bool operator==(const name& left, const name& right) {
    return left.base == right.base && left.args == right.args;
}
} // namespace lidl