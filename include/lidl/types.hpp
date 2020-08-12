#pragma once

#include <gsl/span>
#include <iostream>
#include <lidl/binary_writer.hpp>
#include <lidl/layout.hpp>
#include <lidl/source_info.hpp>
#include <lidl/base.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace lidl {
enum class type_categories {
    value,
    reference,
    view
};

struct module;
struct type : public base {
public:
    virtual raw_layout wire_layout(const module& mod) const = 0;

    virtual type_categories category(const module& mod) const = 0;

    bool is_value(const module& mod) const {
        return category(mod) == type_categories::value;
    }

    bool is_reference_type(const module& mod) const {
        return category(mod) == type_categories::reference;
    }

    bool is_view(const module& mod) const {
        return category(mod) == type_categories::view;
    }

    virtual YAML::Node bin2yaml(const module&, ibinary_reader&) const = 0;

    virtual int yaml2bin(const module& mod, const YAML::Node&, ibinary_writer&) const = 0;

    virtual ~type() = default;

    std::optional<source_info> src_info;
};

struct value_type : type {
    type_categories category(const module& mod) const override {
        return type_categories::value;
    }
};

struct reference_type : type {
    type_categories category(const module& mod) const override {
        return type_categories::reference;
    }

    virtual raw_layout wire_layout(const module&) const override {
        return raw_layout{/*.size=*/2,
                          /*.alignment=*/2};
    }
};
} // namespace lidl