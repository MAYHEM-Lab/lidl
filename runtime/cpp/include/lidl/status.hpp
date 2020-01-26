#pragma once

#include <optional>

namespace lidl {
template <class T>
using status = std::optional<T>;
}