//
// Created by fatih on 1/24/20.
//

#pragma once

#include <lidl/meta.hpp>
#include <lidl/status.hpp>
#include <string_view>
#include <tuple>


namespace lidl {
template<class T>
class service_descriptor;

template<auto Fn, class ParamsT>
struct procedure_descriptor {
    std::string_view name;
};

template<class...>
struct procedure_traits;

template<class Type, class RetType, class... ArgTypes>
struct procedure_traits<lidl::status<RetType> (Type::*const)(ArgTypes...)> {
    using return_type = RetType;
    using param_types = meta::list<ArgTypes...>;

    static constexpr bool takes_response_builder() {
        return std::is_same_v<return_type, void>;
    }
};
} // namespace lidl