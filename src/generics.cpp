//
// Created by fatih on 1/18/20.
//

#include <fmt/format.h>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <map>
#include <stdexcept>


namespace lidl {
structure generic_structure::instantiate(const module& mod,
                                         const generic_instantiation& ins) const {
    structure newstr;
    newstr.scope_ = mod.symbols->add_child_scope();

    auto& genstr = dynamic_cast<const generic_structure&>(ins.generic_type());

    std::unordered_map<std::string_view, name> actual;
    int index = 0;
    for (auto& arg : ins.arguments()) {
        if (auto n = std::get_if<name>(&arg)) {
            auto paramname = (genstr.declaration.begin() + index)->first;
            actual.emplace(paramname, *n);
        }
        ++index;
    }

    for (auto& [member_name, mem] : genstr.struct_.members) {
        if (!std::holds_alternative<forward_decl>(get_symbol(mem.type_.base))) {
            // Generic parameters are forward declarations.
            // If a member does not have a forward declared type, we can safely skip it.
            member new_mem;
            new_mem.type_ = mem.type_;
            newstr.members.emplace_back(member_name, std::move(new_mem));
            continue;
        }

        auto generic_param_name = nameof(mem.type_.base);
        auto it = actual.find(generic_param_name);

        if (it == actual.end()) {
            throw std::runtime_error("shouldn't happen");
        }

        name new_name;

        member new_mem;
        new_mem.type_ = it->second;
        newstr.members.emplace_back(member_name, std::move(new_mem));
    }

    return newstr;
}

union_type generic_union::instantiate(const module& mod,
                                      const generic_instantiation& ins) const {
    union_type newstr;

    auto& genstr = dynamic_cast<const generic_union&>(ins.generic_type());

    std::unordered_map<std::string_view, name> actual;
    int index = 0;
    for (auto& arg : ins.arguments()) {
        if (auto n = std::get_if<name>(&arg)) {
            auto paramname = (genstr.declaration.begin() + index)->first;
            actual.emplace(paramname, *n);
        }
        ++index;
    }

    for (auto& [member_name, mem] : genstr.union_.members) {
        if (!std::holds_alternative<forward_decl>(get_symbol(mem.type_.base))) {
            // Generic parameters are forward declarations.
            // If a member does not have a forward declared type, we can safely skip it.
            member new_mem;
            new_mem.type_ = mem.type_;
            newstr.members.emplace_back(member_name, std::move(new_mem));
            continue;
        }

        auto generic_param_name = nameof(mem.type_.base);
        auto it = actual.find(generic_param_name);

        if (it == actual.end()) {
            throw std::runtime_error("shouldn't happen");
        }

        member new_mem;
        new_mem.type_ = it->second;
        newstr.members.emplace_back(member_name, std::move(new_mem));
    }

    return newstr;
}
void generic_union::yaml2bin(const module& mod,
                             const generic_instantiation& instantiation,
                             const YAML::Node& node,
                             ibinary_writer& writer) const {
    return instantiate(mod, instantiation).yaml2bin(mod, node, writer);
}

generic_declaration
make_generic_declaration(std::vector<std::pair<std::string, std::string>> arg) {
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> res;
    for (auto& [name, type] : arg) {
        res.emplace_back(name, get_generic_parameter_for_type(type));
    }
    return generic_declaration(std::move(res));
}

generic::~generic() = default;

std::unique_ptr<generic_parameter> get_generic_parameter_for_type(std::string_view type) {
    if (type == "type") {
        return std::make_unique<type_parameter>();
    }
    if (type == "i32") {
        return std::make_unique<value_parameter>();
    }
    return nullptr;
}
} // namespace lidl