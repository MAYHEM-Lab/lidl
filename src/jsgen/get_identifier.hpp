#pragma once

#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <string>

namespace lidl::js {
std::string get_js_identifier(const module& mod, const name& n);
std::string get_local_js_identifier(const module& mod, const name& n);
std::string get_identifier(const module& mod, const name& n);
std::string get_local_identifier(const module& mod, const name& n);
std::string get_user_identifier(const module& mod, const name& n);
} // namespace lidl::js