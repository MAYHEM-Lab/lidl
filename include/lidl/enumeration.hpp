#pragma once

#include <lidl/attributes.hpp>
#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <stdexcept>

namespace lidl {
struct enum_member {
    int value;
};

struct enumeration : type {
public:
    name underlying_type;
    std::vector<std::pair<std::string, enum_member>> members;

    bool is_reference_type(const module& mod) const override {
        return false;
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        return get_type(mod, underlying_type)->wire_layout(mod);
    }

    std::vector<std::pair<std::string, enum_member>>::const_iterator
    find_by_value(int val) const {
        auto it = std::find_if(members.begin(), members.end(), [val](auto& member) {
            return member.second.value == val;
        });
        return it;
    }

    virtual std::pair<YAML::Node, size_t>
    bin2yaml(const module& mod, gsl::span<const uint8_t> data) const override {
        auto integral =
            get_type(mod, underlying_type)->bin2yaml(mod, data).first.as<uint64_t>();
        auto it = std::find_if(members.begin(), members.end(), [integral](auto& member) {
            return member.second.value == integral;
        });
        if (it == members.end()) {
            throw std::runtime_error("unknown enum value");
        }
        return {YAML::Node(it->first), 0};
    }

private:
};

struct union_enum_attribute : attribute {
    union_enum_attribute(enumeration* e)
        : attribute("union_enum")
        , union_enum(e) {
    }

    enumeration* union_enum;
};
} // namespace lidl