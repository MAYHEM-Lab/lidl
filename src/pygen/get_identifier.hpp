#pragma once

#include <lidl/basic.hpp>
#include <string>

namespace lidl::py {
std::string get_identifier(const module& mod, const name& n);
std::string get_local_identifier(const module& mod, const name& from, const name& n);
std::string import_string(const module& mod, const base* sym);
}