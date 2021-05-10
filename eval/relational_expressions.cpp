#include <lidl/eval/relational_expressions.hpp>

namespace lidl::eval {
evaluate_result binary_expression::evaluate(const module& mod) const noexcept {
    auto left_val  = std::get<value>(left->evaluate(mod));
    auto right_val = std::get<value>(right->evaluate(mod));

    switch (op) {
    case binary_operators::eqeq:
        return value(left_val == right_val);
    }
}
} // namespace lidl::eval
