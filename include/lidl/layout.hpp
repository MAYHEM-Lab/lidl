#pragma once

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <numeric>
#include <optional>
#include <unordered_map>
#include <ostream>

namespace lidl {
struct raw_layout {
public:
    raw_layout(size_t sz, size_t align)
        : m_size{sz}
        , m_alignment{align} {
        if ((m_size % m_alignment) != 0) {
            throw std::runtime_error("Bad alignment!");
        }

        if (m_alignment != 0) {
            while (m_size % m_alignment) {
                m_size++;
                m_padding++;
            }
        }
    }

    [[nodiscard]] int16_t size() const {
        return m_size;
    }

    [[nodiscard]] int16_t alignment() const {
        return m_alignment;
    }

    [[nodiscard]] int16_t padding() const {
        return m_padding;
    }

    friend bool operator==(const raw_layout& left, const raw_layout& right) {
        return left.size() == right.size() &&
               left.alignment() ==
                   right.alignment(); // left.padding() == right.padding();
    }

private:
    int16_t m_padding = 0;
    size_t m_size;
    size_t m_alignment;
};

inline std::ostream& operator<<(std::ostream& os, const raw_layout& layout) {
    return os << fmt::format(
               "{{ size: {}, alignment: {} }}", layout.size(), layout.alignment());
}

struct union_layout_computer {
public:
    union_layout_computer& add(const raw_layout& layout) {
        m_current = raw_layout(
            std::max(m_current.size(), layout.size()),
            std::lcm(std::max<int>(1, m_current.alignment()), layout.alignment()));
        return *this;
    }

    [[nodiscard]] raw_layout get() const {
        return m_current;
    }

private:
    raw_layout m_current{0, 1};
};

class compound_layout {
public:
    compound_layout& add_member(std::string_view member_name,
                                const raw_layout& member_layout);

    [[nodiscard]] std::optional<size_t> offset_of(std::string_view name) const;

    const raw_layout& get() const {
        return m_current;
    }

private:
    raw_layout m_current{1, 1};
    size_t m_padding = 1;
    std::unordered_map<std::string, size_t> m_offsets;
};
} // namespace lidl