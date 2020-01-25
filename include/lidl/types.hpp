#pragma once

#include "identifier.hpp"

#include <iostream>
#include <lidl/layout.hpp>
#include <memory>
#include <optional>
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

    virtual raw_layout wire_layout(const module& mod) const = 0;

    virtual bool is_reference_type(const module& mod) const = 0;

    virtual ~type() = default;
};

struct value_type : type {
    virtual bool is_raw(const module& mod) const override {
        return true;
    }

    virtual bool is_reference_type(const module&) const override {
        return false;
    }
};

struct reference_type : type {
    virtual bool is_raw(const module& mod) const override {
        return true;
    }

    virtual bool is_reference_type(const module&) const override {
        return true;
    }

    virtual raw_layout wire_layout(const module&) const override {
        return raw_layout{/*.size=*/2,
                          /*.alignment=*/2};
    }
};

namespace detail {
struct future_type : type {
    virtual raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a forward declaration!");
    }

    virtual bool is_reference_type(const module&) const override {
        return false;
    }
};
} // namespace detail
} // namespace lidl