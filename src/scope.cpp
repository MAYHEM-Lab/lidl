#include <doctest.h>
#include <lidl/symbol_table.hpp>


namespace lidl {
symbol_handle scope::declare(std::string_view name) {
    m_syms.emplace_back(forward_decl{});
    auto res = symbol_handle(m_syms.size());
    m_names.emplace(std::string(name), res);
    return res;
}

void scope::define(symbol_handle sym, const type* def) {
    REQUIRE_FALSE(is_defined(*this, sym));
    redefine(sym, def);
}

void scope::redefine(symbol_handle sym, const type* def) {
    mutable_lookup(sym) = def;
}

bool is_defined(const scope& s, symbol_handle handle) {
    auto elem = s.lookup(handle);
    return std::get_if<scope::forward_decl>(&elem) == nullptr;
}
} // namespace lidl