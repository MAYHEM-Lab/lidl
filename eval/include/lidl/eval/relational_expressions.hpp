#pragma once

#include <lidl/eval/expression.hpp>
#include <lidl/eval/value.hpp>
#include <lidl/module.hpp>
#include <memory>

namespace lidl::eval {
enum class binary_operators
{
    eqeq
};

struct binary_expression final : expression {
    std::unique_ptr<expression> left, right;
    binary_operators op;

    evaluate_result evaluate(const module& mod) const noexcept override;
};
} // namespace lidl::eval