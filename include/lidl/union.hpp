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

    union_type() = default;
    union_type(const union_type&) = delete;
    union_type(union_type&&) = default;
    union_type& operator=(union_type&&) = default;

    bool is_reference_type(const module& mod) const override {
        return std::any_of(members.begin(), members.end(), [&](auto& mem) {
            auto& [name, member] = mem;
            return get_type(member.type_)->is_reference_type(mod);
        });
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        return std::accumulate(
            members.begin(),
            members.end(),
            raw_layout{0, 0},
            [&](const raw_layout& cur, auto& mem) {
                auto member_layout =
                    get_type(std::get<member>(mem).type_)->wire_layout(mod);
                auto max_align = std::max(cur.alignment(), member_layout.alignment());
                auto max_size = std::max(cur.size(), member_layout.size());
                return raw_layout{max_size, max_align};
            });
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override {
        YAML::Node node;

        auto cur_span = span;
        for (auto it = members.rbegin(); it != members.rend(); ++it) {
            auto& [name, member] = *it;
            node[name] = get_type(member.type_)->bin2yaml(module, cur_span).first;
            cur_span = cur_span.subspan(
                0, cur_span.size() - get_type(member.type_)->wire_layout(module).size());
        }

        return {node, wire_layout(module).size()};
    }
};

} // namespace lidl