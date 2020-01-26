//
// Created by fatih on 1/24/20.
//

#pragma once

#include <deque>
#include <lidl/attributes.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct procedure {
    std::deque<const type*> return_types;
    std::deque<std::pair<std::string, const type*>> parameters;
};

struct service {
    std::deque<std::pair<std::string, procedure>> procedures;
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