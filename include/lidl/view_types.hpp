#pragma once

#include <lidl/basic.hpp>
#include <lidl/generics.hpp>
#include <lidl/types.hpp>

namespace lidl {
struct view_type : type {
    explicit view_type(const name& wire_type)
        : m_wire_type{wire_type} {
    }

    type_categories category(const module& mod) const override {
        return type_categories::view;
    }

    raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error("View types do not have wire layouts");
    }

    YAML::Node bin2yaml(const module& module, ibinary_reader& span) const override {
        throw std::runtime_error("View type");
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        throw std::runtime_error("View type");
    }

    name get_wire_type(const module& mod) const {
        return m_wire_type;
    }

private:
    name m_wire_type;
};

struct span_type : generic {
    span_type()
        : generic(make_generic_declaration({{"T", "type"}})) {
    }

    std::unique_ptr<type> instantiate(const module& mod,
                                      const generic_instantiation& ins) const override {
        return std::make_unique<view_type>(name{
            recursive_name_lookup(*mod.symbols, "vector").value(), ins.get_name().args});
    }
};
} // namespace lidl