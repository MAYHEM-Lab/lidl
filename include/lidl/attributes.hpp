#pragma once

#include <lidl/basic.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace lidl {
struct attribute {
    virtual ~attribute() = default;

    explicit attribute(std::string name)
        : m_name(std::move(name)) {
    }

    std::string_view name() const {
        return m_name;
    }

private:
    std::string m_name;
};

struct attribute_holder {
public:
    template<class T>
    const T* get(std::string_view name) const {
        auto untyped = get_untyped(name);
        return dynamic_cast<const T*>(untyped);
    }

    const attribute* get_untyped(std::string_view name) const {
        auto it = m_attribs.find(name);
        if (it == m_attribs.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    void add(std::unique_ptr<const attribute> attrib) {
        m_attribs.emplace(attrib->name(), std::move(attrib));
    }

    attribute_holder() = default;
    attribute_holder(const attribute_holder&) = delete;
    attribute_holder(attribute_holder&&) noexcept = default;
    attribute_holder& operator=(attribute_holder&&) noexcept = default;

private:
    std::unordered_map<std::string_view, std::unique_ptr<const attribute>> m_attribs;
};

namespace detail {
struct raw_attribute : attribute {
    raw_attribute(bool b)
        : attribute("raw")
        , raw(b) {
    }
    bool raw;
};

struct default_numeric_value_attribute : attribute {
    default_numeric_value_attribute(double f)
        : attribute("default")
        , val(f) {
    }
    double val;
};

struct nullable_attribute : attribute {
    nullable_attribute(bool n)
        : attribute("nullable")
        , nullable(n) {
    }
    bool nullable;
};
} // namespace detail
} // namespace lidl