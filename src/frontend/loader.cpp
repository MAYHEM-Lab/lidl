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


struct loader : module_loader {
    loader(load_context& ctx, ast::module&& mod, std::optional<std::string> origin = {})
        : m_ast_mod(std::move(mod))
        , m_context{&ctx}
        , m_root{&ctx.get_root()}
        , m_origin{std::move(origin)} {
        auto meta = get_metadata();
        m_mod     = &m_root->get_child(meta.name ? *meta.name : std::string("module"));
    }

    void load() override {
        declare_pass();
        define_pass();
    }

    module& get_module() override {
        return *m_mod;
    }

    module_meta get_metadata() override {
        return module_meta();
    }

private:
    name parse(const ast::name& nm, base& s);
    std::unique_ptr<member> parse(const ast::member& mem, base& s);
    std::unique_ptr<structure> parse(const ast::structure& str, base& s);
    std::unique_ptr<union_type> parse(const ast::union_& str, base& s);
    std::unique_ptr<enumeration> parse(const ast::enumeration& str, base& s) {
        return nullptr;
    }

    void add(std::string_view name, std::unique_ptr<structure>&& str) {
        m_mod->structs.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->structs.back().get());
    }

    void add(std::string_view name, std::unique_ptr<union_type>&& str) {
        m_mod->unions.emplace_back(std::move(str));
        define(m_mod->symbols(), name, m_mod->unions.back().get());
    }

    void add(std::string_view name, std::unique_ptr<enumeration>&& str) {
        //        m_mod->enums.emplace_back(std::move(str));
        //        define(m_mod->symbols(), name, m_mod->structs.back().get());
    }

    void declare_pass();
    void define_pass();

    ast::module m_ast_mod;
    load_context* m_context;

    module* m_root;
    module* m_mod;
    std::optional<std::string> m_origin;
};

void lidl::frontend::loader::declare_pass() {
    for (const auto& e : m_ast_mod.elements) {
        std::visit(
            [this](auto& x) {
                std::cerr << "Declare " << x.name << '\n';
                m_mod->symbols().declare(x.name);
            },
            e);
    }
}

void lidl::frontend::loader::define_pass() {
    for (const auto& e : m_ast_mod.elements) {
        std::visit(
            [this](auto& x) {
                std::cerr << "Define " << x.name << '\n';
                add(x.name, parse(x, *m_mod));
            },
            e);
    }
}

name lidl::frontend::loader::parse(const ast::name& nm, base& s) {
    auto res = name();
    res.base = recursive_full_name_lookup(s.get_scope(), nm.base).value();

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

std::unique_ptr<structure> lidl::frontend::loader::parse(const ast::structure& str,
                                                         base& s) {
    auto res = std::make_unique<structure>(&s);
    for (auto& mem : str.members) {
        res->add_member(mem.name, *parse(mem, *res));
    }
    return res;
}

std::unique_ptr<union_type> lidl::frontend::loader::parse(const ast::union_& str,
                                                          base& s) {
    auto res = std::make_unique<union_type>(&s);
    for (auto& mem : str.members) {
        res->add_member(mem.name, *parse(mem, *res));
    }
    return res;
}
} // namespace
} // namespace lidl::frontend

namespace lidl {
std::unique_ptr<module_loader> make_frontend_loader(load_context& root,
                                                    std::istream& file,
                                                    std::optional<std::string> origin) {
    std::string data(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>{});

    auto mod = frontend::parse_module(data);

    if (!mod) {
        return nullptr;
    }

    return std::make_unique<frontend::loader>(root, std::move(*mod), origin);
}
} // namespace lidl