//
// Created by fatih on 1/24/20.
//

#pragma once

#include <deque>
#include <lidl/attributes.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <lidl/union.hpp>

namespace lidl {
struct procedure {
    std::deque<name> return_types;
    std::deque<std::pair<std::string, name>> parameters;
    const structure* params_struct;
    const structure* results_struct;
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

class procedure_params_attribute : public attribute {
public:
    explicit procedure_params_attribute(std::string serv_name,
                                        std::string name,
                                        const procedure& p)
        : attribute("procedure_params")
        , serv_name(std::move(serv_name))
        , proc_name(std::move(name))
        , proc(&p) {
    }
    std::string serv_name;
    std::string proc_name;
    const procedure* proc;
};
} // namespace lidl