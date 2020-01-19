#pragma once

#include <lidl/attributes.hpp>
#include <lidl/types.hpp>
#include <map>
#include <algorithm>

namespace lidl {
struct member {
    const type* type_;
    attribute_holder attributes;
};

struct structure {
    bool is_raw(const module& mod) const {
        auto detected = std::all_of(members.begin(), members.end(), [&mod](auto& mem) {
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

    std::map<std::string, member> members;
    attribute_holder attributes;
};

class user_defined_type : public type {
public:
    explicit user_defined_type(structure s)
        : str(std::move(s)) {
    }

    virtual bool is_raw(const module& mod) const override {
        return str.is_raw(mod);
    }

    structure str;
};
} // namespace lidl