#pragma once

#include <algorithm>
#include <deque>
#include <lidl/basic.hpp>
#include <lidl/member.hpp>
#include <lidl/types.hpp>
#include <lidl/service.hpp>

namespace lidl {
struct structure : public value_type {
    std::deque<std::tuple<std::string, member>> members;

    std::optional<procedure_params_info> params_info;
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