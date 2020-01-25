#pragma once

#include <algorithm>
#include <lidl/buffer.hpp>
#include <lidl/ptr.hpp>

namespace lidl {
struct message_builder {
public:
    message_builder(tos::span<uint8_t> buf)
        : m_buffer(buf)
        , m_cur_ptr(m_buffer.data()) {
    }

    uint16_t size() const {
        return m_cur_ptr - m_buffer.data();
    }

    buffer get_buffer() const {
        return buffer{m_buffer};
    }

    uint8_t* allocate(size_t size, size_t align) {
        auto ptr = m_cur_ptr;
        m_cur_ptr += size;
        return ptr;
    }
private:
    tos::span<uint8_t> m_buffer;
    uint8_t* m_cur_ptr;
};

template<class T>
T& append_raw(message_builder& builder, const T& t) {
    auto alloc = builder.allocate(sizeof(T), alignof(T));
    auto ptr = new (alloc) T(t);
    return *ptr;
}

template<class T, class... Ts>
T& emplace_raw(message_builder& builder, Ts&&... args) {
    auto alloc = builder.allocate(sizeof(T), alignof(T));
    auto ptr = new (alloc) T{std::forward<Ts>(args)...};
    return *ptr;
}

template <class T, class... Ts>
T& create(message_builder& builder, Ts&&... args) {
    return emplace_raw<T>(builder, std::forward<Ts>(args)...);
}
} // namespace lidl