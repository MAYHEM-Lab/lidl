//
// Created by fatih on 1/24/20.
//

#pragma once

#include <deque>
#include <lidl/types.hpp>
#include <lidl/union.hpp>

namespace lidl {
enum class param_flags {
    in = 1,
    out = 2,
    in_out = 3
};

struct parameter {
    lidl::name type;
    param_flags flags = param_flags::in;
    std::optional<source_info> src_info;
};

struct procedure {
    std::deque<name> return_types;
    std::deque<std::pair<std::string, parameter>> parameters;
    const structure* params_struct;
    const structure* results_struct;

    std::optional<source_info> src_info;
};

struct property : member {
    using member::member;
};

struct service {
    std::vector<name> extends;
    std::deque<std::pair<std::string, property>> properties;
    std::deque<std::pair<std::string, procedure>> procedures;
    const union_type* procedure_params_union = nullptr;
    const union_type* procedure_results_union = nullptr;

    std::optional<source_info> src_info;
};
} // namespace lidl