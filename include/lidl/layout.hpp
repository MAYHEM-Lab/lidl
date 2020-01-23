#pragma once

#include <algorithm>
#include <cstdint>

namespace lidl {
struct raw_layout {
public:
    raw_layout(int16_t sz, int16_t align)
        : m_size{sz}
        , m_alignment{align} {
        if (m_alignment != 0) {
            while (m_size % m_alignment) {
                m_size++;
            }
        }
    }

    int16_t size() const {
        return m_size;
    }

    int16_t alignment() const {
        return m_alignment;
    }

    friend bool operator==(const raw_layout& left, const raw_layout& right) {
        return left.size() == right.size() && left.alignment() && right.alignment();
    }

private:
    int16_t m_size;
    int16_t m_alignment;
};

struct aggregate_layout_computer {
public:
    void add(const raw_layout& layout) {
        auto bigger_alignment = std::max(m_current.alignment(), layout.alignment());
        int padding = 0;
        while ((m_current.size() + padding) % layout.alignment()) {
            padding++;
        }
        m_current = raw_layout(
            m_current.size() + layout.size() + padding, 
            bigger_alignment);
    }

    raw_layout get() const {
        return m_current;
    }

private:
    raw_layout m_current{0, 0};
};
} // namespace lidl