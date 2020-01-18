#pragma once

#include "identifier.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
struct type {
public:
    virtual bool is_raw() const {
        return false;
    }

    identifier name() const {
        return m_name;
    }

    virtual ~type() = default;

protected:
    explicit type(identifier name)
        : m_name(std::move(name)) {
    }

private:
    identifier m_name;
};

namespace detail {
struct future_type : type {
    explicit future_type(identifier name)
        : type(name) {
    }
};
} // namespace detail

struct basic_type : type {
    explicit basic_type(identifier name, int bits)
        : type(name)
        , size_in_bits(bits) {
    }

    virtual bool is_raw() const override {
        return true;
    }

    int32_t size_in_bits;
};

struct integral_type : basic_type {
    explicit integral_type(identifier name, int bits, bool unsigned_)
        : basic_type(name, bits)
        , is_unsigned(unsigned_) {
    }

    bool is_unsigned;
};

struct half_type : basic_type {
    explicit half_type()
        : basic_type(identifier("f16"), 16) {
    }
};

struct float_type : basic_type {
    explicit float_type()
        : basic_type(identifier("f32"), 32) {
    }
};

struct double_type : basic_type {
    explicit double_type()
        : basic_type(identifier("f64"), 64) {
    }
};

struct string_type : type {
    explicit string_type()
        : type(identifier("string")) {
    }
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