#pragma once

#include <algorithm>
#include <deque>
#include <lidl/attributes.hpp>
#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <lidl/member.hpp>

namespace lidl {
struct structure : public value_type {
    std::deque<std::tuple<std::string, member>> members;
    attribute_holder attributes;
    std::weak_ptr<scope> scope_;

    structure() = default;
    structure(const structure&) = delete;
    structure(structure&&) = default;
    structure& operator=(structure&&) = default;

    bool is_reference_type(const module& mod) const override;

    virtual raw_layout wire_layout(const module& mod) const override;

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override;
};
} // namespace lidl