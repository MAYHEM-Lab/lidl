#pragma once

#include <algorithm>
#include <deque>
#include <lidl/basic.hpp>
#include <lidl/member.hpp>
#include <lidl/types.hpp>
#include <lidl/service.hpp>

namespace lidl {
struct structure : public type {
    std::deque<std::pair<std::string, member>> members;

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

    bool is_reference_type(const module& mod) const override;

    raw_layout wire_layout(const module& mod) const override;

    YAML::Node bin2yaml(const module& mod,
                                           ibinary_reader& span) const override;

    int yaml2bin(const module& mod,
                  const YAML::Node& node,
                  ibinary_writer& writer) const override;

    compound_layout layout(const module& mod) const;
};
} // namespace lidl