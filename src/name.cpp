#include "lidl/basic.hpp"
#include "lidl/scope.hpp"

#include <cassert>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>


namespace lidl {
const type* get_type(const module& mod, const name& n) {
    auto base = get_symbol(n.base);

    if (auto base_type = dynamic_cast<const type*>(base); base_type) {
        assert(n.args.empty());
        return base_type;
    }

    auto base_type = &dynamic_cast<const generic&>(*base);
    if (base_type->declaration.arity() != n.args.size()) {
        throw std::runtime_error(
            fmt::format("mismatched number of args and params for generic: "
                        "expected {}, got {}",
                        base_type->declaration.arity(),
                        n.args.size()));
    }

    return &mod.create_or_get_instantiation(n);
}

bool operator==(const name& left, const name& right) {
    return left.base == right.base && left.args == right.args;
}
} // namespace lidl