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
struct module : public cbase<base::categories::module> {
    using cbase::cbase;

    module(const module&) = delete;
    module(module&&) = delete;

    const module* parent = nullptr;

    std::deque<std::unique_ptr<type>> basic_types;
    std::deque<std::unique_ptr<view_type>> basic_views;
    std::deque<std::unique_ptr<generic_type>> basic_generics;
    std::deque<std::unique_ptr<generic_view_type>> basic_generic_views;

    std::vector<std::unique_ptr<structure>> structs;
    std::vector<std::unique_ptr<union_type>> unions;
    std::vector<std::unique_ptr<enumeration>> enums;

    std::vector<std::unique_ptr<generic_structure>> generic_structs;
    std::vector<std::unique_ptr<generic_union>> generic_unions;

    std::vector<std::unique_ptr<service>> services;
    mutable std::vector<std::unique_ptr<basic_generic_instantiation>> instantiations;

    const basic_generic_instantiation& create_or_get_instantiation(const name& ins) const;

    module& get_child(qualified_name child_name);
    module& get_child(std::string_view child_name);

    module& add_child(qualified_name child_name, std::unique_ptr<module> child);
    module& add_child(std::string_view child_name, std::unique_ptr<module> child);

    mutable std::deque<std::pair<std::string, std::unique_ptr<module>>> children;
    mutable std::vector<std::pair<name, basic_generic_instantiation*>> name_ins;

    mutable std::vector<std::unique_ptr<base>> throwaway;

    // Modules that were imported during the loading of this module
    std::vector<module*> imported_modules;

    module() = default;

    scope& symbols() {
        return get_scope();
    }

    scope& symbols() const {
        return const_cast<scope&>(get_scope());
    }

    bool imported() const {
        return m_imported;
    }

    void set_imported() {
        m_imported = true;
    }

private:
    bool m_imported = false;
};

std::unique_ptr<module> basic_module();

inline const module& root_module(const module& mod) {
    auto ptr = &mod;
    while (ptr->parent) {
        ptr = ptr->parent;
    }
    return *ptr;
}
} // namespace lidl