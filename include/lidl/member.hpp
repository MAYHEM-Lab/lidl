#pragma once

#include "attributes.hpp"
#include "basic.hpp"

namespace lidl {
struct member {
    name type_;
    attribute_holder attributes;

    member() = default;
    member(const member&) = delete;
    member(member&&) noexcept = default;

    member& operator=(member&&) noexcept = default;

    bool is_nullable() const {
        if (auto attr = attributes.get<detail::nullable_attribute>("nullable");
            attr && attr->nullable) {
            return true;
        }
        return false;
    }
};
}