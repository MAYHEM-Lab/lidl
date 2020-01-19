#pragma once

#include <lidl/generics.hpp>
#include <lidl/identifier.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <lidl/symbol_table.hpp>
#include <variant>
#include <vector>

namespace lidl {
struct module {
    symbol_table syms;

    mutable std::unordered_map<symbol, std::vector<generic_argument>> generated;

    std::vector<std::pair<std::string, structure>> structs;
    std::vector<std::pair<std::string, generic_structure>> generic_structs;
};
} // namespace lidl