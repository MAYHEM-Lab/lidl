#pragma once

#include <cassert>
#include <lidl/basic.hpp>
#include <list>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lidl {
using qualified_name = std::vector<std::string_view>;

using symbol = const base*;
extern base forward_decl;
class scope {
public:
    explicit scope(base& obj)
        : m_object{&obj} {
    }

    explicit scope()
        : m_object{nullptr} {
    }

    symbol_handle declare(std::string_view name);
    void define(symbol_handle sym, symbol def);
    void undefine(std::string_view name);

    /**
     * Returns the declared name of the given symbol handle.
     */
    std::string_view nameof(symbol_handle sym) const;

    symbol lookup(const symbol_handle& handle) const {
        return m_syms.at(handle.m_id - 1);
    }

    /**
     * Finds a symbol by it's name.
     */
    std::optional<symbol_handle> name_lookup(std::string_view) const;

    /**
     * Finds a symbol by it's definition.
     */
    std::optional<symbol_handle> definition_lookup(symbol def) const;

    std::vector<symbol_handle> all_handles() {
        std::vector<symbol_handle> res;
        for (size_t i = 0; i < m_syms.size(); ++i) {
            if (m_syms[i]) {
                res.push_back(symbol_handle{*this, static_cast<int>(i + 1)});
            }
        }
        return res;
    }

    const std::vector<symbol>& all_symbols() const {
        return m_syms;
    }

    void dump(std::ostream& str, int indent = 0) const;

    base& object() const {
        return *m_object;
    }

    void set_object(base& obj) {
        m_object = &obj;
    }

    const scope* parent_scope() const;
private:
    symbol& mutable_lookup(const symbol_handle& handle) {
        return m_syms.at(handle.m_id - 1);
    }

    // id -> symbol
    std::vector<symbol> m_syms;

    // name -> id
    std::unordered_map<std::string, symbol_handle> m_aliases;

    // id -> name
    std::vector<std::string_view> m_names;

    base* m_object;
};

bool is_defined(const symbol_handle& handle);
symbol_handle define(scope& s, std::string_view name, symbol sym);

std::string_view local_name(const symbol_handle& sym);

std::vector<symbol> path_to_root(const symbol_handle& sym);
qualified_name path_to_name(const std::vector<symbol>& path);

qualified_name absolute_name(const symbol_handle& sym);

std::optional<symbol_handle> recursive_name_lookup(const scope& s, std::string_view name);

std::optional<symbol_handle> recursive_definition_lookup(const scope& s, symbol name);

std::optional<symbol_handle>
recursive_full_name_lookup(const scope& s, const qualified_name& name);

std::optional<symbol_handle> recursive_full_name_lookup(const scope& s,
                                                        std::string_view name);

symbol get_symbol(const symbol_handle&);

const scope& root_scope(const scope& s);
} // namespace lidl