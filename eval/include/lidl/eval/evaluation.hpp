#pragma once

#include <lidl/eval/expression.hpp>
#include <lidl/module.hpp>

namespace lidl::eval {
struct any_value {};

any_value evaluate_expression(const module& mod, const expression& expr);
} // namespace lidl::eval