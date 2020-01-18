#pragma once

#include <algorithm>
#include <initializer_list>
#include <lidl/identifier.hpp>
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
}; // namespace lidl

struct member {
    const type* type_;
};

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

struct structure {
    bool is_raw() const {
        auto detected = std::all_of(members.begin(), members.end(), [](auto& mem) {
            return mem.second.type_->is_raw();
        });
        auto attrib = attributes.get<detail::raw_attribute>("raw");
        if (attrib) {
            if (attrib->raw && !detected) {
                throw std::runtime_error("Forced raw struct cannot be satisfied!");
            }
            return attrib->raw;
        }
        return detected;
    }

    std::map<std::string, member> members;
    attribute_holder attributes;
};
} // namespace lidl
