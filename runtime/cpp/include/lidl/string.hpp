#pragma once

#include "ptr.hpp"

#include <lidl/buffer.hpp>
#include <lidl/builder.hpp>
#include <string_view>

namespace lidl {
class string {
public:
    string(message_builder&, uint8_t* data)
        : m_ptr(data) {
    }

    string(const string&) = delete;
    string(string&&) = delete;

    [[nodiscard]]
    std::string_view unsafe_string_view() const {
        return {&m_ptr.unsafe().get(), static_cast<size_t>(m_ptr.get_offset())};
    }

    [[nodiscard]]
    std::string_view string_view(const buffer& buf) const {
        return {&buf[m_ptr], static_cast<size_t>(m_ptr.get_offset())};
    }

private:
    ptr<char> m_ptr{nullptr};
};

static_assert(sizeof(string) == 2);
static_assert(alignof(string) == 2);

inline string& create_string(message_builder& builder, int len) {
    auto alloc = builder.allocate(len, 1);
    return emplace_raw<string>(builder, builder, alloc);
}

inline string& create_string(message_builder& builder, std::string_view sv) {
    auto alloc = builder.allocate(sv.size(), 1);
    std::copy(sv.begin(), sv.end(), reinterpret_cast<char*>(alloc));
    return emplace_raw<string>(builder, builder, alloc);
}
} // namespace lidl