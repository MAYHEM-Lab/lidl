#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <lidl/basic.hpp>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace lidl::eval {
struct value {
    using vec_t = std::vector<value>;
    using map_t = std::unordered_map<std::string, value>;
    using fun_t = std::function<value(map_t)>;

    struct enum_val_t {
        name enum_value;
    };

    explicit value(std::string_view val)
        : m_data(std::string(val)) {
    }

    explicit value(uint64_t val)
        : m_data(val) {
    }

    explicit value(double val)
        : m_data(val) {
    }

    explicit value(bool val)
        : m_data(val) {
    }

    explicit value(fun_t val)
        : m_data(std::move(val)) {
    }

    explicit value(map_t val)
        : m_data(std::move(val)) {
    }

    explicit value(vec_t val)
        : m_data(std::move(val)) {
    }

    explicit value(enum_val_t val)
        : m_data(std::move(val)) {
    }

    bool is_string() const noexcept {
        return std::get_if<std::string>(&m_data);
    }

    bool is_double() const {
        return std::get_if<double>(&m_data);
    }

    bool is_integral() const {
        return std::get_if<uint64_t>(&m_data);
    }

    bool is_bool() const {
        return std::get_if<bool>(&m_data);
    }

    bool is_function() const {
        return std::get_if<fun_t>(&m_data);
    }

    bool is_vector() const {
        return std::get_if<vec_t>(&m_data);
    }

    bool is_map() const {
        return std::get_if<map_t>(&m_data);
    }

    std::string_view as_string() const noexcept {
        assert(is_string());
        return std::get<std::string>(m_data);
    }

    double as_double() const noexcept {
        assert(is_double());
        return std::get<double>(m_data);
    }

    uint64_t as_integral() const noexcept {
        assert(is_integral());
        return std::get<uint64_t>(m_data);
    }

    bool as_bool() const noexcept {
        assert(is_bool());
        return std::get<bool>(m_data);
    }

    friend bool operator==(const value& a, const value& b);

private:
    std::variant<std::string, uint64_t, double, bool, fun_t, map_t, vec_t, enum_val_t>
        m_data;
};

} // namespace lidl::eval