#pragma once

#include "identifier.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
class module;
struct type {
public:
    virtual bool is_raw(const module& mod) const {
        return false;
    }

    virtual int32_t wire_size_bytes(const module& mod) const = 0;

    virtual ~type() = default;
};

namespace detail {
struct future_type : type {};
} // namespace detail
} // namespace lidl