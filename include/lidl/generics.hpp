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

struct generic_parameters {
public:
    explicit generic_parameters(
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
static_assert(std::is_move_constructible_v<generic_parameters>);

generic_parameters
    make_generic_declaration(std::vector<std::pair<std::string, std::string>>);

struct module;
class generic_instantiation;
struct generic : public base {
    explicit generic(generic_parameters decl,
                     base* parent                       = nullptr,
                     std::optional<source_info> src_loc = {})
        : base(parent, std::move(src_loc))
        , declaration(std::move(decl)) {
    }

    generic(generic&&) = default;

    virtual type_categories category(const module& mod,
                                     const generic_instantiation& instantiation) const {
        return instantiate(mod, instantiation)->category(mod);
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation& instantiation) const {
        return instantiate(mod, instantiation)->wire_layout(mod);
    }

    virtual YAML::Node bin2yaml(const module& mod,
                                const generic_instantiation& instantiation,
                                ibinary_reader& span) const {

        return instantiate(mod, instantiation)->bin2yaml(mod, span);
    }

    virtual int yaml2bin(const module& mod,
                         const generic_instantiation& instantiation,
                         const YAML::Node& node,
                         ibinary_writer& writer) const {
        return instantiate(mod, instantiation)->yaml2bin(mod, node, writer);
    }

    virtual name get_wire_type(const module& mod,
                               const generic_instantiation& instantiation) const {
        return instantiate(mod, instantiation)->get_wire_type(mod);
    }

    virtual ~generic() = default;

    virtual std::unique_ptr<type> instantiate(const module& mod,
                                              const generic_instantiation& ins) const {
        return nullptr;
    }

    generic_parameters declaration;
};

struct generic_structure : generic {
    explicit generic_structure(generic_parameters decl,
                               base* parent                       = nullptr,
                               std::optional<source_info> src_loc = {})
        : generic(std::move(decl), parent, std::move(src_loc)) {
    }

    std::unique_ptr<type> instantiate(const module& mod,
                                      const generic_instantiation& ins) const override;

    std::unique_ptr<structure> struct_;
};

struct generic_union : generic {
    explicit generic_union(generic_parameters decl,
                           base* parent                       = nullptr,
                           std::optional<source_info> src_loc = {})
        : generic(std::move(decl), parent, std::move(src_loc)), union_{nullptr}  {
    }

    std::unique_ptr<type> instantiate(const module& mod,
                                      const generic_instantiation& ins) const override;

    std::unique_ptr<union_type> union_;
};

class generic_instantiation final : public type {
public:
    explicit generic_instantiation(name n)
        : m_name(std::move(n)) {
        auto base      = get_symbol(m_name.base);
        auto base_type = &dynamic_cast<const generic&>(*base);
        m_actual       = base_type;
    }

    type_categories category(const module& mod) const override {
        return m_actual->category(mod, *this);
    }

    raw_layout wire_layout(const module& mod) const override {
        return m_actual->wire_layout(mod, *this);
    }

    YAML::Node bin2yaml(const module& module, ibinary_reader& span) const override {
        return m_actual->bin2yaml(module, *this, span);
    }

    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return m_actual->yaml2bin(mod, *this, node, writer);
    }

    name get_wire_type(const module& mod) const override {
        return m_actual->get_wire_type(mod, *this);
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

struct pointer_type : generic {
    pointer_type();

    type_categories category(const module& mod,
                             const generic_instantiation& instantiation) const override;

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const override;

    int yaml2bin(const module& mod,
                 const generic_instantiation& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    YAML::Node bin2yaml(const module& mod,
                        const generic_instantiation& instantiation,
                        ibinary_reader& span) const override;
};
} // namespace lidl
