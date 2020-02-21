#pragma once

#include <tuple>
#include <type_traits>

namespace lidl {
template<class T>
struct struct_base {};

template<class T, class ConstT>
struct member_info;

template<class T, class RetT, class ConstRetT>
struct member_info<RetT (T::*)(), ConstRetT (T::*)() const> {
    constexpr member_info(const char* name, RetT (T::*fn)(), ConstRetT (T::*cfn)() const)
        : name{name}
        , function{fn}
        , const_function{cfn} {
    }
    const char* name;
    RetT (T::*function)();
    ConstRetT (T::*const_function)() const;
};

template<class T, class RetT, class ConstRetT>
member_info(const char* name, RetT (T::*fn)(), ConstRetT (T::*cfn)() const)
    ->member_info<RetT (T::*)(), ConstRetT (T::*)() const>;

template<class T>
struct struct_traits;
} // namespace lidl

namespace std {
template<size_t I,
         class T,
         std::enable_if_t<(lidl::struct_traits<T>::arity > I)>* = nullptr>
auto& get(const lidl::struct_base<T>& str) {
    auto& elem = static_cast<T&>(str);
    return str.*(std::get<I>(lidl::struct_traits<T>::members).const_function);
}
} // namespace std