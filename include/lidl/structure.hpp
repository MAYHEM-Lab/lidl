#pragma once

#include <lidl/attributes.hpp>
#include <lidl/types.hpp>
#include <map>

namespace lidl {
struct member {
    const type* type_;
    attribute_holder attributes;
};

struct structure {
    bool is_raw() const {
        auto detected = std::all_of(members.begin(), members.end(), [](auto& mem) {
            return mem.second.type_->is_raw();
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
    user_defined_type(identifier name, const structure& s)
        : type(name)
        , str(s) {
    }

    virtual bool is_raw() const override {
        return str.is_raw();
    }

    structure str;
};
} // namespace lidl