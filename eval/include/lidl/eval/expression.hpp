#pragma once

#include <lidl/eval/value.hpp>
#include <lidl/module.hpp>
#include <memory>

namespace lidl::eval {
struct common_errors {
    std::optional<source_info> src_info;
};

using evaluate_result = std::variant<value, common_errors>;

struct expression {
    virtual evaluate_result evaluate(const module& mod) const noexcept = 0;

    std::optional<source_info> src_info;

    virtual ~expression() = default;
};
} // namespace lidl::eval