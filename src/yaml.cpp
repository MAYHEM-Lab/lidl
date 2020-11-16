#include "yaml.hpp"

#include "lidl/basic.hpp"
#include "lidl/module_meta.hpp"
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
class missing_node_error : public error {
public:
    missing_node_error(std::string_view missing_key, std::optional<source_info> inf)
        : error(fmt::format("Missing key \"{}\"", missing_key), std::move(inf)) {
    }
};

class yaml_loader {
    std::optional<source_info> make_source_info(const YAML::Node& node) {
        auto&& mark = node.Mark();
        if (mark.is_null()) {
            return {};
        }
        return source_info{mark.line, mark.column, mark.pos, m_origin};
    }

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
                throw no_generic_type(type, make_source_info(node));
            }
            params.emplace_back(name, std::move(param_type));
        }

        return generic_declaration(std::move(params));
    }

    std::vector<generic_argument> parse_generic_args(const YAML::Node& node,
                                                     const scope& s) {
        std::vector<std::string> arg_strs;
        if (node) {
            arg_strs = node.as<std::vector<std::string>>();
        }

        std::vector<generic_argument> args;

        for (auto& arg : arg_strs) {
            auto arg_lookup = recursive_full_name_lookup(s, arg);
            if (!arg_lookup) {
                args.emplace_back(std::stoll(arg));
                continue;
            }
            args.emplace_back(name{*arg_lookup});
        }

        return args;
    }

    name read_type(const YAML::Node& type_node, const scope& s) {
        if (type_node.IsScalar()) {
            auto type_name = type_node.as<std::string>();
            auto lookup    = recursive_full_name_lookup(s, type_name);
            if (!lookup) {
                throw unknown_type_error(type_name, make_source_info(type_node));
            }
            return name{*lookup};
        } else {
            auto name_node = type_node["name"];
            if (!name_node) {
                throw missing_node_error("name", make_source_info(type_node));
            }

            auto base_name = name_node.as<std::string>();

            auto lookup = recursive_full_name_lookup(s, base_name);
            if (!lookup) {
                throw unknown_type_error(base_name, make_source_info(type_node));
            }

            auto args = parse_generic_args(type_node["parameters"], s);

            return name{*lookup, args};
        }
        throw std::runtime_error("shouldn't reach here");
    }

    parameter read_parameter(const YAML::Node& param_node, const scope& s) {
        auto type = read_type(param_node, s);

        return parameter{type, param_flags::in};
    }

    static void read_member_attributes(const YAML::Node& attrib_node, member& mem) {
        if (!attrib_node) {
            return;
        }
        if (auto&& nullable = attrib_node["nullable"]) {
            mem.nullable = nullable.as<bool>();
        }
    }

    member read_member(const YAML::Node& node, const scope& s) {
        member m;
        m.src_info = make_source_info(node);

        if (node.IsScalar()) {
            m.type_ = read_type(node, s);
            return m;
        }

        read_member_attributes(node["attributes"], m);

        auto type_name = node["type"];

        if (!type_name) {
            throw missing_node_error("type", make_source_info(node));
        }

        m.type_ = read_type(type_name, s);
        return m;
    }

    structure read_structure(const YAML::Node& node, scope& scop) {
        structure s;
        s.src_info = make_source_info(node);

        auto members = node["members"];
        if (!members) {
            throw missing_node_error("members", make_source_info(node));
        }
        for (auto&& e : members) {
            auto& [key, val] = static_cast<const std::pair<YAML::Node, YAML::Node>&>(e);
            s.members.emplace_back(key.as<std::string>(), read_member(val, scop));
        }

        return s;
    }

    union_type read_union(const YAML::Node& node, scope& scop) {
        union_type u;
        u.src_info = make_source_info(node);

        auto variants = node["variants"];
        if (!variants) {
            throw missing_node_error("variants", make_source_info(node));
        }
        for (auto&& e : variants) {
            auto& [key, val] = static_cast<const std::pair<YAML::Node, YAML::Node>&>(e);
            u.members.emplace_back(key.as<std::string>(), read_member(val, scop));
        }

        auto raw = node["raw"];
        u.raw    = raw && raw.as<bool>();

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
        result.src_info = make_source_info(node);

        if (auto returns = node["returns"]; returns) {
            for (auto&& ret : returns) {
                result.return_types.push_back(read_type(ret, *mod.symbols));
            }
        }
        if (auto params = node["parameters"]; params) {
            for (auto&& param : params) {
                auto& [name, val] =
                    static_cast<std::pair<YAML::Node, YAML::Node>&>(param);
                if (!val) {
                    std::cerr << param << '\n';
                    throw std::runtime_error("wtf");
                }
                result.parameters.emplace_back(name.as<std::string>(),
                                               read_parameter(val, *mod.symbols));
            }
        }
        return result;
    }

    service parse_service(const YAML::Node& node, const module& mod) {
        service serv;
        serv.src_info = make_source_info(node);

        for (auto&& base : node["extends"]) {
            serv.extends.push_back(read_type(base, *mod.symbols));
        }

        for (auto&& procedure : node["procedures"]) {
            auto& [name, val] =
                static_cast<const std::pair<YAML::Node, YAML::Node>&>(procedure);
            serv.procedures.emplace_back(name.as<std::string>(),
                                         parse_procedure(val, mod));
        }
        return serv;
    }

    enumeration read_enum(const YAML::Node& node, const scope& scop) {
        enumeration e;
        e.src_info = make_source_info(node);

        e.underlying_type = name{recursive_full_name_lookup(scop, "i8").value()};
        for (auto&& member : node["members"]) {
            e.members.emplace_back(member.as<std::string>(),
                                   enum_member{static_cast<int>(e.members.size()),
                                               make_source_info(member)});
        }
        return e;
    }

