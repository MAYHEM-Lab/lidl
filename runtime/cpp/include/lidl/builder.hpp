#pragma once

#include "ptr.hpp"
#include "string.hpp"

#include <algorithm>

namespace lidl {
struct message_builder {
public:
    message_builder(uint8_t* ptr, uint16_t size)
        : base(ptr)
        , cur_ptr(ptr)
        , current(0) {
    }

    void increment(uint16_t diff) {
        current += diff;
        cur_ptr += diff;
    }

    uint16_t size() const {
        return current;
    }

    uint8_t* base;
    uint8_t* cur_ptr;
    uint16_t current;
};

template<class T>
T& append_raw(message_builder& builder, const T& t) {
    auto off = builder.current;
    auto ptr = new (builder.cur_ptr) T(t);
    builder.increment(sizeof *ptr);
    return *ptr;
}

template<class T, class... Ts>
T& emplace_raw(message_builder& builder, Ts&&... args) {
    auto off = builder.current;
    auto ptr = new (builder.cur_ptr) T{std::forward<Ts>(args)...};
    builder.increment(sizeof *ptr);
    return *ptr;
}

string& create_string(message_builder& builder, std::string_view sv) {
    auto off = builder.current;
    std::copy(sv.begin(), sv.end(), reinterpret_cast<char*>(builder.cur_ptr));
    builder.increment(sv.size());
    return emplace_raw<string>(builder, ptr<char>(builder.current - off), uint16_t(sv.size()));
}
} // namespace lidl