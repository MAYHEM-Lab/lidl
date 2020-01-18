#pragma once

#include "ptr.hpp"

#include <string_view>

namespace lidl {
class string {
public:
    string(ptr<char> begin, uint16_t size)
        : m_ptr(begin)
        , m_size(size) {
    }

    std::string_view string_view(const uint8_t* base) const {
        return {&m_ptr.get(base), m_size};
    }

private:
    ptr<char> m_ptr;
    uint16_t m_size;
};
} // namespace lidl