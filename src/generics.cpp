//
// Created by fatih on 1/18/20.
//

#include <fmt/format.h>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <lidl/types.hpp>
#include <map>
#include <stdexcept>

namespace lidl {
std::unique_ptr<wire_type> generic_structure::instantiate(const module& mod,
                                                          const name& ins) const {
    auto newstr = std::make_unique<structure>(const_cast<module*>(&mod), src_info);

    auto& genstr = dynamic_cast<const generic_structure&>(*get_symbol(ins.base));

    std::unordered_map<std::string_view, name> actual;
    int index = 0;
    for (auto& arg : ins.args) {
        if (auto n = std::get_if<name>(&arg)) {
            auto& paramname = (genstr.declaration.begin() + index)->first;
            actual.emplace(paramname, *n);
        }
        ++index;
    }

    for (auto& [member_name, mem] : genstr.struct_->all_members()) {
        if (get_symbol(mem.type_.base) != &forward_decl) {
            // Generic parameters are forward declarations.
            // If a member does not have a forward declared type, we can safely skip it.
            member new_mem(newstr.get());
            new_mem.type_ = mem.type_;
            newstr->add_member(std::string(member_name), std::move(new_mem));
            continue;
        }

        auto generic_param_name = local_name(mem.type_.base);
        auto it                 = actual.find(generic_param_name);

        if (it == actual.end()) {
            throw std::runtime_error("shouldn't happen");
        }

        member new_mem(newstr.get());
        new_mem.type_ = it->second;
        newstr->add_member(std::string(member_name), std::move(new_mem));
    }

    return newstr;
}

std::unique_ptr<wire_type> generic_union::instantiate(const module& mod,
                                                      const name& ins) const {
    auto newstr = std::make_unique<union_type>(const_cast<module*>(&mod), src_info);

    auto& genstr = dynamic_cast<const generic_union&>(*get_symbol(ins.base));

    std::unordered_map<std::string_view, name> actual;
    int index = 0;
    for (auto& arg : ins.args) {
        if (auto n = std::get_if<name>(&arg)) {
            auto& paramname = (genstr.declaration.begin() + index)->first;
            actual.emplace(paramname, *n);
        }
        ++index;
    }

    for (auto& [member_name, mem] : genstr.union_->all_members()) {
        if (get_symbol(mem->type_.base) != &forward_decl) {
            // Generic parameters are forward declarations.
            // If a member does not have a forward declared type, we can safely skip it.
            member new_mem(newstr.get());
            new_mem.type_ = mem->type_;
            newstr->add_member(std::string(member_name), std::move(new_mem));
            continue;
        }

        auto generic_param_name = local_name(mem->type_.base);
        auto it                 = actual.find(generic_param_name);

        if (it == actual.end()) {
            std::cerr << fmt::format("Could not find name: {}\n", generic_param_name);
            std::cerr << "The following names were provided:\n";
            for (auto& [name, _] : actual) {
                std::cerr << '"' << name << '"' << '\n';
            }
            throw std::runtime_error("shouldn't happen");
        }

        member new_mem(newstr.get());
        new_mem.type_ = it->second;
        newstr->add_member(std::string(member_name), std::move(new_mem));
    }

    return newstr;
}

generic_parameters
make_generic_declaration(std::vector<std::pair<std::string, std::string>> arg) {
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> res;
    for (auto& [name, type] : arg) {
        res.emplace_back(name, get_generic_parameter_for_type(type));
    }
    return generic_parameters(std::move(res));
}

std::unique_ptr<generic_parameter> get_generic_parameter_for_type(std::string_view type) {
    if (type == "type") {
        return std::make_unique<type_parameter>();
    }
    if (type == "i32") {
        return std::make_unique<value_parameter>();
    }
    return nullptr;
}

type_categories generic_wire_type_instantiation::category(const module& mod) const {
    return this->get_generic()->category(mod, args);
}

raw_layout generic_wire_type_instantiation::wire_layout(const module& mod) const {
    return this->get_generic()->wire_layout(mod, args);
}

YAML::Node generic_wire_type_instantiation::bin2yaml(const module& module,
                                                     ibinary_reader& reader) const {
    return this->get_generic()->bin2yaml(module, args, reader);
}

int generic_wire_type_instantiation::yaml2bin(const module& mod,
                                              const YAML::Node& node,
                                              ibinary_writer& writer) const {
    return this->get_generic()->yaml2bin(mod, args, node, writer);
}

type_categories generic_reference_type::category(const module& mod,
                                                 const name& instantiation) const {
    return type_categories::reference;
}
} // namespace lidl