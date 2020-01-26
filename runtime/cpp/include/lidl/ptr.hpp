#pragma once

#include <cstdint>

namespace lidl {
template<class T>
class ptr {
    struct unsafe_;

public:
    explicit ptr(std::nullptr_t)
        : m_unsafe{0} {
    }

    explicit ptr(uint8_t* to) : ptr(reinterpret_cast<uint8_t*>(this) - to) {}
    template <class U = T, std::enable_if_t<!std::is_same_v<U, uint8_t*>>* =nullptr>
    explicit ptr(const T& to) : ptr(reinterpret_cast<const uint8_t*>(this) - reinterpret_cast<const uint8_t*>(&to)) {}
    template <class U = T, std::enable_if_t<!std::is_same_v<U, uint8_t>>* =nullptr>
    explicit ptr(const T* to) : ptr(reinterpret_cast<const uint8_t*>(this) - reinterpret_cast<const uint8_t*>(to)) {}

    explicit ptr(uint16_t offset)
        : m_unsafe{offset} {
    }

    ptr(const ptr&) = delete;
    ptr(ptr&&) = delete;

    [[nodiscard]]
    uint16_t get_offset() const {
        return m_unsafe.m_offset;
    }

    [[nodiscard]]
    unsafe_& unsafe() {
        return m_unsafe;
    }

    [[nodiscard]]
    const unsafe_& unsafe() const {
        return m_unsafe;
    }

    [[nodiscard]]
    operator bool() const {
        return get_offset() != 0;
    }

private:
    struct unsafe_ {
        uint16_t m_offset;

        const T& get() const {
            auto self = reinterpret_cast<const uint8_t*>(this);
            auto ptr = self - m_offset;
            return *reinterpret_cast<const T*>(ptr);
        }

        T& get() {
            auto self = reinterpret_cast<uint8_t*>(this);
            auto ptr = self - m_offset;
            return *reinterpret_cast<T*>(ptr);
        }

        T* operator->() {
            return &get();
        }
        const T* operator->() const {
            return &get();
        }
    } m_unsafe;
};

static_assert(sizeof(ptr<int>) == 2);
static_assert(alignof(ptr<int>) == 2);
} // namespace lidl