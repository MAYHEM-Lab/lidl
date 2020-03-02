#pragma once

#include <gsl/span>
#include <iostream>
#include <lidl/layout.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace lidl {
struct ibinary_writer {
    template <class T>
    void write(const T& t) {
        write_raw({reinterpret_cast<const char*>(&t), sizeof t});
    }

    void align(int alignment) {
        while((tell() % alignment) != 0) {
            char c = 0;
            write(c);
        }
    }

    virtual int tell() = 0;
    virtual void write_raw(gsl::span<const char> data) = 0;
    virtual ~ibinary_writer() = default;
};

struct module;
struct type {
public:
    virtual raw_layout wire_layout(const module& mod) const = 0;

    virtual bool is_reference_type(const module& mod) const = 0;

    virtual std::pair<YAML::Node, size_t> bin2yaml(const module&,
                                                   gsl::span<const uint8_t>) const = 0;

    virtual int yaml2bin(const module& mod, const YAML::Node&, ibinary_writer&) const = 0;

    virtual ~type() = default;
};

struct value_type : type {
    virtual bool is_reference_type(const module&) const override {
        return false;
    }
};

struct reference_type : type {
    virtual bool is_reference_type(const module&) const override {
        return true;
    }

    virtual raw_layout wire_layout(const module&) const override {
        return raw_layout{/*.size=*/2,
                          /*.alignment=*/2};
    }
};

namespace detail {
struct future_type : type {
    virtual raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a forward declaration!");
    }

    virtual bool is_reference_type(const module&) const override {
        return false;
    }
};
} // namespace detail
} // namespace lidl