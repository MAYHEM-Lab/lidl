#include "lidl/basic.hpp"
#include "lidl/identifier.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/types.hpp>
#include <lidl/yaml.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace lidl::yaml {
namespace {
std::vector<identifier> parse_parameters(const YAML::Node& node) {
    if (!node) {
        return {};
    }

    std::vector<identifier> res;
    for (auto e : node) {
        res.emplace_back(e.as<std::string>());
    }
    return res;
}

identifier get_identifier_for_def(std::string_view base, const YAML::Node& node) {
    if (node["parameters"]) {
        return identifier(std::string(base), parse_parameters(node["parameters"]));
    }

    return identifier(std::string(base));
}

std::shared_ptr<attribute> read_attribute(std::string_view name, const YAML::Node& node) {
    if (name == "raw") {
        return std::make_shared<detail::raw_attribute>(node.as<bool>());
    }
    return nullptr;
}

member
read_member(const YAML::Node& node, const module& module, const type_db& param_symbols) {
    auto type_name = node["type"];
    Expects(type_name);
    if (type_name.IsScalar()) {
        auto type = param_symbols.get(type_name.as<std::string>());
        if (!type) {
            type = module.symbols.get(type_name.as<std::string>());
            if (!type) {
                // error
                throw std::runtime_error("no such type: " + type_name.as<std::string>());
            }
        }

        member m;
        m.type_ = type;
        return m;
    } else {
        auto base_name = type_name["name"].as<std::string>();

        std::vector<identifier> args;
        if (type_name["parameters"]) {
            args = parse_parameters(type_name["parameters"]);
        }

        auto type = module.symbols.get(base_name);
        if (type) {
            // not a generic
            if (!args.empty()) {
                throw std::runtime_error(base_name + " is not generic!");
            }
        } else {
            type = module.generics.get(base_name);
            if (!type) {
                // error
                throw std::runtime_error("no such type: " + type_name.as<std::string>());
            }

            if (type->name().arity() != args.size()) {
                throw std::runtime_error(
                    "mismatched number of args and params for generic");
            }

            type = new generic_instantiation(identifier(base_name, args), *type);
        }

        member m;
        m.type_ = type;
        return m;
    }
}

struct generic_parameter : type {
    generic_parameter(identifier id)
        : type(std::move(id)) {
    }
};

structure read_structure(const YAML::Node& node, const module& module) {
    structure s;

    auto param_symbols = new type_db;

    for (auto& param : parse_parameters(node["parameters"])) {
        param_symbols->define(std::make_unique<generic_parameter>(param));
    }

    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, module, *param_symbols);
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
} // namespace
module load_module(std::string_view path) {
    std::ifstream file{std::string(path)};
    auto node = YAML::Load(file);

    module m;
    add_basic_types(m.symbols);
    add_basic_types(m.generics);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            m.symbols.declare(get_identifier_for_def(key.as<std::string>(), val));
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            m.generics.declare(get_identifier_for_def(key.as<std::string>(), val));
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);
        auto identifier = get_identifier_for_def(key.as<std::string>(), val);

        if (val["type"].as<std::string>() == "structure") {
            auto s = read_structure(val, m);
            m.symbols.define(std::make_unique<user_defined_type>(identifier, s));
            m.structs.emplace_back(identifier, s);
        } else if (val["type"].as<std::string>() == "generic<structure>") {
            auto s = read_structure(val, m);
            m.symbols.define(std::make_unique<user_defined_type>(identifier, s));
            m.structs.emplace_back(identifier, s);
        }
    }

    return m;
}
} // namespace lidl::yaml