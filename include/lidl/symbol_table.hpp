#pragma once
#include "basic.hpp"
#include "generics.hpp"
#include "types.hpp"

#include <deque>
#include <variant>

namespace lidl {
class symbol_table {
public:
    const symbol* lookup(std::string_view symbol_name) const;

    void declare_generic(const std::string& name,
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
    }

    void declare_regular(const std::string& name) {
        auto sym = allocate_name(name);
        if (!sym) {
            // name already exists!
            throw std::runtime_error("duplicate declaration: " + name);
        }

        auto [it, res] = m_types.emplace(sym, std::make_unique<detail::future_type>());
        *sym = it->second.get();
    }

    void define(const std::string& name, std::unique_ptr<const type> t) {
        auto sym = allocate_name(name);
        define(sym, std::move(t));
    }

    void define(const std::string& name, std::unique_ptr<const generic> t) {
        auto sym = allocate_name(name);
        define(sym, std::move(t));
    }

    void define(const symbol* sym, std::unique_ptr<const type> t) {
        m_types[sym] = std::move(t);
        *find(sym) = m_types[find(sym)].get();
    }

    void define(const symbol* sym, std::unique_ptr<const generic> t) {
        m_generics[sym] = std::move(t);
        *find(sym) = m_generics[find(sym)].get();
    }

    const symbol* reverse_lookup(const type* t) const {
        auto res = std::find_if(
            m_types.begin(), m_types.end(), [&](auto& x) { return x.second.get() == t; });
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

    std::unordered_map<const symbol*, std::unique_ptr<const type>> m_types;

    std::unordered_map<const symbol*, std::unique_ptr<const generic>> m_generics;
    std::unordered_map<const symbol*, std::shared_ptr<const generic_declaration>>
        m_generic_decls;
};

void add_basic_types(symbol_table&);
} // namespace lidl