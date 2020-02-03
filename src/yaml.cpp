#include "yaml.hpp"

#include "lidl/attributes.hpp"
#include "lidl/basic.hpp"
#include "lidl/identifier.hpp"
#include "lidl/scope.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/errors.hpp>
#include <lidl/generics.hpp>
#include <lidl/service.hpp>
#include <lidl/types.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>



namespace lidl::yaml {
namespace {
generic_declaration parse_parameters(const YAML::Node& node) {
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

    return generic_declaration(std::move(params));
}

std::unique_ptr<attribute> read_attribute(std::string_view name, const YAML::Node& node) {
    if (name == "raw") {
        return std::make_unique<detail::raw_attribute>(node.as<bool>());
    }
    if (name == "nullable") {
        return std::make_unique<detail::nullable_attribute>(node.as<bool>());
    }
    if (name == "default") {
        return std::make_unique<detail::default_numeric_value_attribute>(
            node.as<double>());
    } else {
        throw unknown_attribute_error(name);
    }
    return nullptr;
}

std::vector<generic_argument> parse_generic_args(const YAML::Node& node, const scope& s) {
    std::vector<std::string> arg_strs;
    if (node) {
        arg_strs = node.as<std::vector<std::string>>();
    }

    std::vector<generic_argument> args;

    for (auto& arg : arg_strs) {
        auto arg_lookup = recursive_name_lookup(s, arg);
        if (!arg_lookup) {
            args.push_back(std::stoll(arg));
            continue;
        }
        args.push_back(name{*arg_lookup});
    }

    return args;
}

name read_type(const YAML::Node& type_node, const scope& s) {
    if (type_node.IsScalar()) {
        auto type_name = type_node.as<std::string>();
        auto lookup = recursive_name_lookup(s, type_name);
        if (!lookup) {
            throw std::runtime_error("no such type: " + type_name);
        }
        return name{*lookup};
    } else {
        auto base_name = type_node["name"].as<std::string>();

        auto lookup = recursive_name_lookup(s, base_name);
        if (!lookup) {
            throw std::runtime_error("no such type: " + base_name);
        }

        auto args = parse_generic_args(type_node["parameters"], s);

        return name{*lookup, args};
    }
    throw std::runtime_error("shouldn't reach here");
}

attribute_holder read_attributes(const YAML::Node& attrib_node) {
    attribute_holder attrs;

    if (attrib_node) {
        for (auto e : attrib_node) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            attrs.add(read_attribute(key.as<std::string>(), val));
        }
    }

    return attrs;
}

member read_member(const YAML::Node& node, const scope& s) {
    member m;
    m.attributes = read_attributes(node["attributes"]);

    auto type_name = node["type"];
    Expects(type_name);
    m.type_ = read_type(type_name, s);
    return m;
}

structure read_structure(const YAML::Node& node, const scope& scop) {
    structure s;
    s.scope = scop.add_child_scope();

    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        s.members.emplace_back(key.as<std::string>(), read_member(val, *s.scope.lock()));
    }

    s.attributes = read_attributes(node["attributes"]);
    return s;
}

generic_structure read_generic_structure(const YAML::Node& node, const scope& scop) {
    auto gen_scope = scop.add_child_scope();

    auto params = parse_parameters(node["parameters"]);
    for (auto& [name, param] : params) {
        gen_scope->declare(name);
    }

    auto str = read_structure(node, *gen_scope);

    return generic_structure{std::move(params), std::move(str)};
}

procedure parse_procedure(const YAML::Node& node, const module& mod) {
    procedure result;
    if (auto returns = node["returns"]; returns) {
        result.return_types.push_back(read_type(returns[0], *mod.symbols));
    }
    if (auto params = node["parameters"]; params) {
        for (auto param : params) {
            auto& [name, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(param);
            if (!val) {
                std::cerr << param << '\n';
                throw std::runtime_error("wtf");
            }
            result.parameters.emplace_back(name.as<std::string>(),
                                           read_type(val, *mod.symbols));
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
    add_basic_types(m);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            m.symbols->declare(key.as<std::string>());
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.structs.emplace_back(read_structure(val, *m.symbols));
            define(*m.symbols, key.as<std::string>(), &m.structs.back());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            m.generic_structs.emplace_back(read_generic_structure(val, *m.symbols));
            define(*m.symbols, key.as<std::string>(), &m.generic_structs.back());
        } else if (val["type"].as<std::string>() == "service") {
            m.services.emplace_back(key.as<std::string>(), parse_service(val, m));
        }
    }

    return m;
}
} // namespace lidl::yaml