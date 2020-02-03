#pragma once

#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <string>

namespace lidl::cpp {
std::string get_identifier(const module& mod, const symbol_handle& handle);
std::string get_identifier(const module& mod, const name& n);
} // namespace lidl::cpp