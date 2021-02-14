#pragma once

#include <lidl/base.hpp>
#include <lidl/basic.hpp>
#include <list>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lidl {
struct forward_decl {};
inline bool operator==(const forward_decl&, const forward_decl&) {
    return false;
}

inline base forward_decl{};

class scope;

using symbol = const base*;

class scope
    : public base
    , public std::enable_shared_from_this<scope> {
public:
    explicit scope(std::weak_ptr<const scope> parent)
        : m_parent(std::move(parent)) {
        auto handle = declare("");
        define(handle, this);
        m_aliases.emplace("", handle);
    }

    scope()
        : scope(std::weak_ptr<const scope>()) {
    }

    symbol_handle declare(std::string_view name);

    void define(symbol_handle sym, symbol def);
    void redefine(symbol_handle sym, symbol def);

    /**
     * Returns the declared name of the given symbol handle.
     */
    std::string_view nameof(symbol_handle sym) const;

    symbol lookup(const symbol_handle& handle) const {
        return m_syms.at(handle.m_id - 1);
    }

    std::shared_ptr<scope> add_child_scope(std::string name,
                                           std::shared_ptr<scope> child = nullptr) {
        if (!child) {
            child = std::make_shared<scope>(weak_from_this());
        } else {
            child->set_parent(weak_from_this());
        }

        m_children.emplace_back(child);

        if (name.empty()) {
            child->m_sibling = m_sibling;
            m_sibling        = child.get();
        } else {
            auto handle = declare(name);
            define(handle, m_children.back().get());
            m_aliases.emplace(std::move(name), handle);
        }

        return m_children.back();
    }

    /**
     * Finds a symbol by it's name.
     */
    std::optional<symbol_handle> name_lookup(std::string_view) const;

    /**
     * Finds a symbol by it's definition.
     */
    std::optional<symbol_handle> definition_lookup(symbol def) const;

    const scope* parent() const {
        return m_parent.lock().get();
    }

    const std::vector<std::shared_ptr<scope>> children() const {
        return m_children;
    }

    std::vector<symbol_handle> all_handles() {
        std::vector<symbol_handle> res;
        for (size_t i = 0; i < m_syms.size(); ++i) {
            res.push_back(symbol_handle{*this, static_cast<int>(i + 1)});
        }
        return res;
    }

    const scope* get_scope() const override;

    const std::vector<symbol>& all_symbols() const {
        return m_syms;
    }
private:
    void set_parent(std::weak_ptr<scope> parent) {
        m_parent = std::move(parent);
    }

    symbol& mutable_lookup(const symbol_handle& handle) {
        return m_syms.at(handle.m_id - 1);
    }

    std::vector<symbol> m_syms;
    std::vector<std::vector<std::string>> m_names;

    /**
     * Siblings store scopes with the same name under the same parent
     */
    const scope* m_sibling = nullptr;

    std::unordered_map<std::string, symbol_handle> m_aliases;
    std::weak_ptr<const scope> m_parent;
    mutable std::vector<std::shared_ptr<scope>> m_children;
};

bool is_defined(const symbol_handle& handle);
symbol_handle define(scope& s, std::string_view name, symbol sym);

std::string_view local_name(const symbol_handle& sym);

std::vector<std::string_view> absolute_name(const symbol_handle& sym);

/**
 * Returns a list of names that reach the given symbol from the given scope in the
 * shortest manner.
 */
std::vector<std::string_view> relative_name(const symbol_handle& sym, const scope& from);

std::optional<symbol_handle> recursive_name_lookup(const scope& s, std::string_view name);

std::optional<symbol_handle> recursive_definition_lookup(const scope& s, symbol name);

std::optional<symbol_handle>
recursive_full_name_lookup(const scope& s, const std::vector<std::string_view>& name);

std::optional<symbol_handle> recursive_full_name_lookup(const scope& s,
                                                        std::string_view name);

symbol get_symbol(const symbol_handle&);
} // namespace lidl