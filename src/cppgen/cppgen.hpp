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

/**
 * After calling this function for a symbol, all future identifier lookups will use the
 * given name instead of its definition.
 */
void rename(const module& mod, const symbol_handle& sym, std::string rename_to);
} // namespace lidl::cpp