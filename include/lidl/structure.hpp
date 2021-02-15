#pragma once

#include <algorithm>
#include <deque>
#include <lidl/basic.hpp>
#include <lidl/member.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct structure : public type {
    using type::type;

    const std::deque<std::pair<std::string, member>>& all_members() const {
        return members;
    }

    std::deque<std::pair<std::string, member>>& own_members() {
        return members;
    }

    const std::deque<std::pair<std::string, member>>& own_members() const {
        return members;
    }

    void add_member(std::string name, member mem) {
        members.emplace_back(std::move(name), std::move(mem));
        define(get_scope(), members.back().first, &members.back().second);
    }

    /**
     * If this member has a value, this structure is generated to transport the parameters
     * for that procedure.
     */
    std::optional<procedure_params_info> params_info;

    /**
     * If this member has a value, this structure is generated to transport the return
     * values of that procedure.
     */
    std::optional<procedure_params_info> return_info;

    type_categories category(const module& mod) const override;

    raw_layout wire_layout(const module& mod) const override;

    YAML::Node bin2yaml(const module& mod,
                                           ibinary_reader& span) const override;

    int yaml2bin(const module& mod,
                  const YAML::Node& node,
                  ibinary_writer& writer) const override;

    compound_layout layout(const module& mod) const;

private:
    std::deque<std::pair<std::string, member>> members;
};
} // namespace lidl