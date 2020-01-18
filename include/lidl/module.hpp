#pragma once

#include <lidl/basic.hpp>
#include <lidl/identifier.hpp>
#include <lidl/types.hpp>
#include <map>

namespace lidl {
struct module {
    type_db symbols;
    generics_table generics;
    std::map<identifier, structure> structs;
};
} // namespace lidl