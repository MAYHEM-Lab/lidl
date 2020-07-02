#pragma once

#include <array>
#include <cstdint>

namespace lidl {
template<class T, std::size_t N>
using array = std::array<T, N>;
}