public:
    yaml_loader(module& root,
                const YAML::Node& node,
                std::optional<std::string> origin = {})
        : m_root{&root}
        , m_node{node}
        , m_origin{std::move(origin)} {
    }

    module_meta parse_metadata() {
        module_meta res;
        auto metadata = m_node["$lidlmeta"];
        if (metadata) {
            if (auto ns = metadata["name"]; ns) {
                res.name = ns.as<std::string>();
            }
            if (auto imports = metadata["imports"]; imports) {
                res.imports = imports.as<std::vector<std::string>>();
            }
        }
        return res;
    }

    module& start() {
        auto meta = parse_metadata();
        m_mod     = &m_root->get_child(meta.name ? *meta.name : std::string("module"));

        for (auto& import : meta.imports) {
            std::cerr << "Import " << import << '\n';
            auto path = fmt::format("{}.yaml", import);
            std::ifstream import_file(path);
            if (!import_file.good()) {
                std::cerr << "Could not open " << path << '\n';
            }
            auto import_node = YAML::Load(import_file);
            yaml_loader loader(*m_root, import_node);

            auto imported_meta = loader.parse_metadata();

            auto& imported_module = loader.start();
            m_imports.push_back(std::move(loader));
        }

        return *m_mod;
    }

    void declare_pass() {
        for (auto& imported : m_imports) {
            imported.declare_pass();
        }

        for (auto e : m_node) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            Expects(key);
            Expects(val);

            if (key.as<std::string>() == "$lidlmeta") {
                continue;
            }

            Expects(val["type"]);

            if (val["type"].as<std::string>() == "structure") {
                m_mod->symbols->declare(key.as<std::string>());
            } else if (val["type"].as<std::string>() == "union") {
                m_mod->symbols->declare(key.as<std::string>());
            } else if (val["type"].as<std::string>() == "enumeration") {
                m_mod->symbols->declare(key.as<std::string>());
            } else if (val["type"].as<std::string>() == "generic<structure>") {
                m_mod->symbols->declare(key.as<std::string>());
            } else if (val["type"].as<std::string>() == "generic<union>") {
                m_mod->symbols->declare(key.as<std::string>());
            } else if (val["type"].as<std::string>() == "service") {
                m_mod->symbols->declare(key.as<std::string>());
            }
        }
    }

    void define_pass() {
        for (auto& imported : m_imports) {
            imported.define_pass();
        }

        for (auto e : m_node) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            Expects(key);

            auto name = key.as<std::string>();

            if (name == "$lidlmeta") {
                continue;
            }

            Expects(val);
            Expects(val["type"]);

            auto scope = m_mod->symbols->add_child_scope(name);

            if (val["type"].as<std::string>() == "structure") {
                m_mod->structs.emplace_back(read_structure(val, *scope));
                define(*m_mod->symbols, name, &m_mod->structs.back());
            } else if (val["type"].as<std::string>() == "union") {
                m_mod->unions.emplace_back(read_union(val, *scope));
                define(*m_mod->symbols, name, &m_mod->unions.back());
            } else if (val["type"].as<std::string>() == "enumeration") {
                m_mod->enums.emplace_back(read_enum(val, *scope));
                define(*m_mod->symbols, name, &m_mod->enums.back());
            } else if (val["type"].as<std::string>() == "generic<structure>") {
                m_mod->generic_structs.emplace_back(read_generic_structure(val, *scope));
                define(*m_mod->symbols, name, &m_mod->generic_structs.back());
            } else if (val["type"].as<std::string>() == "generic<union>") {
                m_mod->generic_unions.emplace_back(read_generic_union(val, *scope));
                define(*m_mod->symbols, name, &m_mod->generic_unions.back());
            } else if (val["type"].as<std::string>() == "service") {
                m_mod->services.emplace_back(parse_service(val, *m_mod));
                define(*m_mod->symbols, name, &m_mod->services.back());
            }
        }
    }

    module& get() {
        return *m_mod;
    }

private:
    std::vector<yaml_loader> m_imports;

    YAML::Node m_node;
    module* m_root;
    module* m_mod;
    std::optional<std::string> m_origin;
};
} // namespace

module& load_module(module& root, std::istream& file, std::optional<std::string> origin) {
    auto node = YAML::Load(file);

    yaml_loader loader(root, node, origin);

    auto& m = loader.start();

    loader.declare_pass();
    loader.define_pass();
    return m;
}
} // namespace lidl::yaml
