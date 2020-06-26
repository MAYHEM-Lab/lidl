#pragma once

#include <lidl/basic.hpp>
#include <lidl/types.hpp>
#include <lidl/generics.hpp>

namespace lidl {
struct view_type : type {
    explicit view_type(const name& wire_type) : m_wire_type{wire_type} {}

    raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error("View types do not have wire layouts");
    }

    bool is_reference_type(const module& mod) const override {
        return false;
    }

    YAML::Node bin2yaml(const module& module,
                                           ibinary_reader& span) const override {
        throw std::runtime_error("View type");
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        throw std::runtime_error("View type");
    }

    name get_wire_type() const {
        return m_wire_type;
    }

private:
    name m_wire_type;
};
}