#pragma once

#include "basic.hpp"
#include "source_info.hpp"

#include <lidl/base.hpp>

namespace lidl {
struct member : public base {
    member() = default;
    explicit member(name type) : type_{std::move(type)} {}

    name type_;
    bool nullable = false;

    bool is_nullable() const {
        return nullable;
    }
};
} // namespace lidl