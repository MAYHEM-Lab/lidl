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

    string(const string&) = delete;
    string(string&&) = delete;

    std::string_view string_view() const {
        return {&m_ptr.get(), m_size};
    }

private:
    ptr<char> m_ptr;
    uint16_t m_size;
};

static_assert(sizeof(string) == 4);
static_assert(alignof(string) == 2);
} // namespace lidl