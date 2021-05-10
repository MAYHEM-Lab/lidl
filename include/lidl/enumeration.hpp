#pragma once

#include <deque>
#include <lidl/basic.hpp>
#include <lidl/inheritance.hpp>
#include <lidl/types.hpp>
#include <stdexcept>

namespace lidl {
struct enum_member : public cbase<base::categories::enum_member> {
    enum_member(enumeration& en, int val, std::optional<source_info> src_info = {});
    int value;
};

struct enumeration
    : value_type
    , public extendable<enumeration> {
public:
    using value_type::value_type;

    name underlying_type;
    std::deque<std::pair<std::string, enum_member>> members;

    void add_member(std::string val, std::optional<source_info> src_info = {}) {
        if (find_by_name(val) >= 0) {
            // Member already exists
            throw std::runtime_error("Duplicate member in enumeration!");
        }
        members.emplace_back(std::move(val),
                             enum_member{*this,
                                         static_cast<int>(all_members().size()),
                                         std::move(src_info)});
        define(get_scope(), members.back().first, &members.back().second);
    }

    [[nodiscard]] raw_layout wire_layout(const module& mod) const override {
        return get_type<wire_type>(mod, underlying_type)->wire_layout(mod);
    }

    [[nodiscard]] std::vector<std::pair<std::string_view, const enum_member*>>
    all_members() const {
        std::vector<std::pair<std::string_view, const enum_member*>> res;
        for (auto& s : inheritance_list()) {
            for (auto& [name, proc] : s->members) {
                res.emplace_back(name, &proc);
            }
        }
        return res;
    }

    [[nodiscard]] std::deque<std::pair<std::string, enum_member>>::const_iterator
    find_by_value(int val) const {
        auto it = std::find_if(members.begin(), members.end(), [val](auto& member) {
            return member.second.value == val;
        });

        if (it == members.end()) {
            if (auto base = get_base()) {
                return base->find_by_value(val);
            }
        }

        return it;
    }

    [[nodiscard]] int find_by_name(std::string_view name) const {
        auto mems = all_members();
        auto it   = std::find_if(mems.begin(), mems.end(), [name](auto& member) {
            return member.first == name;
        });
        if (it == mems.end()) {
            return -1;
        }
        return it->second->value;
    }

    YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override;

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        auto val        = node.as<int64_t>();
        auto underlying = get_type<wire_type>(mod, underlying_type);
        writer.align(underlying->wire_layout(mod).alignment());
        auto pos = writer.tell();
        underlying->yaml2bin(mod, node, writer);
        return pos;
    }

private:
};
} // namespace lidl