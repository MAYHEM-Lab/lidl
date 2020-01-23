#pragma once

#include "ptr.hpp"
#include "string.hpp"

#include <algorithm>
#include <lidl/buffer.hpp>

namespace lidl {
struct message_builder {
public:
    message_builder(tos::span<uint8_t> buf)
        : m_buffer(buf)
        , cur_ptr(m_buffer.data()) {
    }

    void increment(uint16_t diff) {
        cur_ptr += diff;
    }

    uint16_t size() const {
        return cur_ptr - m_buffer.data();
    }

    buffer get_buffer() const {
        return buffer{m_buffer};
    }

    tos::span<uint8_t> m_buffer;
    uint8_t* cur_ptr;
};

template<class T>
T& append_raw(message_builder& builder, const T& t) {
    auto ptr = new (builder.cur_ptr) T(t);
    builder.increment(sizeof *ptr);
    return *ptr;
}

template<class T, class... Ts>
T& emplace_raw(message_builder& builder, Ts&&... args) {
    auto ptr = new (builder.cur_ptr) T{std::forward<Ts>(args)...};
    builder.increment(sizeof *ptr);
    return *ptr;
}

string& create_string(message_builder& builder, std::string_view sv) {
    auto off = builder.cur_ptr;
    std::copy(sv.begin(), sv.end(), reinterpret_cast<char*>(builder.cur_ptr));
    builder.increment(sv.size());
    return emplace_raw<string>(builder, ptr<char>(builder.cur_ptr - off), uint16_t(sv.size()));
}
} // namespace lidl