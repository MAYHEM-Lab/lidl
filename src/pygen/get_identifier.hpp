#pragma once

#include <lidl/basic.hpp>
#include <string>

namespace lidl::py {
std::string get_identifier(const module& mod, const name& n);
}