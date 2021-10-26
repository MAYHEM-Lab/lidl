#include "parser.hpp"

#include <lidl/loader.hpp>
#include <lidl/module.hpp>

namespace lidl::frontend {
namespace {
template<class... Fs>
struct overload : Fs... {
    template<class... Ts>
    overload(Ts&&... ts)
        : Fs{std::forward<Ts>(ts)}... {
    }

    using Fs::operator()...;
};

template<class... Ts>
auto make_overload(Ts&&... ts) {
    return overload<std::remove_reference_t<Ts>...>(std::forward<Ts>(ts)...);
}

struct loader final : module_loader {
    loader(load_context& ctx, ast::module&& mod, std::optional<std::string> origin = {})
        : m_ast_mod(std::move(mod))
        , m_context{&ctx}
        , m_root{&ctx.get_root()}
        , m_origin{std::move(origin)} {
        auto meta = get_metadata();
        m_mod     = &m_root->get_child(meta.name ? *meta.name : std::string("module"));
        m_mod->src_info = source_info{
            .origin = m_origin
        };
    }

    void load() override {
        declare_pass();
        define_pass();
    }

    module& get_module() override {
        return *m_mod;
    }

    module_meta get_metadata() override {
        auto res = module_meta();
        if (!m_ast_mod.meta) {
            return res;
        }
        res.name    = m_ast_mod.meta->name_space;
        res.imports = m_ast_mod.meta->imports;
        return res;
    }

private:
    name parse(const ast::name& nm, base& s);
    std::unique_ptr<member> parse(const ast::member& mem, base& s);
    bool parse(const ast::structure& str, structure& res);
    bool parse(const ast::generic_structure& str, generic_structure& res);
    bool parse(const ast::generic_union& str, generic_union& res);
    bool parse(const ast::union_& str, union_type& s);
    bool parse(const ast::enumeration& str, enumeration& s);
    generic_parameters parse(const std::vector<ast::generic_parameter>& params);
    parameter parse(const ast::service::procedure::parameter& proc, base& s);
    std::unique_ptr<procedure> parse(const ast::service::procedure& proc, base& s);
    bool parse(const ast::service& serv, service& s);

    void add(std::string_view name, std::unique_ptr<structure>&& str) {
        m_mod->structs.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->structs.back().get());
    }

    void add(std::string_view name, std::unique_ptr<generic_structure>&& str) {
        m_mod->generic_structs.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->generic_structs.back().get());
    }

    void add(std::string_view name, std::unique_ptr<union_type>&& str) {
        m_mod->unions.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->unions.back().get());
    }

    void add(std::string_view name, std::unique_ptr<generic_union>&& str) {
        m_mod->generic_unions.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->generic_unions.back().get());
    }

    void add(std::string_view name, std::unique_ptr<enumeration>&& str) {
        m_mod->enums.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->enums.back().get());
    }

    void add(std::string_view name, std::unique_ptr<service>&& str) {
        m_mod->services.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->services.back().get());
    }

    template<class T>
    void add_default_top_level(std::string_view name) {
        m_mod->symbols().declare(name);
        add(name, std::make_unique<T>(&*m_mod));
    }

    void declare_pass();
    void define_pass();

    ast::module m_ast_mod;
    load_context* m_context;

    module* m_root;
    module* m_mod;
    std::optional<std::string> m_origin;
};

template<class T>
struct ast_to_lidl_map;

template<>
struct ast_to_lidl_map<ast::structure> {
    using type = structure;
};
template<>
struct ast_to_lidl_map<ast::generic_structure> {
    using type = generic_structure;
};
template<>
struct ast_to_lidl_map<ast::generic_union> {
    using type = generic_union;
};
template<>
struct ast_to_lidl_map<ast::union_> {
    using type = union_type;
};
template<>
struct ast_to_lidl_map<ast::enumeration> {
    using type = enumeration;
};
template<>
struct ast_to_lidl_map<ast::service> {
    using type = service;
};

template<class T>
using ast_to_lidl_map_t = typename ast_to_lidl_map<T>::type;

void lidl::frontend::loader::declare_pass() {
    for (const auto& e : m_ast_mod.elements) {
        std::visit(
            [this](auto& x) {
                if constexpr (!std::is_same_v<const ast::static_assertion&,
                                              decltype(x)>) {
#ifdef LIDL_VERBOSE_LOG
                    std::cerr << "Declare " << x.name << '\n';
#endif
                    using lidl_type = ast_to_lidl_map_t<
                        std::remove_const_t<std::remove_reference_t<decltype(x)>>>;
                    add_default_top_level<lidl_type>(x.name);
                }
            },
            e);
    }
}

