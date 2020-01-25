//
// Created by fatih on 1/24/20.
//

#pragma once

#include <deque>
#include <lidl/types.hpp>

namespace lidl {
struct procedure {
    std::deque<const type*> return_types;
    std::deque<std::pair<std::string, const type*>> parameters;
};

struct service {
    std::deque<std::pair<std::string, procedure>> procedures;
};
}