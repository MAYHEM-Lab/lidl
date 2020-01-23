#pragma once

#include <tos/span.hpp>
#include <lidl/ptr.hpp>

namespace lidl {
struct buffer {
public:
    explicit buffer(tos::span<uint8_t> buf)
        : m_buffer{buf} {
    }

    template <class T>
    T& operator[](ptr<T>& p) {
        auto off = p.get_offset();
        auto ptr_base = reinterpret_cast<uint8_t*>(&p);
        if (ptr_base - off < m_buffer.begin()) {
            // bad access!
        }
        return p.unsafe_get();
    }

    template<class T>
    const T& operator[](const ptr<T>& p) const {
        auto off = p.get_offset();
        auto ptr_base = reinterpret_cast<const uint8_t*>(&p);
        if (ptr_base - off < m_buffer.begin()) {
            // bad access!
        }
        return p.unsafe_get();
    }

    template<class T>
    const T& operator[](const ptr<const T>& p) const {
        auto off = p.get_offset();
        auto ptr_base = reinterpret_cast<const uint8_t*>(&p);
        if (ptr_base - off < m_buffer.begin()) {
            // bad access!
        }
        return p.unsafe_get();
    }

private:
    tos::span<uint8_t> m_buffer;
};
} // namespace lidl