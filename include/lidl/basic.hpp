#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lidl {
struct type {
public:
    std::string_view name() const {
        return m_name;
    }
    virtual ~type() = default;

protected:
    explicit type(std::string name)
        : m_name(std::move(name)) {
    }

private:
    std::string m_name;
};

struct basic_type : type {
    explicit basic_type(std::string name, int bits)
        : type(name)
        , size_in_bits(bits) {
    }
    int32_t size_in_bits;
};

struct integral_type : basic_type {
    explicit integral_type(std::string name, int bits, bool unsigned_)
        : basic_type(name, bits)
        , is_unsigned(unsigned_) {
    }

    bool is_unsigned;
};

struct half_type : basic_type {
    explicit half_type()
        : basic_type("f16", 16) {
    }
};

struct float_type : basic_type {
    explicit float_type()
        : basic_type("f32", 32) {
    }
};

struct double_type : basic_type {
    explicit double_type()
        : basic_type("f64", 64) {
    }
};

struct string_type : type {
    explicit string_type()
        : type("string") {
    }
};

struct member {
    const type* type_;
    std::map<std::string, std::string> attributes;
};

struct attribute {
    ~attribute() = default;

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
    std::shared_ptr<const attribute> get(std::string_view name) const {
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
    raw_attribute()
        : attribute("raw") {
    }
};
} // namespace detail

struct structure {
    bool is_raw() const {
        return attributes.get("raw") != nullptr;
    }

    std::map<std::string, member> members;
    attribute_holder attributes;
};
} // namespace lidl
