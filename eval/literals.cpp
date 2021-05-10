#include <lidl/eval/literals.hpp>

namespace lidl::eval {
evaluate_result name_literal_expression::evaluate(const module& mod) const noexcept {
    auto sym_base = get_symbol(value_name.base);

    if (sym_base->category() == base::categories::enum_member) {
        return value(value::enum_val_t{value_name});
    }

    return common_errors{};
}
} // namespace lidl::eval