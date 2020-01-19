#pragma once

#include <variant>

namespace lidl {
struct type;
struct generic_type;

using symbol = std::variant<std::monostate, const type*, const generic_type*>;
} // namespace lidl
