#pragma once

#include <gsl/span>
#include <iostream>
#include <lidl/binary_writer.hpp>
#include <lidl/layout.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <lidl/source_info.hpp>

namespace lidl {
enum class type_categories {
    value,
    reference,
    view
};

struct module;
struct type {
public:
    virtual raw_layout wire_layout(const module& mod) const = 0;

    virtual bool is_reference_type(const module& mod) const = 0;

    virtual YAML::Node bin2yaml(const module&, ibinary_reader&) const = 0;

    virtual int yaml2bin(const module& mod, const YAML::Node&, ibinary_writer&) const = 0;

    virtual ~type() = default;

    std::optional<source_info> src_info;
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
} // namespace lidl