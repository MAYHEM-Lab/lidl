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

    virtual bool is_raw(const module& mod) const override {
        auto detected =
            std::all_of(members.begin(), members.end(), [&mod](auto& mem) {
                auto& [name, member] = mem;
                return member.type_->is_raw(mod);
            });
        auto attrib = attributes.get<detail::raw_attribute>("raw");
        if (attrib) {
            if (attrib->raw && !detected) {
                throw std::runtime_error("Forced raw struct cannot be satisfied!");
            }
            return attrib->raw;
        }
        return detected;
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        aggregate_layout_computer computer;
        for (auto& [name, member] : members) {
            auto layout = member.type_->wire_layout(mod);
            computer.add(layout);
        }
        return computer.get();
    }
};
} // namespace lidl