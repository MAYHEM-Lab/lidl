#include "yaml.hpp"

#include "lidl/attributes.hpp"
#include "lidl/basic.hpp"
#include "lidl/scope.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/enumeration.hpp>
#include <lidl/errors.hpp>
#include <lidl/generics.hpp>
#include <lidl/service.hpp>
#include <lidl/types.hpp>
#include <lidl/union.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace lidl::yaml {
namespace {
generic_declaration parse_parameters(const YAML::Node& node) {
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> params;

    for (auto e : node) {
        std::string name, type;
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        if (val.IsScalar()) {
            name = key.as<std::string>();
            type = val.as<std::string>();
        } else {
            name = e["name"].as<std::string>();
            type = e["type"].as<std::string>();
        }
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
        auto lookup    = recursive_name_lookup(s, type_name);
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
    if (node.IsScalar()) {
        m.type_ = read_type(node, s);
        return m;
    }

    m.attributes = read_attributes(node["attributes"]);

    auto type_name = node["type"];
    Expects(type_name);
    m.type_ = read_type(type_name, s);
    return m;
}

structure read_structure(const YAML::Node& node, scope& scop) {
    structure s;
    //    s.scope_ = scop.add_child_scope();

    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        s.members.emplace_back(key.as<std::string>(), read_member(val, scop));
    }

    s.attributes = read_attributes(node["attributes"]);
    return s;
}

union_type read_union(const YAML::Node& node, scope& scop) {
    union_type u;

    auto members = node["variants"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        u.members.emplace_back(key.as<std::string>(), read_member(val, scop));
    }

    u.attributes = read_attributes(node["attributes"]);
    return u;
}

generic_structure read_generic_structure(const YAML::Node& node, scope& gen_scope) {
    auto params = parse_parameters(node["parameters"]);
    for (auto& [name, param] : params) {
        gen_scope.declare(name);
    }

    auto str = read_structure(node, gen_scope);

    return generic_structure{std::move(params), std::move(str)};
}

generic_union read_generic_union(const YAML::Node& node, scope& scop) {
    auto params = parse_parameters(node["parameters"]);
    for (auto& [name, param] : params) {
        scop.declare(name);
    }

    auto str = read_union(node, scop);

    return generic_union{std::move(params), std::move(str)};
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
    for (auto base : node["extends"]) {
        serv.extends.push_back(read_type(base, *mod.symbols));
    }

    for (auto procedure : node["procedures"]) {
        auto& [name, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(procedure);
        serv.procedures.emplace_back(name.as<std::string>(), parse_procedure(val, mod));
    }
    return serv;
}

enumeration read_enum(const YAML::Node& node, const scope& scop) {
    enumeration e;
    e.underlying_type = name{recursive_name_lookup(scop, "i8").value()};
    for (auto member : node["members"]) {
        e.members.emplace_back(member.as<std::string>(),
                               enum_member{static_cast<int>(e.members.size())});
    }
    return e;
}
} // namespace
module& load_module(module& root, std::istream& file) {
    auto node = YAML::Load(file);

    std::string module_name = "default_module";
    auto metadata = node["$lidlmeta"];
    if (metadata) {
        if (auto ns = metadata["name"]; ns) {
            module_name = ns.as<std::string>();
        }
    }

    auto& m = root.get_child(module_name);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);

        if (key.as<std::string>() == "$lidlmeta") {
            continue;
        }

        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "union") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "enumeration") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "generic<union>") {
            m.symbols->declare(key.as<std::string>());
        } else if (val["type"].as<std::string>() == "service") {
            m.symbols->declare(key.as<std::string>());
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);

        auto name = key.as<std::string>();

        if (name == "$lidlmeta") {
            continue;
        }

        Expects(val);
        Expects(val["type"]);

        auto scope = m.symbols->add_child_scope(name);

        if (val["type"].as<std::string>() == "structure") {
            m.structs.emplace_back(read_structure(val, *scope));
            define(*m.symbols, name, &m.structs.back());
        } else if (val["type"].as<std::string>() == "union") {
            m.unions.emplace_back(read_union(val, *scope));
            define(*m.symbols, name, &m.unions.back());
        } else if (val["type"].as<std::string>() == "enumeration") {
            m.enums.emplace_back(read_enum(val, *scope));
            define(*m.symbols, name, &m.enums.back());
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            m.generic_structs.emplace_back(read_generic_structure(val, *scope));
            define(*m.symbols, name, &m.generic_structs.back());
        } else if (val["type"].as<std::string>() == "generic<union>") {
            m.generic_unions.emplace_back(read_generic_union(val, *scope));
            define(*m.symbols, name, &m.generic_unions.back());
        } else if (val["type"].as<std::string>() == "service") {
            m.services.emplace_back(parse_service(val, m));
            define(*m.symbols, name, &m.services.back());
        }
    }

    return m;
}
} // namespace lidl::yaml