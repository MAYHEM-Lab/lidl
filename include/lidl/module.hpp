#pragma once

#include <lidl/generics.hpp>
#include <lidl/scope.hpp>
#include <lidl/service.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <variant>
#include <vector>


namespace lidl {
struct module {
    std::shared_ptr<scope> symbols = std::make_shared<scope>();

    std::deque<std::unique_ptr<type>> basic_types;
    std::deque<std::unique_ptr<generic>> basic_generics;

    std::deque<structure> structs;
    std::deque<generic_structure> generic_structs;
    std::deque<std::pair<std::string, service>> services;
};

void add_basic_types(module&);
} // namespace lidl