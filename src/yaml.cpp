#include "lidl/basic.hpp"
#include "lidl/identifier.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/generics.hpp>
#include <lidl/service.hpp>
#include <lidl/types.hpp>
#include <lidl/errors.hpp>
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
            throw no_generic_type(type);
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
    if (name == "nullable") {
        return std::make_shared<detail::nullable_attribute>(node.as<bool>());
    }
    if (name == "default") {
        return std::make_shared<detail::default_numeric_value_attribute>(node.as<double>());
    }
    else {
        throw unknown_attribute_error(name);
    }
    return nullptr;
}

const type* read_type(const YAML::Node& type_node, const module& module,
                      const symbol_table* scope_syms) {
    if (type_node.IsScalar()) {
        auto name = type_node.as<std::string>();
        decltype(scope_syms->lookup(name)) lookup =
            !scope_syms ? nullptr : scope_syms->lookup(name);
        if (!lookup || std::get_if<std::monostate>(lookup)) {
            lookup = module.symbols.lookup(name);
            if (std::get_if<std::monostate>(lookup)) {
                return nullptr;
            }
        }

        auto regular_type = std::get_if<const type*>(lookup);

        if (!regular_type) {
            throw std::runtime_error("not a regular type: " +
                                     type_node.as<std::string>());
        }

        return *regular_type;
    } else {
        auto base_name = type_node["name"].as<std::string>();

        auto lookup = module.symbols.lookup(base_name);
        if (std::get_if<std::monostate>(lookup)) {
            throw std::runtime_error("no such type: " + base_name);
        }

        std::vector<std::string> args;
        if (type_node["parameters"]) {
            args = parse_arguments(type_node["parameters"]);
        }

        if (auto regular_type = std::get_if<const type*>(lookup); regular_type) {
            if (!args.empty()) {
                throw std::runtime_error("expected a generic type, got a regular type!");
            }

            return *regular_type;
        } else if (auto gen_type = std::get_if<const generic*>(lookup); gen_type) {
            if ((*gen_type)->declaration->arity() != args.size()) {
                throw std::runtime_error(
                    "mismatched number of args and params for generic");
            }

            std::vector<generic_argument> name;
            name.push_back(lookup);

            for (auto& arg : args) {
                auto arg_lookup = module.symbols.lookup(arg);
                if (!arg_lookup) {
                    name.push_back(std::stoll(arg));
                    continue;
                }
                name.push_back(arg_lookup);
            }

            auto res = new generic_instantiation(**gen_type, name);
            symbol sym = res;
            module.generated.emplace(sym, name);
            return res;
        }
    }
    throw std::runtime_error("shouldn't reach here");
}

member read_member(const YAML::Node& node,
                   const module& module,
                   const symbol_table* scope_syms) {
    member m;
    auto attribs = node["attributes"];
    if (attribs) {
        for (auto e : attribs) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            auto attr = read_attribute(key.as<std::string>(), val);
            m.attributes.add(attr);
        }
    }

    auto type_name = node["type"];
    Expects(type_name);
    m.type_ = read_type(type_name, module, scope_syms);
    Expects(m.type_);
    return m;
}

structure read_structure(const YAML::Node& node, const module& module) {
    structure s;

    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, module, nullptr);
        s.members.emplace_back(key.as<std::string>(), mem);
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
        param_symbols->define(name, new generic_type_parameter());
    }

    auto& s = res.struct_;
    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, module, param_symbols);
        s.members.emplace_back(key.as<std::string>(), mem);
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

procedure parse_procedure(const YAML::Node& node, const module& mod) {
    procedure result;
    if (auto returns = node["returns"]; returns) {
        result.return_types.push_back(read_type(returns[0], mod, nullptr));
    }
    if (auto params = node["parameters"]; params) {
        for (auto param : params) {
            auto& [name, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(param);
            if (!val) {
                std::cerr << param << '\n';
                throw std::runtime_error("wtf");
            }
            auto type = read_type(val, mod, nullptr);
            if (!type) {
                throw std::runtime_error("All parameters must have a type!");
            }
            result.parameters.emplace_back(name.as<std::string>(), type);
        }
    }
    return result;
}

service parse_service(const YAML::Node& node, const module& mod) {
    service serv;
    for (auto procedure : node["procedures"]) {
        auto& [name, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(procedure);
        serv.procedures.emplace_back(name.as<std::string>(), parse_procedure(val, mod));
    }
    return serv;
}
} // namespace
module load_module(std::istream& file) {
    auto node = YAML::Load(file);

    module m;
    add_basic_types(m.symbols);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.symbols.declare_regular(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            auto decl = parse_parameters(val["paramters"]);
            m.symbols.declare_generic(key.as<std::string>(), std::move(decl));
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            auto s = read_structure(val, m);
            m.structs.emplace_back(s);
            m.symbols.define(m.symbols.lookup(key.as<std::string>()),
                          &m.structs.back());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            auto s = read_generic_structure(val, m);
            m.symbols.define(m.symbols.lookup(key.as<std::string>()),
                          std::make_unique<user_defined_generic>(s));
            m.generic_structs.emplace_back(key.as<std::string>(), std::move(s));
        } else if (val["type"].as<std::string>() == "service") {
            auto s = parse_service(val, m);
            m.services.emplace_back(key.as<std::string>(), std::move(s));
        }
    }

    return m;
}
} // namespace lidl::yaml