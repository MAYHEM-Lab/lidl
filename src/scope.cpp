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
    m_syms.emplace_back(&forward_decl);
    auto res         = symbol_handle(*this, m_syms.size());
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
        if (m_sibling) {
            return m_sibling->name_lookup(name);
        }
        return std::nullopt;
    }
    return it->second;
}

std::optional<symbol_handle> scope::definition_lookup(symbol def) const {
    auto it = std::find(m_syms.begin(), m_syms.end(), def);

    if (it == m_syms.end()) {
        if (m_sibling) {
            return m_sibling->definition_lookup(def);
        }
        return std::nullopt;
    }

    return symbol_handle(const_cast<scope&>(*this),
                         std::distance(m_syms.begin(), it) + 1);
}

const scope* scope::get_scope() const {
    return this;
}

bool is_defined(const symbol_handle& handle) {
    auto s = handle.get_scope();
    if (!s) {
        return false;
    }
    auto symbol = s->lookup(handle);
    return symbol != &forward_decl;
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

std::string_view local_name(const symbol_handle& handle) {
    return handle.get_scope()->nameof(handle);
}

std::vector<std::string_view> absolute_name(const symbol_handle& sym) {
    std::vector<const scope*> path;
    path.push_back(sym.get_scope());
    while (path.back()->parent()) {
        path.push_back(path.back()->parent());
    }

    std::vector<std::string_view> names;
    for (auto it = path.rbegin(); it + 1 != path.rend(); ++it) {
        auto handle = *(*it)->definition_lookup(*(it + 1));
        auto name   = (*it)->nameof(handle);
        if (name.empty()) {
            continue;
        }
        names.push_back(name);
    }

    names.push_back(local_name(sym));

    return names;
}

std::optional<symbol_handle> recursive_name_lookup(const scope& s,
                                                   std::string_view name) {
    if (auto sym = s.name_lookup(name); sym) {
        return sym;
    }
    if (s.parent()) {
        return recursive_name_lookup(*s.parent(), name);
    }
    return std::nullopt;
}

const scope& root_scope(const scope& s) {
    auto ptr = &s;
    for (; ptr->parent(); ptr = ptr->parent())
        ;
    return *ptr;
}
namespace {
std::optional<symbol_handle> do_recursive_definition_lookup(const scope& s, symbol name) {
    if (auto sym = s.definition_lookup(name); sym) {
        return sym;
    }
    for (auto& child : s.all_symbols()) {
        auto child_scope = child->get_scope();
        if (!child_scope || child_scope == &s) {
            continue;
        }
        if (auto res = do_recursive_definition_lookup(*child_scope, name)) {
            return res;
        }
    }
    return std::nullopt;
}
} // namespace

std::optional<symbol_handle> recursive_definition_lookup(const scope& s, symbol name) {
    return do_recursive_definition_lookup(root_scope(s), name);
}

namespace {
template<class OutIt>
void split(std::string_view sv, std::string_view delim, OutIt it) {
    while (true) {
        if (sv.empty())
            break;
        auto i = sv.find(delim);
        *it++  = sv.substr(0, i);
        if (i == sv.npos) {
            break;
        }
        sv = sv.substr(i + delim.size());
    }
}

inline std::vector<std::string_view> split(std::string_view sv, std::string_view delim) {
    std::vector<std::string_view> splitted;
    split(sv, delim, std::back_inserter(splitted));
    return splitted;
}
} // namespace

std::optional<symbol_handle>
recursive_full_name_lookup(const scope& s, const std::vector<std::string_view>& parts) {
    auto cur_lookup = recursive_name_lookup(s, parts[0]);
    if (!cur_lookup) {
        return std::nullopt;
    }
    if (parts.size() == 1) {
        return cur_lookup;
    }
    for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
        auto cur_scope = get_symbol(*cur_lookup)->get_scope();
        if (!cur_scope) {
            // scope does not exist
            return std::nullopt;
        }
        cur_lookup = cur_scope->name_lookup(*it);
    }
    return cur_lookup;
}

std::optional<symbol_handle> recursive_full_name_lookup(const scope& s,
                                                        std::string_view name) {
    return recursive_full_name_lookup(s, split(name, "."));
}

namespace {
std::ostream& indent(std::ostream& str, int level, char character = ' ') {
    for (int i = 0; i < level; ++i) {
        str << character;
    }
    return str;
}
}

void scope::dump(std::ostream& str, int indent_level) const {
    for (int i = 0; i < m_syms.size(); ++i) {
        if (m_syms[i] == this) continue;
        indent(str, indent_level) << m_names[i][0] << ":";
        if (auto scop = m_syms[i]->get_scope()) {
            scop->dump(str, indent_level + 1);
        }
    }
}
} // namespace lidl