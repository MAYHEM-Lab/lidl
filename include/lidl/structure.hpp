#pragma once

#include <algorithm>
#include <lidl/attributes.hpp>
#include <lidl/types.hpp>
#include <deque>

namespace lidl {
struct member {
    const type* type_;
    attribute_holder attributes;

    bool is_nullable() const {
        if (auto attr = attributes.get<detail::nullable_attribute>("nullable"); attr && attr->nullable) {
            return true;
        }
        return false;
    }
};

struct structure : public value_type {
    std::deque<std::tuple<std::string, member>> members;
    attribute_holder attributes;

    bool is_reference_type(const module& mod) const override {
        return std::any_of(members.begin(), members.end(), [&](auto& mem) {
            auto& [name, member] = mem;
            return member.type_->is_reference_type(mod);
        });
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        aggregate_layout_computer computer;
        for (auto& [name, member] : members) {
            if (member.type_->is_reference_type(mod)) {
                computer.add({2, 2});
            } else {
                computer.add(member.type_->wire_layout(mod));
            }
        }
        return computer.get();
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override {
        YAML::Node node;

        auto cur_span = span;
        for (auto it = members.rbegin(); it != members.rend(); ++it) {
            auto& [name, member] = *it;
            node[name] = member.type_->bin2yaml(module, cur_span).first;
            cur_span = cur_span.subspan(0, cur_span.size() - member.type_->wire_layout(module).size());
        }

        return {node, wire_layout(module).size()};
    }
};
} // namespace lidl