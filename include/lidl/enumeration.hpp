#pragma once

#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <stdexcept>

namespace lidl {
struct enum_member {
    int value;
    std::optional<source_info> src_info;
};

struct enumeration : value_type {
public:
    name underlying_type;
    std::vector<std::pair<std::string, enum_member>> members;

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

    int find_by_name(std::string_view name) const {
        auto it = std::find_if(members.begin(), members.end(), [name](auto& member) {
            return member.first == name;
        });
        return it->second.value;
    }

    virtual YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override;

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        auto val        = node.as<int64_t>();
        auto underlying = get_type(mod, underlying_type);
        writer.align(underlying->wire_layout(mod).alignment());
        auto pos = writer.tell();
        underlying->yaml2bin(mod, node, writer);
        return pos;
    }

private:
};
} // namespace lidl