void lidl::frontend::loader::define_pass() {
    for (const auto& e : m_ast_mod.elements) {
        std::visit(
            [this](auto& x) {
                if constexpr (!std::is_same_v<const ast::static_assertion&,
                                              decltype(x)>) {
#ifdef LIDL_VERBOSE_LOG
                    std::cerr << "Define " << x.name << '\n';
#endif
                    auto sym = m_mod->symbols().lookup(
                        m_mod->symbols().name_lookup(x.name).value());
                    using lidl_type = ast_to_lidl_map_t<
                        std::remove_const_t<std::remove_reference_t<decltype(x)>>>;
                    auto elem =
                        const_cast<lidl_type*>(dynamic_cast<const lidl_type*>(sym));
                    parse(x, *elem);
                }
            },
            e);
    }
}

name lidl::frontend::loader::parse(const ast::name& nm, base& s) {
    auto res = name();

    auto lookup = recursive_full_name_lookup(s.get_scope(), nm.base);

    if (!lookup) {
        report_user_error(
            error_type::fatal, nm.src_info, "Unknown type name: {}", nm.base);
    }

    res.base = *lookup;

    if (nm.args) {
        for (auto& arg : *nm.args) {
            res.args.push_back(std::visit(
                make_overload([&](const ast::name& sub_nm)
                                  -> generic_argument { return this->parse(sub_nm, s); },
                              [](const int64_t& num) -> generic_argument { return num; }),
                arg));
        }
    }

    return res;
}

std::unique_ptr<member> lidl::frontend::loader::parse(const ast::member& mem, base& s) {
    auto res = std::make_unique<member>(&s);

    res->type_ = parse(mem.type_name, *res);

    return res;
}

bool lidl::frontend::loader::parse(const ast::structure& str, structure& res) {
    for (auto& mem : str.body.members) {
        res.add_member(mem.name, *parse(mem, res));
    }
    return true;
}

bool lidl::frontend::loader::parse(const ast::union_& str, union_type& res) {
    for (auto& mem : str.body.members) {
        res.add_member(mem.name, *parse(mem, res));
    }
    return true;
}

bool lidl::frontend::loader::parse(const ast::enumeration& enu, enumeration& res) {
    res.underlying_type = name{recursive_full_name_lookup(res.get_scope(), "i8").value()};
    for (auto& [name, val] : enu.values) {
        res.add_member(name);
    }
    return true;
}

parameter lidl::frontend::loader::parse(const ast::service::procedure::parameter& proc,
                                        base& s) {
    parameter res(&s);

    res.type = parse(proc.type, res);

    return res;
}

std::unique_ptr<procedure>
lidl::frontend::loader::parse(const ast::service::procedure& proc, base& s) {
    auto res = std::make_unique<procedure>(&s);

    res->add_return_type(parse(proc.return_type, *res));

    for (auto& param : proc.params) {
        res->add_parameter(param.name, parse(param, *res));
    }

    return res;
}

bool lidl::frontend::loader::parse(const ast::service& serv, service& res) {
    if (serv.extends) {
        res.set_base(parse(*serv.extends, res));
    }

    for (auto& proc : serv.procedures) {
        res.add_procedure(proc.name, parse(proc, res));
    }

    return true;
}

bool lidl::frontend::loader::parse(const ast::generic_structure& str,
                                   generic_structure& res) {
    res.struct_     = std::make_unique<structure>(&res);
    res.declaration = parse(str.params);

    for (auto& [name, param] : res.declaration) {
        res.get_scope().declare(name);
    }

    for (auto& mem : str.body.members) {
        res.struct_->add_member(mem.name, *parse(mem, *res.struct_));
    }

    return true;
}

bool lidl::frontend::loader::parse(const ast::generic_union& str, generic_union& res) {
    res.union_      = std::make_unique<union_type>(&res);
    res.declaration = parse(str.params);

    for (auto& [name, param] : res.declaration) {
        res.get_scope().declare(name);
    }

    for (auto& mem : str.body.members) {
        res.union_->add_member(mem.name, *parse(mem, *res.union_));
    }

    return true;
}

generic_parameters
lidl::frontend::loader::parse(const std::vector<ast::generic_parameter>& params) {
    std::vector<std::pair<std::string, std::string>> pairs;

    pairs.reserve(params.size());
    for (auto& p : params) {
        pairs.emplace_back(p.name, "type");
    }

    return make_generic_declaration(pairs);
}
} // namespace
} // namespace lidl::frontend

namespace lidl {
std::shared_ptr<module_loader> make_frontend_loader(load_context& root,
                                                    std::istream& file,
                                                    std::optional<std::string> origin) {
    std::string data(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>{});

    auto mod = frontend::parse_module(data);

    if (!mod) {
        return nullptr;
    }

    return std::make_shared<frontend::loader>(root, std::move(*mod), origin);
}
} // namespace lidl