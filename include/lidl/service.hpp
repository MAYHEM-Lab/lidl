//
// Created by fatih on 1/24/20.
//

#pragma once

#include <deque>
#include <lidl/types.hpp>
#include <lidl/union.hpp>

namespace lidl {
struct procedure {
    std::deque<name> return_types;
    std::deque<std::pair<std::string, name>> parameters;
    const structure* params_struct;
    const structure* results_struct;

    std::optional<source_info> source_information;
};

struct property : member {
    using member::member;
};

struct service {
    std::vector<name> extends;
    std::deque<std::pair<std::string, property>> properties;
    std::deque<std::pair<std::string, procedure>> procedures;
    const union_type* procedure_params_union;
    const union_type* procedure_results_union;
};
} // namespace lidl