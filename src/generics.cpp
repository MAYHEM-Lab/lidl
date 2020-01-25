//
// Created by fatih on 1/18/20.
//

#include <fmt/format.h>
#include <lidl/generics.hpp>
#include <lidl/module.hpp>
#include <stdexcept>
#include <map>

namespace lidl {
namespace {

struct type_parameter : generic_parameter {};

struct value_parameter : generic_parameter {
    const type* m_type;
};
} // namespace

std::shared_ptr<const generic_declaration>
make_generic_declaration(std::vector<std::pair<std::string, std::string>> arg) {
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> res;
    for (auto& [name, type] : arg) {
        res.emplace_back(name, get_generic_parameter_for_type(type));
    }
    return std::make_shared<generic_declaration>(std::move(res));
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

structure generic_structure::instantiate(const module& mod,
                                         const std::vector<generic_argument>& args) {
    if (args.size() != declaration->arity()) {
        throw std::runtime_error(fmt::format(
            "Mismatched arity expected {}, got {}", declaration->arity(), args.size()));
    }

    structure s;

    std::map<std::string, generic_argument> param_arg;

    auto decl_it = declaration->begin();
    auto arg_it = args.begin();
    while (arg_it != args.end()) {
        // TODO: validate args here
        param_arg.emplace(decl_it++->first, *arg_it++);
    }

    s.attributes = struct_.attributes;

    for (auto& [name, def] : struct_.members) {
        auto gen_type = dynamic_cast<const generic_type_parameter*>(def.type_);
        if (!gen_type) {
            s.members.emplace_back(name, def);
            continue;
        }
        member mem;
        // mem.type_ = mod.syms.lookup(param_arg[gen_type->name().name]);
    }

    return s;
}
} // namespace lidl