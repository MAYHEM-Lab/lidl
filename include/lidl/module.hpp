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
    std::string name_space;
    const module* parent = nullptr;
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

    const generic_instantiation& create_or_get_instantiation(const name& ins) const {
        auto it = std::find_if(name_ins.begin(), name_ins.end(), [&](auto& p) {
            return p.first == ins;
        });
        if (it != name_ins.end()) {
            return *it->second;
        }

        instantiations.emplace_back(ins);
        name_ins.emplace_back(ins, &instantiations.back());
        return instantiations.back();
    }

    module& get_child(std::string_view child_name) const {
        auto it = std::find_if(children.begin(), children.end(), [&](auto& child) {
            return child.first == child_name;
        });
        if (it != children.end()) {
            return *(it->second);
        }

        children.emplace_back(std::string(child_name), std::make_unique<module>());
        auto& res = *children.back().second;
        res.parent = this;
        res.symbols = symbols->add_child_scope();
        return res;
    }

    mutable std::deque<std::pair<std::string, std::unique_ptr<module>>> children;
    mutable std::vector<std::pair<name, generic_instantiation*>> name_ins;
    module() = default;

private:
    friend const module& get_root_module();
};

const module& get_root_module();
} // namespace lidl