//
// Created by fatih on 1/24/20.
//

#pragma once

#include <lidl/status.hpp>
#include <tuple>

namespace lidl {
template <class T>
class service_descriptor;

template <auto Fn>
struct procedure_params_t;

template <auto Fn>
struct procedure_descriptor {
    std::string_view name;
};
}