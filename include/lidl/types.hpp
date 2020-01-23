#pragma once

#include "identifier.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <lidl/layout.hpp>
#include <optional>

namespace lidl {
class module;
struct type {
public:
    virtual bool is_raw(const module& mod) const {
        return false;
    }

    virtual raw_layout wire_layout(const module& mod) const = 0;

    virtual ~type() = default;
};

namespace detail {
struct future_type : type {
    virtual raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a forward declaration!");
    }
};
} // namespace detail
} // namespace lidl