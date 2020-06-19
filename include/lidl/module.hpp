#pragma once


#include <lidl/enumeration.hpp>
#include <lidl/generics.hpp>
#include <lidl/scope.hpp>
#include <lidl/service.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <lidl/union.hpp>
#include <variant>
#include <vector>

namespace lidl {
struct module {
    std::string module_name;
    std::string name_space = "lidlmod";

    const module* parent           = nullptr;
    std::shared_ptr<scope> symbols = std::make_shared<scope>();

    std::deque<std::unique_ptr<type>> basic_types;
    std::deque<std::unique_ptr<generic>> basic_generics;

    std::deque<structure> structs;
    std::deque<union_type> unions;
    std::deque<enumeration> enums;

    std::deque<generic_structure> generic_structs;
    std::deque<generic_union> generic_unions;

    std::deque<service> services;
    mutable std::deque<generic_instantiation> instantiations;

    const generic_instantiation& create_or_get_instantiation(const name& ins) const;

    module& get_child(std::string_view child_name);

    module& add_child(std::string_view child_name, std::unique_ptr<module> child);

    mutable std::deque<std::pair<std::string, std::unique_ptr<module>>> children;
    mutable std::vector<std::pair<name, generic_instantiation*>> name_ins;
    module() = default;

private:
    friend const module& get_root_module();
};

std::unique_ptr<module> basic_module();
} // namespace lidl