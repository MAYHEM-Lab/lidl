#pragma once

#include <gsl/span>
#include <iostream>
#include <lidl/base.hpp>
#include <lidl/basic.hpp>
#include <lidl/binary_writer.hpp>
#include <lidl/layout.hpp>
#include <lidl/scope.hpp>
#include <lidl/source_info.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace lidl {
enum class type_categories
{
    value,
    reference,
    view
};

struct module;
struct type : public cbase<base::categories::type> {
public:
    using cbase::cbase;

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

    virtual std::optional<name> get_wire_type(const module& mod) const {
        return {};
    }
};

struct view_type : type {
    explicit view_type(base* parent = nullptr, std::optional<source_info> loc = {})
        : type(parent, loc) {
    }

    type_categories category(const module& mod) const final {
        return type_categories::view;
    }
};

struct known_view_type : view_type {
    explicit known_view_type(name wire_type,
                             base* parent                   = nullptr,
                             std::optional<source_info> loc = {})
        : view_type(parent, loc)
        , m_wire_type{std::move(wire_type)} {
    }

    std::optional<name> get_wire_type(const module& mod) const override {
        return m_wire_type;
    }

private:
    name m_wire_type;
};

struct wire_type : type {
    using type::type;
    virtual raw_layout wire_layout(const module& mod) const                           = 0;
    virtual YAML::Node bin2yaml(const module&, ibinary_reader&) const                 = 0;
    virtual int yaml2bin(const module& mod, const YAML::Node&, ibinary_writer&) const = 0;
    std::optional<name> get_wire_type(const module& mod) const override;
};

struct value_type : wire_type {
    using wire_type::wire_type;

    type_categories category(const module& mod) const override {
        return type_categories::value;
    }
};

struct reference_type : wire_type {
    using wire_type::wire_type;

    type_categories category(const module& mod) const override {
        return type_categories::reference;
    }

    raw_layout wire_layout(const module&) const override {
        return raw_layout{/*.size=*/2,
                          /*.alignment=*/2};
    }
};
} // namespace lidl