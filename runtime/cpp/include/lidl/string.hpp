#pragma once

#include "ptr.hpp"

#include <string_view>
#include <lidl/buffer.hpp>

namespace lidl {
class string {
public:
    string(ptr<char> begin, uint16_t size)
        : m_ptr(begin)
        , m_size(size) {
    }

    string(const string&) = delete;
    string(string&&) = delete;

    std::string_view unsafe_string_view() const {
        return {&m_ptr.unsafe_get(), m_size};
    }

    std::string_view string_view(const buffer& buf) const {
        return {&buf[m_ptr], m_size};
    }

private:
    ptr<char> m_ptr;
    uint16_t m_size;
};

static_assert(sizeof(string) == 4);
static_assert(alignof(string) == 2);
} // namespace lidl