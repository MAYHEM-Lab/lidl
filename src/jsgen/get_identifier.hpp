#pragma once

#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <string>

namespace lidl::js {
std::string get_local_type_name(const module& mod, const name& n);
std::string get_local_obj_name(const module& mod, const name& n);
std::string get_local_user_obj_name(const module& mod, const name& n);
} // namespace lidl::js