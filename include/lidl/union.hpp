//
// Created by fatih on 2/16/20.
//

#pragma once

#include <algorithm>
#include <deque>
#include <lidl/attributes.hpp>
#include <lidl/basic.hpp>
#include <lidl/member.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct union_type : public value_type {
    std::deque<std::tuple<std::string, member>> members;
    attribute_holder attributes;

    const enumeration& get_enum() const;

    union_type() = default;
    union_type(const union_type&) = delete;
    union_type(union_type&&) = default;
    union_type& operator=(union_type&&) = default;

    bool is_reference_type(const module& mod) const override;

    virtual raw_layout wire_layout(const module& mod) const override;

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override;
};

} // namespace lidl