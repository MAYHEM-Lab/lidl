#include "lidl/basic.hpp"
#include "lidl/identifier.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/generics.hpp>
#include <lidl/types.hpp>
#include <lidl/yaml.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace lidl::yaml {
namespace {
std::shared_ptr<const generic_declaration> parse_parameters(const YAML::Node& node) {
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> params;

    for (auto e : node) {
        auto name = e["name"].as<std::string>();
        auto type = e["type"].as<std::string>();
        auto param_type = get_generic_parameter_for_type(type);
        if (!param_type) {
            throw std::runtime_error("No such generic param type: " + type);
        }
        params.emplace_back(name, std::move(param_type));
    }

    return std::make_shared<generic_declaration>(std::move(params));
}

std::vector<std::string> parse_arguments(const YAML::Node& node) {
    std::vector<std::string> args;

    for (auto e : node) {
        args.emplace_back(e.as<std::string>());
    }

    return args;
}

std::shared_ptr<attribute> read_attribute(std::string_view name, const YAML::Node& node) {
    if (name == "raw") {
        return std::make_shared<detail::raw_attribute>(node.as<bool>());
    }
    return nullptr;
}

member read_member(const YAML::Node& node,
                   const module& module,
                   const symbol_table* scope_syms) {
    auto type_name = node["type"];
    Expects(type_name);
    if (type_name.IsScalar()) {
        auto name = type_name.as<std::string>();
        decltype(scope_syms->lookup(name)) lookup =
            !scope_syms ? nullptr : scope_syms->lookup(name);
        if (!lookup || std::get_if<std::monostate>(lookup)) {
            lookup = module.syms.lookup(name);
            if (std::get_if<std::monostate>(lookup)) {
                throw std::runtime_error("no such type: " + type_name.as<std::string>());
            }
        }

        auto regular_type = std::get_if<const type*>(lookup);

        if (!regular_type) {
            throw std::runtime_error("not a regular type: " +
                                     type_name.as<std::string>());
        }

        member m;
        m.type_ = *regular_type;
        return m;
    } else {
        auto base_name = type_name["name"].as<std::string>();

        auto lookup = module.syms.lookup(base_name);
        if (std::get_if<std::monostate>(lookup)) {
            throw std::runtime_error("no such type: " + base_name);
        }

        std::vector<std::string> args;
        if (type_name["parameters"]) {
            args = parse_arguments(type_name["parameters"]);
        }

        if (auto regular_type = std::get_if<const type*>(lookup); regular_type) {
            if (!args.empty()) {
                throw std::runtime_error("expected a generic type, got a regular type!");
            }

            member m;
            m.type_ = *regular_type;
            return m;
        } else if (auto gen_type = std::get_if<const generic_type*>(lookup); gen_type) {
            if ((*gen_type)->declaration->arity() != args.size()) {
                throw std::runtime_error(
                    "mismatched number of args and params for generic");
            }

            std::vector<generic_argument> name;
            name.push_back(lookup);

            for (auto& arg : args) {
                auto arg_lookup = module.syms.lookup(arg);
                if (!arg_lookup) {
                    name.push_back(std::stoll(arg));
                    continue;
                }
                name.push_back(arg_lookup);
            }

            member m;
            m.type_ = new generic_instantiation(**gen_type, name);
            symbol sym = m.type_;
            module.generated.emplace(sym, name);
            return m;
        }
    }
    throw std::runtime_error("nope");
}

structure read_structure(const YAML::Node& node, const module& module) {
    structure s;

    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, module, nullptr);
        s.members.emplace(key.as<std::string>(), mem);
    }

    auto attribs = node["attributes"];
    if (attribs) {
        for (auto e : attribs) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            auto attr = read_attribute(key.as<std::string>(), val);
            s.attributes.add(attr);
        }
    }
    return s;
}

generic_structure read_generic_structure(const YAML::Node& node, const module& module) {
    generic_structure res{parse_parameters(node["parameters"])};

    auto param_symbols = new symbol_table;

    for (auto& [name, param] : *res.declaration) {
        param_symbols->define(name, std::make_unique<generic_type_parameter>());
    }

    auto& s = res.struct_;
    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, module, param_symbols);
        s.members.emplace(key.as<std::string>(), mem);
    }

    auto attribs = node["attributes"];
    if (attribs) {
        for (auto e : attribs) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            auto attr = read_attribute(key.as<std::string>(), val);
            s.attributes.add(attr);
        }
    }

    return res;
}
} // namespace
module load_module(std::string_view path) {
    std::ifstream file{std::string(path)};
    auto node = YAML::Load(file);

    module m;
    add_basic_types(m.syms);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.syms.declare_regular(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            auto decl = parse_parameters(val["paramters"]);
            m.syms.declare_generic(key.as<std::string>(), std::move(decl));
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            auto s = read_structure(val, m);
            m.syms.define(m.syms.lookup(key.as<std::string>()),
                          std::make_unique<user_defined_type>(s));
            m.structs.emplace_back(key.as<std::string>(), s);
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            auto s = read_generic_structure(val, m);
            m.syms.define(m.syms.lookup(key.as<std::string>()),
                          std::make_unique<user_defined_generic>(s));
            m.generic_structs.emplace_back(key.as<std::string>(),
                                           std::move(s));
        }
    }

    return m;
}
} // namespace lidl::yaml