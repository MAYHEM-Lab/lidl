#pragma once

#include <cstdint>

namespace lidl {
template<class T>
class ptr {
public:
    explicit ptr(uint16_t offset)
        : m_offset(offset) {
    }

    const T& get(const uint8_t* base) const {
        auto ptr = base + m_offset;
        return *reinterpret_cast<const T*>(ptr);
    }

    T& get(uint8_t* base) {
        auto ptr = base + m_offset;
        return *reinterpret_cast<T*>(ptr);
    }

private:
    uint16_t m_offset;
};
} // namespace lidl