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
struct module : public base {
    using base::base;
    std::string name_space = "lidlmod";

    const module* parent = nullptr;

    std::deque<std::unique_ptr<type>> basic_types;
    std::deque<std::unique_ptr<generic>> basic_generics;

    std::vector<std::unique_ptr<structure>> structs;
    std::vector<std::unique_ptr<union_type>> unions;
    std::vector<std::unique_ptr<enumeration>> enums;

    std::vector<std::unique_ptr<generic_structure>> generic_structs;
    std::deque<generic_union> generic_unions;

    std::vector<std::unique_ptr<service>> services;
    mutable std::deque<generic_instantiation> instantiations;

    const generic_instantiation& create_or_get_instantiation(const name& ins) const;

    module& get_child(std::string_view child_name);

    module& add_child(std::string_view child_name, std::unique_ptr<module> child);

    mutable std::deque<std::pair<std::string, std::unique_ptr<module>>> children;
    mutable std::vector<std::pair<name, generic_instantiation*>> name_ins;
    module() = default;

    scope& symbols() {
        return get_scope();
    }

    scope& symbols() const {
        return const_cast<scope&>(get_scope());
    }

private:
    friend const module& get_root_module();
};

std::unique_ptr<module> basic_module();
} // namespace lidl