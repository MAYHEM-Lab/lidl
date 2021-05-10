#pragma once

#include <cstdint>
#include <lidl/eval/detail/expression_common.hpp>
#include <lidl/eval/expression.hpp>
#include <lidl/module.hpp>
#include <string>
#include <variant>

namespace lidl::eval {
struct integral_literal_expression final : expression {
    uint64_t value;
    evaluate_result evaluate(const module& mod) const noexcept override {
        return lidl::eval::value(this->value);
    }
};

struct floating_literal_expression final : expression {
    double value;
    evaluate_result evaluate(const module& mod) const noexcept override {
        return lidl::eval::value(this->value);
    }
};

struct string_literal_expression final : expression {
    std::string value;
    evaluate_result evaluate(const module& mod) const noexcept override {
        return lidl::eval::value(this->value);
    }
};

struct boolean_literal_expression final : expression {
    bool value;
    evaluate_result evaluate(const module& mod) const noexcept override {
        return lidl::eval::value(this->value);
    }
};

struct name_literal_expression final : expression {
    name value_name;

    evaluate_result evaluate(const module& mod) const noexcept override;
};
} // namespace lidl::eval