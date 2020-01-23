#pragma once

#include <cstdint>

namespace lidl {
template<class T>
class ptr {
public:
    explicit ptr(uint16_t offset)
        : m_offset(offset) {
    }

    const T& unsafe_get() const {
        auto self = reinterpret_cast<const uint8_t*>(this);
        auto ptr = self - m_offset;
        return *reinterpret_cast<const T*>(ptr);
    }

    T& unsafe_get() {
        auto self = reinterpret_cast<uint8_t*>(this);
        auto ptr = self - m_offset;
        return *reinterpret_cast<T*>(ptr);
    }

    uint16_t get_offset() const {
        return m_offset;
    }

private:
    uint16_t m_offset;
};

static_assert(sizeof(ptr<int>) == 2);
static_assert(alignof(ptr<int>) == 2);
} // namespace lidl