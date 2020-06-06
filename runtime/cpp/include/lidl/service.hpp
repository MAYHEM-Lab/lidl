//
// Created by fatih on 1/24/20.
//

#pragma once

#include <lidl/meta.hpp>
#include <lidl/status.hpp>
#include <string_view>
#include <tuple>
#include <lidl/builder.hpp>

namespace lidl {
template<class T>
class service_descriptor;

template<auto Fn, class ParamsT, class ResultsT>
struct procedure_descriptor {
    std::string_view name;
};

template<class...>
struct procedure_traits;

template<class Type, class RetType, class... ArgTypes>
struct procedure_traits<RetType (Type::*const)(ArgTypes...)> {
    using return_type = RetType;
    using param_types = meta::list<ArgTypes...>;

    static constexpr bool takes_response_builder() {
        return (... || std::is_same_v<std::remove_reference_t<ArgTypes>, message_builder>);
    }
};

template<class Type, class RetType, class... ArgTypes>
struct procedure_traits<RetType (Type::*)(ArgTypes...)> : procedure_traits<RetType (Type::*const)(ArgTypes...)> {
};

template <class T>
class rpc_param_traits;

class service_base {
public:
    virtual ~service_base() = default;
};

template <class ServiceT>
class service : service_base {
};
} // namespace lidl