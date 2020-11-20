#pragma once

#include <lidl/module.hpp>
#include <lidl/scope.hpp>
#include <string>
#include "generator.hpp"

namespace lidl::cpp {
using codegen::section;
using codegen::section_key_t;
using codegen::section_type;
std::string get_identifier(const module& mod, const name& n);
std::string get_local_identifier(const module& mod, const name& n);
std::string get_user_identifier(const module& mod, const name& n);
} // namespace lidl::cpp