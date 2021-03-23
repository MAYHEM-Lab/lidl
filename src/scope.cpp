#include "lidl/basic.hpp"

#include <cassert>
#include <iostream>
#include <lidl/base.hpp>
#include <lidl/scope.hpp>
#include <stdexcept>


namespace lidl {
base forward_decl{};

symbol_handle::symbol_handle(scope& s, int id)
    : m_id(id)
    , m_scope(&s) {
}

scope* symbol_handle::get_scope() const {
    return m_scope;
}

symbol_handle scope::declare(std::string_view name) {
    m_syms.emplace_back(&forward_decl);
    auto res         = symbol_handle(*this, m_syms.size());
    auto [it, emres] = m_aliases.emplace(std::string(name), res);
    assert(emres);
    m_names.emplace_back(it->first);
    return res;
}

std::string_view scope::nameof(symbol_handle sym) const {
    return m_names[sym.get_id() - 1];
}

void scope::define(symbol_handle sym, symbol def) {
    assert(!is_defined(sym));
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
    std::vector<const base*> path;
    path.push_back(&sym.get_scope()->object());
    while (path.back()->parent()) {
        path.push_back(path.back()->parent());
    }

    std::vector<std::string_view> names;
    for (auto it = path.rbegin(); it + 1 != path.rend(); ++it) {
        auto handle_res = (*it)->get_scope().definition_lookup(*(it + 1));
        assert(handle_res);
        auto handle = *handle_res;
        auto name   = (*it)->get_scope().nameof(handle);
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
    if (auto sym = s.name_lookup(name)) {
        return sym;
    }

    if (auto empty = s.name_lookup("")) {
        auto sym = get_symbol(*empty);
        assert(sym);
        if (auto res = recursive_name_lookup(sym->get_scope(), name)) {
            return res;
        }
    }

    if (s.object().parent()) {
        assert(s.parent_scope());
        return recursive_name_lookup(*s.parent_scope(), name);
    }

    return std::nullopt;
}

const scope& root_scope(const scope& s) {
    auto ptr = &s;
    for (; ptr->parent_scope(); ptr = ptr->parent_scope())
        ;
    return *ptr;
}
namespace {
std::optional<symbol_handle> do_recursive_definition_lookup(const scope& s, symbol name) {
    if (auto sym = s.definition_lookup(name); sym) {
        return sym;
    }
    for (auto& child : s.all_symbols()) {
        if (!child) {
            continue;
        }
        auto& child_scope = child->get_scope();
        if (auto res = do_recursive_definition_lookup(child_scope, name)) {
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

    for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
        auto& cur_scope = get_symbol(*cur_lookup)->get_scope();
        cur_lookup      = cur_scope.name_lookup(*it);
        if (!cur_lookup) {
            return std::nullopt;
        }
    }
    return cur_lookup;
}

std::optional<symbol_handle> recursive_full_name_lookup(const scope& s,
                                                        std::string_view name) {
    return recursive_full_name_lookup(s, split(name, "::"));
}

namespace {
std::ostream& indent(std::ostream& str, int level, char character = ' ') {
    for (int i = 0; i < level; ++i) {
        str << character;
    }
    return str;
}
} // namespace

void scope::dump(std::ostream& str, int indent_level) const {
    for (int i = 0; i < m_syms.size(); ++i) {
        if (!m_syms[i])
            continue;
        indent(str, indent_level) << "- " << m_names[i] << "\n";
        m_syms[i]->get_scope().dump(str, indent_level + 1);
    }
}

void scope::undefine(std::string_view name) {
    auto it = m_aliases.find(std::string(name));
    assert(it != m_aliases.end());

    auto handle = it->second;

    m_syms[handle.get_id() - 1]  = nullptr;
    m_names[handle.get_id() - 1] = {};
    m_aliases.erase(it);
}

const scope* scope::parent_scope() const {
    if (auto par_obj = object().parent()) {
        return &par_obj->get_scope();
    }
    return nullptr;
}
} // namespace lidl