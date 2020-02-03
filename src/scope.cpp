#include "lidl/basic.hpp"

#include <cassert>
#include <lidl/scope.hpp>
#include <stdexcept>



namespace lidl {
symbol_handle::symbol_handle(scope& s, int id)
    : m_id(id)
    , m_scope(s.weak_from_this()) {
}

scope* symbol_handle::get_scope() const {
    return m_scope.lock().get();
}

symbol_handle scope::declare(std::string_view name) {
    m_syms.emplace_back(forward_decl{});
    auto res = symbol_handle(*this, m_syms.size());
    auto [it, emres] = m_aliases.emplace(std::string(name), res);
    m_names.emplace_back();
    m_names.back().push_back(it->first);
    return res;
}

std::string_view scope::nameof(symbol_handle sym) const {
    return m_names[sym.get_id() - 1][0];
}

void scope::define(symbol_handle sym, symbol def) {
    assert(!is_defined(sym));
    redefine(sym, def);
}

void scope::redefine(symbol_handle sym, symbol def) {
    mutable_lookup(sym) = def;
}

std::optional<symbol_handle> scope::name_lookup(std::string_view name) const {
    auto it = m_aliases.find(std::string(name));
    if (it == m_aliases.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<symbol_handle> scope::definition_lookup(symbol def) const {
    auto it = std::find(m_syms.begin(), m_syms.end(), def);

    if (it == m_syms.end()) {
        return std::nullopt;
    }

    return symbol_handle(const_cast<scope&>(*this),
                         std::distance(m_syms.begin(), it) + 1);
}

bool is_defined(const symbol_handle& handle) {
    auto s = handle.get_scope();
    if (!s) {
        return false;
    }
    auto symbol = s->lookup(handle);
    return std::get_if<forward_decl>(&symbol) == nullptr;
}

symbol get_symbol(const symbol_handle& handle) {
    return handle.get_scope()->lookup(handle);
}

symbol_handle define(scope& s, std::string_view name, symbol def) {
    if (auto existing = s.name_lookup(name); existing) {
        if (is_defined(*existing)) {
            throw std::runtime_error("Can't redefine an existing name!");
        }
        s.define(*existing, def);
        return *existing;
    }

    auto decl = s.declare(name);
    s.define(decl, def);
    return decl;
}

std::string_view nameof(const symbol_handle& handle) {
    return handle.get_scope()->nameof(handle);
}

std::optional<symbol_handle> recursive_name_lookup(const scope& s,
                                                   std::string_view name) {
    if (auto sym = s.name_lookup(name); sym) {
        return sym;
    }
    return recursive_name_lookup(*s.parent(), name);
}
} // namespace lidl