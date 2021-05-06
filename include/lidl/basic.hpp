#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace lidl {
class base;
struct type;
struct wire_type;
struct enumeration;
struct union_type;
struct structure;
struct service;
struct procedure;

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
    scope* m_scope;
    friend class scope;
    friend bool operator==(const symbol_handle& left, const symbol_handle& right) {
        return left.m_scope == right.m_scope && left.m_id == right.m_id;
    }
    friend bool operator!=(const symbol_handle& left, const symbol_handle& right) {
        return left.m_scope != right.m_scope || left.m_id != right.m_id;
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

bool is_type(const name&);
bool is_service(const name&);

struct generic_argument : std::variant<name, int64_t> {
    using variant::variant;
    std::variant<name, int64_t>& get_variant() {
        return *this;
    }
    const std::variant<name, int64_t>& get_variant() const {
        return *this;
    }

    const name& as_name() const {
        return std::get<name>(*this);
    }
};

bool operator==(const symbol_handle& left, const symbol_handle& right);
bool operator==(const name&, const name&);

struct module;
const type* get_type(const module& mod, const name&);

template <class Type>
const Type* get_type(const module& mod, const name& n) {
    auto t = get_type(mod, n);
    if (!t) {
        return nullptr;
    }
    return dynamic_cast<const Type*>(t);
}

const wire_type* get_wire_type(const module& mod, const name& n);

// Reference types in lidl cannot be used directly (for instance a string), and must
// always go through a pointer. The reason being that they have unknown size.
// However, users aren't expected to use ptr<>s in their schemas.
// This function takes a name, and converts the given to T to a ptr<T> if needed.
// Returns true if the name was converted to a pointer.
bool add_pointer_to_name_if_needed(const module& mod, name& n);

struct procedure_params_info {
    const service* serv;
    std::string proc_name;
    const procedure* proc;
};

inline constexpr std::string_view scope_separator = "::";
inline constexpr std::string_view hidden_magic    = "#";
} // namespace lidl
