#pragma once

namespace lidl {
template <class T>
class vector {
public:
    explicit vector(message_builder&, T* base) : m_ptr{base} {}

    [[nodiscard]]
    tos::span<T> span() {
        auto base = &m_ptr.unsafe().get();
        return tos::span<T>(base, size());
    }
    
private:
    [[nodiscard]]
    size_t size() const {
        return m_ptr.get_offset() / sizeof(T);
    }

    ptr<T> m_ptr{nullptr};
};

template <class T>
vector<T>& create_vector(message_builder& builder, int size) {
    auto alloc = builder.allocate(size * sizeof(T), alignof(T));
    auto& res = emplace_raw<vector<T>>(builder, builder, reinterpret_cast<T*>(alloc));
    std::uninitialized_default_construct_n(res.span().begin(), size);
    return res;
}
}