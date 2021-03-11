#pragma once

#include "ast.hpp"

namespace lidl::frontend {
std::optional<ast::module> parse_module(std::string_view input);
}