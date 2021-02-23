#include "yaml.hpp"

#include "lidl/basic.hpp"
#include "lidl/module_meta.hpp"
#include "lidl/scope.hpp"

#include <fstream>
#include <gsl/gsl_assert>
#include <lidl/enumeration.hpp>
#include <lidl/errors.hpp>
#include <lidl/generics.hpp>
#include <lidl/loader.hpp>
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

class yaml_loader : public module_loader {
    std::optional<source_info> make_source_info(const YAML::Node& node) {
        auto&& mark = node.Mark();
        if (mark.is_null()) {
            return {};
        }
        return source_info{mark.line, mark.column, mark.pos, m_origin};
    }

    generic_parameters parse_parameters(const YAML::Node& node) {
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

        return generic_parameters(std::move(params));
    }

    std::vector<generic_argument> parse_generic_args(const YAML::Node& node,
                                                     const base& s) {
        std::vector<std::string> arg_strs;
        if (node) {
            arg_strs = node.as<std::vector<std::string>>();
        }

        std::vector<generic_argument> args;

        for (auto& arg : arg_strs) {
            auto arg_lookup = recursive_full_name_lookup(s.get_scope(), arg);
            if (!arg_lookup) {
                args.emplace_back(std::stoll(arg));
                continue;
            }
            args.emplace_back(name{*arg_lookup});
        }

        return args;
    }

    name read_type(const YAML::Node& type_node, const base& s) {
        if (type_node.IsScalar()) {
            auto type_name = type_node.as<std::string>();
            auto lookup    = recursive_full_name_lookup(s.get_scope(), type_name);
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

            auto lookup = recursive_full_name_lookup(s.get_scope(), base_name);
            if (!lookup) {
                throw unknown_type_error(base_name, make_source_info(type_node));
            }

            auto args = parse_generic_args(type_node["parameters"], s);

            return name{*lookup, args};
        }
        throw std::runtime_error("shouldn't reach here");
    }

    parameter read_parameter(const YAML::Node& param_node, procedure& s) {
        auto p  = parameter{&s, make_source_info(param_node)};
        p.type  = read_type(param_node, p);
        p.flags = param_flags::in;
        return p;
    }

    static void read_member_attributes(const YAML::Node& attrib_node, member& mem) {
        if (!attrib_node) {
            return;
        }
        if (auto&& nullable = attrib_node["nullable"]) {
            mem.nullable = nullable.as<bool>();
        }
    }

    member read_member(const YAML::Node& node, base& s) {
        member m(&s, make_source_info(node));

        if (node.IsScalar()) {
            m.type_ = read_type(node, m);
            return m;
        }

        read_member_attributes(node["attributes"], m);

        auto type_name = node["type"];

        if (!type_name) {
            throw missing_node_error("type", make_source_info(node));
        }

        m.type_ = read_type(type_name, m);
        return m;
    }

    std::unique_ptr<structure> read_structure(const YAML::Node& node, base& scop) {
        auto s = std::make_unique<structure>(&scop, make_source_info(node));

        auto members = node["members"];
        if (!members) {
            throw missing_node_error("members", make_source_info(node));
        }
        for (auto&& e : members) {
            auto& [key, val] = static_cast<const std::pair<YAML::Node, YAML::Node>&>(e);
            s->add_member(key.as<std::string>(), read_member(val, *s));
        }

        return s;
    }

    std::unique_ptr<union_type> read_union(const YAML::Node& node, base& scop) {
        auto u = std::make_unique<union_type>(&scop, make_source_info(node));

        auto variants = node["variants"];
        if (!variants) {
            throw missing_node_error("variants", make_source_info(node));
        }

        if (auto base_node = node["extends"]) {
            u->set_base(read_type(base_node, *u));
        }

        for (auto&& e : variants) {
            auto& [key, val] = static_cast<const std::pair<YAML::Node, YAML::Node>&>(e);
            u->add_member(key.as<std::string>(), read_member(val, *u));
        }

        auto raw = node["raw"];
        u->raw   = raw && raw.as<bool>();

        return u;
    }

    std::unique_ptr<generic_structure> read_generic_structure(const YAML::Node& node,
                                                              base& gen_scope) {
        auto params = parse_parameters(node["parameters"]);

        auto genstr = std::make_unique<generic_structure>(
            std::move(params), &gen_scope, make_source_info(node));

        for (auto& [name, param] : genstr->declaration) {
            genstr->get_scope().declare(name);
        }

        genstr->struct_ = read_structure(node, *genstr);
        return genstr;
    }

    generic_union read_generic_union(const YAML::Node& node, base& scop) {
        auto params = parse_parameters(node["parameters"]);

        generic_union genstr{std::move(params), &scop, make_source_info(node)};

        for (auto& [name, param] : genstr.declaration) {
            genstr.get_scope().declare(name);
        }

        genstr.union_ = read_union(node, genstr);
        return genstr;
    }

