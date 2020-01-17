#pragma once

#include "basic.hpp"

#include <lidl/basic.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
namespace detail {
struct future_type : type {
    explicit future_type(std::string name)
        : type(name) {
    }
};
} // namespace detail

class user_defined_type : public type {
public:
    user_defined_type(std::string name, const structure& s)
        : type(name)
        , str(s) {
    }
    structure str;
};

class type_db {
public:
    void declare(std::string_view name) {
        m_types.emplace(name, std::make_unique<detail::future_type>(std::string(name)));
    }

    void define(std::unique_ptr<const type> t) {
        m_types[t->name()] = std::move(t);
    }

    const type* get(std::string_view name) const {
        auto it = m_types.find(name);
        if (it == m_types.end()) {
            return nullptr;
        }
        return it->second.get();
    }

private:
    std::unordered_map<std::string_view, std::unique_ptr<const type>> m_types;
};

void add_basic_types(type_db& db);
} // namespace lidl