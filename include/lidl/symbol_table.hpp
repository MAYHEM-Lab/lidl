#pragma once
#include "basic.hpp"
#include "generics.hpp"
#include "types.hpp"

#include <deque>
#include <string>
#include <unordered_map>
#include <variant>


namespace lidl {
struct type_handle {
private:
    type_handle(int id)
        : m_id(id) {
    }
    int m_id;
    friend class symbol_table;
};

struct generic_handle {
private:
    generic_handle(int id)
        : m_id(id) {
    }
    int m_id;
    friend class symbol_table;
};

class symbol_handle {
    explicit symbol_handle(int id)
        : m_id(id) {
    }
    int m_id;
    friend class symbol_table;
    friend class scope;
};

class scope {
public:
    struct forward_decl {};
    using elem = std::variant<const type*, const generic*, forward_decl>;

    symbol_handle declare(std::string_view name);

    void define(symbol_handle sym, const type* def);
    void redefine(symbol_handle sym, const type* def);

    void alias(std::string_view name, symbol_handle);

    elem lookup(symbol_handle handle) const {
        return m_syms.at(handle.m_id - 1);
    }

    scope& add_child_scope(std::string_view desc) const {
        m_children.emplace_back(desc, scope());
        return m_children.back().second;
    }

private:
    elem& mutable_lookup(symbol_handle handle) {
        return m_syms.at(handle.m_id - 1);
    }

    std::vector<elem> m_syms;

    std::unordered_map<std::string, symbol_handle> m_names;

    mutable std::list<std::pair<std::string_view, scope>> m_children;
};

bool is_defined(const scope& s, symbol_handle handle);

class symbol_table {
public:
    const symbol* lookup(std::string_view symbol_name) const;

    const type* lookup(const type_handle& handle);
    const generic* lookup(const generic_handle& handle);

    generic_handle declare_generic(const std::string& name,
                                   std::shared_ptr<const generic_declaration> def) {
        auto sym = allocate_name(name);
        if (!sym) {
            // name already exists!
            throw std::runtime_error("duplicate declaration: " + name);
        }

        auto [it, res] = m_generic_decls.emplace(sym, std::move(def));
        auto [newit, newres] =
            m_generics.emplace(sym, std::make_unique<detail::forward_decl>(it->second));
        *sym = newit->second.get();
        auto [id, idres] = m_gens.emplace(m_next_gen_id++, newit->second.get());
        return generic_handle{id->first};
    }

    type_handle declare_regular(const std::string& name) {
        auto sym = allocate_name(name);
        if (!sym) {
            // name already exists!
            throw std::runtime_error("duplicate declaration: " + name);
        }

        auto [it, res] = m_types.emplace(sym, &future);
        *sym = it->second;
        auto [id, idres] = m_typeids.emplace(m_next_type_id++, it->second);
        return type_handle{id->first};
    }

    auto define(const std::string& name, const type* t) {
        auto sym = allocate_name(name);
        return define(sym, t);
    }

    auto define(const std::string& name, std::unique_ptr<const generic> t) {
        auto sym = allocate_name(name);
        return define(sym, std::move(t));
    }

    void define(const symbol* sym, const type* t) {
        m_types[sym] = t;
        *find(sym) = m_types[find(sym)];
    }

    void define(const symbol* sym, std::unique_ptr<const generic> t) {
        m_generics[sym] = std::move(t);
        *find(sym) = m_generics[find(sym)].get();
    }

    const symbol* reverse_lookup(const type* t) const {
        auto res = std::find_if(
            m_types.begin(), m_types.end(), [&](auto& x) { return x.second == t; });
        if (res == m_types.end()) {
            return nullptr;
        }
        return res->first;
    }

    const symbol* reverse_lookup(const generic* t) const {
        auto res = std::find_if(m_generics.begin(), m_generics.end(), [&](auto& x) {
            return x.second.get() == t;
        });
        if (res == m_generics.end()) {
            return nullptr;
        }
        return res->first;
    }

    std::string_view name_of(const symbol* sym) const {
        auto res = std::find_if(sym_lookup.begin(), sym_lookup.end(), [&](auto& x) {
            return x.second == sym;
        });
        return res->first;
    }

private:
    symbol* find(const symbol* sym) {
        auto it = std::find(syms.begin(), syms.end(), *sym);
        return &*it;
    }

    std::unordered_map<std::string, symbol*> sym_lookup;
    std::deque<symbol> syms;

    symbol* allocate_name(const std::string& name) {
        auto it = sym_lookup.find(name);
        if (it != sym_lookup.end()) {
            return nullptr;
        }

        syms.emplace_back();
        auto [new_it, res] = sym_lookup.emplace(name, &syms.back());
        return new_it->second;
    }

    detail::future_type future;
    std::unordered_map<const symbol*, const type*> m_types;

    std::unordered_map<const symbol*, std::unique_ptr<const generic>> m_generics;
    std::unordered_map<const symbol*, std::shared_ptr<const generic_declaration>>
        m_generic_decls;

    int m_next_gen_id = 1;
    int m_next_type_id = 1;
    std::unordered_map<int, const generic*> m_gens;
    std::unordered_map<int, const type*> m_typeids;
};

void add_basic_types(symbol_table&);
} // namespace lidl