    std::unique_ptr<procedure> parse_procedure(const YAML::Node& node, service& serv) {
        auto result = std::make_unique<procedure>(&serv, make_source_info(node));

        if (auto returns = node["returns"]; returns) {
            for (auto&& ret : returns) {
                result->return_types.push_back(read_type(ret, *result));
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
                result->add_parameter(name.as<std::string>(),
                                      read_parameter(val, *result));
            }
        }
        return result;
    }

    std::unique_ptr<service> parse_service(const YAML::Node& node, module& mod) {
        auto serv = std::make_unique<service>(&mod, make_source_info(node));

        if (auto&& base = node["extends"]; base) {
            serv->extends = read_type(base, *serv);
        }

        for (auto&& procedure : node["procedures"]) {
            auto& [name, val] =
                static_cast<const std::pair<YAML::Node, YAML::Node>&>(procedure);
            serv->add_procedure(name.as<std::string>(), parse_procedure(val, *serv));
        }
        return serv;
    }

    enumeration read_enum(const YAML::Node& node, base& scop) {
        enumeration e(&scop, make_source_info(node));

        if (auto base_node = node["extends"]) {
            e.set_base(read_type(base_node, e));
        }

        e.underlying_type = name{recursive_full_name_lookup(e.get_scope(), "i8").value()};
        for (auto&& member : node["members"]) {
            e.add_member(member.as<std::string>(), make_source_info(member));
        }
        return e;
    }

public:
    yaml_loader(load_context& ctx,
                const YAML::Node& node,
                std::optional<std::string> origin = {})
        : m_context{&ctx}
        , m_root{&ctx.get_root()}
        , m_node{node}
        , m_origin{std::move(origin)} {
        auto meta = get_metadata();
        m_mod     = &m_root->get_child(meta.name ? *meta.name : std::string("module"));
    }

    module& get_module() override {
        return *m_mod;
    }

    module_meta get_metadata() override {
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

    void declare_pass() {
        for (auto e : m_node) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            Expects(key);
            Expects(val);

            if (key.as<std::string>() == "$lidlmeta") {
                continue;
            }

            Expects(val["type"]);

            auto type_str = val["type"].as<std::string>();

            if (type_str == "structure") {
                m_mod->symbols().declare(key.as<std::string>());
            } else if (type_str == "union") {
                m_mod->symbols().declare(key.as<std::string>());
            } else if (type_str == "enumeration") {
                m_mod->symbols().declare(key.as<std::string>());
            } else if (type_str == "generic<structure>") {
                m_mod->symbols().declare(key.as<std::string>());
            } else if (type_str == "generic<union>") {
                m_mod->symbols().declare(key.as<std::string>());
            } else if (type_str == "service") {
                m_mod->symbols().declare(key.as<std::string>());
            }
        }
    }

    void define_pass() {
        for (auto e : m_node) {
            auto& [key, val] = static_cast<std::pair<YAML::Node, YAML::Node>&>(e);
            Expects(key);

            auto name = key.as<std::string>();

            if (name == "$lidlmeta") {
                continue;
            }

            Expects(val);
            Expects(val["type"]);

            auto type_str = val["type"].as<std::string>();

            if (type_str == "structure") {
                m_mod->structs.emplace_back(read_structure(val, *m_mod));
                define(m_mod->symbols(), name, m_mod->structs.back().get());
            } else if (type_str == "union") {
                m_mod->unions.emplace_back(read_union(val, *m_mod));
                define(m_mod->symbols(), name, m_mod->unions.back().get());
            } else if (type_str == "enumeration") {
                m_mod->enums.emplace_back(read_enum(val, *m_mod));
                define(m_mod->symbols(), name, &m_mod->enums.back());
            } else if (type_str == "generic<structure>") {
                m_mod->generic_structs.emplace_back(read_generic_structure(val, *m_mod));
                define(m_mod->symbols(), name, m_mod->generic_structs.back().get());
            } else if (type_str == "generic<union>") {
                m_mod->generic_unions.emplace_back(read_generic_union(val, *m_mod));
                define(m_mod->symbols(), name, &m_mod->generic_unions.back());
            } else if (type_str == "service") {
                m_mod->services.emplace_back(parse_service(val, *m_mod));
                define(m_mod->symbols(), name, m_mod->services.back().get());
            }
        }
    }

    void load() override {
        declare_pass();
        define_pass();
    }

    module& get() {
        return get_module();
    }

private:
    load_context* m_context;

    YAML::Node m_node;
    module* m_root;
    module* m_mod;
    std::optional<std::string> m_origin;
};
} // namespace
} // namespace lidl::yaml

namespace lidl {
std::unique_ptr<module_loader> make_yaml_loader(load_context& root,
                                                std::istream& file,
                                                std::optional<std::string> origin) {
    auto node = YAML::Load(file);
    return std::make_unique<yaml::yaml_loader>(root, node, origin);
}
} // namespace lidl