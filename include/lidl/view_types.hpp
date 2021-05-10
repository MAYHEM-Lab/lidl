#pragma once

#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct span_type_instantiation : known_view_type {
public:
    span_type_instantiation(const module& mod, const name& ins)
        : known_view_type(
              name{recursive_name_lookup(mod.symbols(), "vector").value(), ins.args},
              nullptr) {
    }
};

struct generic_span_type : instance_based_view_type {
    generic_span_type(module& mod)
        : instance_based_view_type(&mod, {},make_generic_declaration({{"T", "type"}})) {
    }

    std::unique_ptr<view_type> instantiate(const module& mod,
                                           const name& ins) const override {
        return std::make_unique<span_type_instantiation>(mod, ins);
    }
};
} // namespace lidl