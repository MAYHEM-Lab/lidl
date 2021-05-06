#pragma once

#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct span_type : view_type {
public:
    span_type(const module& mod, const generic_instantiation& ins)
        : view_type(name{recursive_name_lookup(mod.symbols(), "vector").value(),
                         ins.get_name().args},
                    nullptr)
        , m_ins{ins} {
    }

private:
    generic_instantiation m_ins;
};

struct generic_span_type : generic {
    generic_span_type()
        : generic(make_generic_declaration({{"T", "type"}})) {
    }

    std::unique_ptr<type> instantiate(const module& mod,
                                      const generic_instantiation& ins) const override {
        return std::make_unique<span_type>(mod, ins);
    }
};
} // namespace lidl