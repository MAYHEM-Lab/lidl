#include <lidl/eval/value.hpp>
namespace lidl::eval {
bool operator==(const value& a, const value& b) {
    return std::visit(
        [](const auto& l, const auto& r) {
            if constexpr (!std::is_same_v<decltype(l), decltype(r)>) {
                return false;
            } else {
                if constexpr (std::is_same_v<const value::fun_t&, decltype(r)>) {
                    return false;
                } else if constexpr (std::is_same_v<const value::enum_val_t&,
                                                    decltype(r)>) {
                    return l.enum_value == r.enum_value;
                } else {
                    return l == r;
                }
            }
        },
        a.m_data,
        b.m_data);
}
} // namespace lidl::eval
