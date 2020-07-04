#pragma once

#include <memory>
#include <variant>
#include <vector>

namespace lidl {
struct type;
struct generic;
struct enumeration;
struct union_type;
struct structure;
struct service;

class scope;

class symbol_handle {
public:
    symbol_handle() = default;

    scope* get_scope() const;
    int get_id() const {
        return m_id;
    }

    bool is_valid() const {
        return m_id != 0;
    }

private:
    explicit symbol_handle(scope& s, int id);
    int m_id = 0;
    std::weak_ptr<scope> m_scope;
    friend class scope;
    friend bool operator==(const symbol_handle& left, const symbol_handle& right) {
        return left.m_scope.lock() == right.m_scope.lock() && left.m_id == right.m_id;
    }
    friend bool operator!=(const symbol_handle& left, const symbol_handle& right) {
        return left.m_scope.lock() != right.m_scope.lock() || left.m_id != right.m_id;
    }
};

struct generic_argument;

/**
 * A name object refers to a concrete type in a lidl module.
 *
 * If the type is a generic instantiation, the name stores the arguments.
 */
struct name {
    symbol_handle base;
    std::vector<generic_argument> args;
};

struct generic_argument : std::variant<name, int64_t> {
    using variant::variant;
    std::variant<name, int64_t>& get_variant() {
        return *this;
    }
    const std::variant<name, int64_t>& get_variant() const {
        return *this;
    }
};

bool operator==(const symbol_handle& left, const symbol_handle& right);
bool operator==(const name&, const name&);

struct module;
const type* get_type(const module& mod, const name&);
} // namespace lidl
