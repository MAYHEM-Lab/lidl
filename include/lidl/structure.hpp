#pragma once

#include <algorithm>
#include <lidl/attributes.hpp>
#include <lidl/types.hpp>
#include <map>

namespace lidl {
struct member {
    const type* type_;
    attribute_holder attributes;
};

struct structure : public type {
    std::map<std::string, member> members;
    attribute_holder attributes;

    virtual bool is_raw(const module& mod) const override {
        auto detected =
            std::all_of(members.begin(), members.end(), [&mod](auto& mem) {
                return mem.second.type_->is_raw(mod);
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

    virtual raw_layout wire_layout(const module& mod) const {
        aggregate_layout_computer computer;
        for (auto& member : members) {
            auto layout = member.second.type_->wire_layout(mod);
            computer.add(layout);
        }
        return computer.get();
    }
};
} // namespace lidl