#pragma once

#include <lidl/basic.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct module {
    type_db symbols;
    std::map<std::string, structure> structs;
};
}