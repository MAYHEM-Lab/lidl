#pragma once

#include <lidl/identifier.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <vector>

namespace lidl {
struct module {
    type_db symbols;
    generics_table generics;
    std::vector<std::pair<identifier, structure>> structs;
};
} // namespace lidl