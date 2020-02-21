#pragma once

namespace lidl {
template<class T>
struct struct_base {};

template<auto Mem>
struct member_info {
    const char* name;
};

template<class T>
struct struct_traits;
} // namespace lidl

namespace std {
template<size_t I, class T>
auto& get(const lidl::struct_base<T>&) {
}
} // namespace std