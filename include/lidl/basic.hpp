#pragma once

#include <variant>

namespace lidl {
struct type;
struct generic;

using symbol = std::variant<std::monostate, const type*, const generic*>;
} // namespace lidl
