#include "lidl/basic.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/types.hpp>
#include <lidl/yaml.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace lidl::yaml {
namespace {
std::shared_ptr<attribute> read_attribute(std::string_view name, const YAML::Node& node) {
    if (name == "raw") {
        return std::make_shared<detail::raw_attribute>();
    }
}

member read_member(const YAML::Node& node, const type_db& types) {
    auto type_name = node["type"];
    Expects(type_name);
    auto type = types.get(type_name.as<std::string>());
    if (!type) {
        // error
        throw std::runtime_error("no such type: " + type_name.as<std::string>());
    }
    member m;
    m.type_ = type;
    return m;
}

structure read_structure(const YAML::Node& node, const type_db& types) {
    structure s;
    auto members = node["members"];
    Expects(members);
    for (auto e : members) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        auto mem = read_member(val, types);
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
    type_db& module_types = m.symbols;
    add_basic_types(module_types);

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);

        if (val["type"].as<std::string>() == "structure") {
            module_types.declare(key.as<std::string>());
        }
    }

    for (auto e : node) {
        auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
        Expects(key);
        Expects(val);
        Expects(val["type"]);
        auto name = key.as<std::string>();

        if (val["type"].as<std::string>() == "structure") {
            auto s = read_structure(val, module_types);
            module_types.define(std::make_unique<user_defined_type>(name, s));
            m.structs.emplace(name, s);
        }
    }

    return m;
}
} // namespace lidl::yaml