#pragma once

#include "identifier.hpp"

#include <iostream>
#include <lidl/basic.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
namespace detail {
struct future_type : type {
    explicit future_type(identifier name)
        : type(name) {
    }
};
} // namespace detail

class user_defined_type : public type {
public:
    user_defined_type(identifier name, const structure& s)
        : type(name)
        , str(s) {
    }

    virtual bool is_raw() const override {
        return str.is_raw();
    }

    structure str;
};

class generic_instantiation : public type {
public:
    generic_instantiation(identifier id, const type& actual)
        : type(std::move(id))
        , m_actual(&actual) {
    }

    virtual bool is_raw() const override {
        return m_actual->is_raw();
    }

private:
    const type* m_actual;
};

class generics_table {
public:
    void declare(const identifier& name) {
        m_types.emplace(std::string(name.name),
                        std::make_unique<detail::future_type>(name));
    }

    void define(std::unique_ptr<const type> t) {
        m_types[t->name().name] = std::move(t);
    }

    const type* get(std::string_view name) const {
        auto it = m_types.find(std::string(name));
        if (it == m_types.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    void dump(std::ostream& err) const {
        for (auto& [name, type] : m_types) {
            err << '+' << to_string(type->name()) << '\n';
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<const type>> m_types;
};

class type_db {
public:
    void declare(const identifier& name) {
        m_types.emplace(to_string(name), std::make_unique<detail::future_type>(name));
    }

    void define(std::unique_ptr<const type> t) {
        m_types[to_string(t->name())] = std::move(t);
    }

    const type* get(std::string_view name) const {
        auto it = m_types.find(std::string(name));
        if (it == m_types.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    void dump(std::ostream& err) const {
        for (auto& [name, type] : m_types) {
            err << '+' << name << '\n';
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<const type>> m_types;
};

void add_basic_types(type_db& db);
void add_basic_types(generics_table& db);
} // namespace lidl