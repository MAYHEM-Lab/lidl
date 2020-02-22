#pragma once

#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <list>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>


namespace lidl {

struct forward_decl {};
inline bool operator==(forward_decl, forward_decl) {
    return false;
}

using symbol = std::variant<const type*, const generic*, forward_decl>;

class scope : public std::enable_shared_from_this<scope> {
public:
    scope() = default;
    explicit scope(std::weak_ptr<const scope> parent)
        : m_parent(parent) {
    }

    symbol_handle declare(std::string_view name);

    void define(symbol_handle sym, symbol def);
    void redefine(symbol_handle sym, symbol def);

    std::string_view nameof(symbol_handle sym) const;

    symbol lookup(symbol_handle handle) const {
        return m_syms.at(handle.m_id - 1);
    }

    std::shared_ptr<scope> add_child_scope() const {
        m_children.emplace_back(std::make_shared<scope>(weak_from_this()));
        return m_children.back();
    }

    std::optional<symbol_handle> name_lookup(std::string_view) const;
    std::optional<symbol_handle> definition_lookup(symbol def) const;

    const scope* parent() const {
        return m_parent.lock().get();
    }

private:
    symbol& mutable_lookup(const symbol_handle& handle) {
        return m_syms.at(handle.m_id - 1);
    }

    std::vector<symbol> m_syms;
    std::vector<std::vector<std::string>> m_names;

    std::unordered_map<std::string, symbol_handle> m_aliases;

    std::weak_ptr<const scope> m_parent;
    mutable std::vector<std::shared_ptr<scope>> m_children;
};

bool is_defined(const symbol_handle& handle);
symbol_handle define(scope& s, std::string_view name, symbol sym);

std::string_view nameof(const symbol_handle&);

std::optional<symbol_handle> recursive_name_lookup(const scope& s, std::string_view name);
std::optional<symbol_handle> recursive_definition_lookup(const scope& s, symbol name);

symbol get_symbol(const symbol_handle&);
} // namespace lidl