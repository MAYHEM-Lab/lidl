#pragma once

#include "structure.hpp"
#include "types.hpp"

#include <gsl/span>
#include <lidl/basic.hpp>
#include <lidl/layout.hpp>
#include <lidl/scope.hpp>
#include <lidl/union.hpp>
#include <string>
#include <vector>

namespace lidl {
struct generic_parameter {
    virtual ~generic_parameter() = default;
};

struct type_parameter : generic_parameter {};

struct value_parameter : generic_parameter {
    const type* m_type;
};

std::unique_ptr<generic_parameter> get_generic_parameter_for_type(std::string_view type);

struct generic_declaration {
public:
    explicit generic_declaration(
        std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>>&& params)
        : m_params{std::move(params)} {
    }

    [[nodiscard]] int32_t arity() const {
        return static_cast<int32_t>(m_params.size());
    }

    [[nodiscard]] auto begin() {
        return m_params.begin();
    }

    [[nodiscard]] auto end() {
        return m_params.end();
    }

    [[nodiscard]] auto begin() const {
        return m_params.begin();
    }

    [[nodiscard]] auto end() const {
        return m_params.end();
    }

private:
    std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>> m_params;
};
static_assert(std::is_move_constructible_v<generic_declaration>);

generic_declaration
    make_generic_declaration(std::vector<std::pair<std::string, std::string>>);

struct module;
class generic_instantiation;
struct generic {
    explicit generic(generic_declaration decl)
        : declaration(std::move(decl)) {
    }

    generic(generic&&) = default;

    virtual bool is_reference(const module& mod, const generic_instantiation&) const = 0;

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const = 0;

    virtual std::pair<YAML::Node, size_t> bin2yaml(const module&,
                                                   const generic_instantiation&,
                                                   gsl::span<const uint8_t>) const = 0;

    virtual int yaml2bin(const module& mod,
                         const generic_instantiation&,
                         const YAML::Node&,
                         ibinary_writer&) const = 0;

    virtual ~generic() = default;

    generic_declaration declaration;
};

struct generic_structure : generic {
    explicit generic_structure(generic_declaration decl, structure s)
        : generic(std::move(decl))
        , struct_(static_cast<structure&&>(s)) {
    }

    virtual raw_layout
    wire_layout(const module& mod,
                const generic_instantiation& instantiation) const override {
        return instantiate(mod, instantiation).wire_layout(mod);
    }

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        return instantiate(mod, instantiation).is_reference_type(mod);
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override {
        return instantiate(mod, instantiation).bin2yaml(mod, span);
    }

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return instantiate(mod, instantiation).yaml2bin(mod, node, writer);
    }

    structure instantiate(const module& mod, const generic_instantiation& ins) const;

    structure struct_;
};

struct generic_union : generic {
    explicit generic_union(generic_declaration decl, union_type s)
        : generic(std::move(decl))
        , union_(std::move(s)) {
    }

    raw_layout wire_layout(const module& mod,
                           const generic_instantiation& instantiation) const override {
        return instantiate(mod, instantiation).wire_layout(mod);
    }

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        return instantiate(mod, instantiation).is_reference_type(mod);
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override {
        return instantiate(mod, instantiation).bin2yaml(mod, span);
    }

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    union_type instantiate(const module& mod, const generic_instantiation& ins) const;

    union_type union_;
};

class generic_instantiation : public type {
public:
    explicit generic_instantiation(name n)
        : m_name(std::move(n)) {
        auto base = get_symbol(m_name.base);
        auto base_type = std::get<const generic*>(base);
        m_actual = base_type;
    }

    bool is_reference_type(const module& mod) const override {
        return m_actual->is_reference(mod, *this);
    }

    raw_layout wire_layout(const module& mod) const override {
        return m_actual->wire_layout(mod, *this);
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override {
        return m_actual->bin2yaml(module, *this, span);
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return m_actual->yaml2bin(mod, *this, node, writer);
    }

    gsl::span<const generic_argument> arguments() const {
        return m_name.args;
    }

    gsl::span<generic_argument> arguments() {
        return m_name.args;
    }

    const generic& generic_type() const {
        return *m_actual;
    }

    const name& get_name() const {
        return m_name;
    }

private:
    name m_name;
    const generic* m_actual;
};

namespace detail {
struct forward_decl : generic {
    using generic::generic;

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a forward declaration!");
    }

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        throw std::runtime_error(
            "Is reference shouldn't be called on a forward declaration!");
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override {
        throw std::runtime_error(
            "bin2yaml shouldn't be called on a forward declaration!");
    }

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        throw std::runtime_error(
            "yaml2bin shouldn't be called on a forward declaration!");
    }
};
} // namespace detail

struct pointer_type : generic {
    pointer_type();

    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        return true;
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const override;

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    std::pair<YAML::Node, size_t> bin2yaml(const module& mod,
                                           const generic_instantiation& instantiation,
                                           gsl::span<const uint8_t> span) const override;
};
} // namespace lidl
