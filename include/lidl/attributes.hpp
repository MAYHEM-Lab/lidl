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
    std::shared_ptr<const T> get(std::string_view name) const {
        auto untyped = get_untyped(name);
        return std::dynamic_pointer_cast<const T>(untyped);
    }

    std::shared_ptr<const attribute> get_untyped(std::string_view name) const {
        auto it = m_attribs.find(name);
        if (it == m_attribs.end()) {
            return nullptr;
        }
        return it->second;
    }

    void add(std::shared_ptr<const attribute> attrib) {
        m_attribs.emplace(attrib->name(), std::move(attrib));
    }

private:
    std::unordered_map<std::string_view, std::shared_ptr<const attribute>> m_attribs;
};

namespace detail {
struct raw_attribute : attribute {
    raw_attribute(bool b)
        : attribute("raw")
        , raw(b) {
    }
    bool raw;
};
} // namespace detail
} // namespace lidl