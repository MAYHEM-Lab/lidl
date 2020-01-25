#pragma once

#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <numeric>

namespace lidl {
struct raw_layout {
public:
    raw_layout(int16_t sz, int16_t align)
        : m_size{sz}
        , m_alignment{align} {
        if (m_alignment != 0) {
            while (m_size % m_alignment) {
                m_size++;
                m_padding++;
            }
        }
    }

    int16_t size() const {
        return m_size;
    }

    int16_t alignment() const {
        return m_alignment;
    }

    int16_t padding() const {
        return m_padding;
    }

    friend bool operator==(const raw_layout& left, const raw_layout& right) {
        return left.size() == right.size() && left.alignment() == right.alignment();//left.padding() == right.padding();
    }

private:
    int16_t m_padding = 0;
    int16_t m_size;
    int16_t m_alignment;
};

inline std::ostream& operator<<(std::ostream& os, const raw_layout& layout) {
    return os << fmt::format(
               "{{ size: {}, alignment: {} }}", layout.size(), layout.alignment());
}

struct aggregate_layout_computer {
public:
    void add(const raw_layout& layout) {
        int padding = 0;
        while ((m_current.size() - m_current.padding() + padding) % layout.alignment()) {
            padding++;
        }
        m_current = raw_layout(
            m_current.size() + layout.size() - m_current.padding() + padding,
            std::lcm(std::max<int>(1, m_current.alignment()), layout.alignment()));
    }

    raw_layout get() const {
        return m_current;
    }

private:
    raw_layout m_current{0, 0};
};
} // namespace lidl