#pragma once

#include <lidl/generics.hpp>
#include <lidl/identifier.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <lidl/symbol_table.hpp>
#include <lidl/service.hpp>
#include <variant>
#include <vector>

namespace lidl {
struct module {
    symbol_table symbols;

    mutable std::unordered_map<symbol, std::vector<generic_argument>> generated;

    std::deque<structure> structs;
    std::deque<std::pair<std::string, generic_structure>> generic_structs;
    std::deque<std::pair<std::string, service>> services;
};
} // namespace lidl