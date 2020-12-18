//
// Created by fatih on 2/16/20.
//

#pragma once

#include <algorithm>
#include <deque>
#include <lidl/basic.hpp>
#include <lidl/member.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct union_type : public type {
    std::deque<std::pair<std::string, member>> members;

    /**
     * If a union is raw, the alternatives type and members are not generated and the
     * union is completely unsafe to use.
     */
    bool raw = false;

    /** If this member is not null, this union stores the parameter structs of that
     * service.
     */
    const service* call_for = nullptr;

    /**
     * If this member is not null, this union stores the return structs of that service.
     */
    const service* return_for = nullptr;

    const enumeration& get_enum(const module& m) const;

    union_type()                  = default;
    union_type(const union_type&) = default;
    union_type(union_type&&)      = default;
    union_type& operator=(union_type&&) = default;

    type_categories category(const module& mod) const override;

    virtual raw_layout wire_layout(const module& mod) const override;

    YAML::Node bin2yaml(const module& mod, ibinary_reader& reader) const override;

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    compound_layout layout(const module& mod) const;

private:
    mutable std::unique_ptr<const enumeration> m_enumeration;
};

enumeration enum_for_union(const module& m, const union_type& u);
} // namespace lidl
