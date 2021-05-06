#pragma once

#include "lidl/generics.hpp"
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

struct basic_generic : base {
    explicit basic_generic(base::categories category,
                           generic_parameters decl,
                           base* parent                       = nullptr,
                           std::optional<source_info> src_loc = {})
        : base(category, parent, std::move(src_loc))
        , declaration(std::move(decl)) {
    }

    generic_parameters declaration;

    virtual ~basic_generic() = default;
};

struct generic_type : basic_generic {
    explicit generic_type(generic_parameters decl,
                          base* parent                       = nullptr,
                          std::optional<source_info> src_loc = {})
        : basic_generic(base::categories::generic_type,
                        std::move(decl),
                        parent,
                        std::move(src_loc)) {
    }

    virtual type_categories category(const module& mod,
                                     const name& instantiation) const = 0;
};

struct generic_wire_type : generic_type {
    using generic_type::generic_type;

    virtual raw_layout wire_layout(const module& mod,
                                   const name& instantiation) const = 0;

    virtual YAML::Node bin2yaml(const module& mod,
                                const name& instantiation,
                                ibinary_reader& span) const = 0;

    virtual int yaml2bin(const module& mod,
                         const name& instantiation,
                         const YAML::Node& node,
                         ibinary_writer& writer) const = 0;
};

struct instance_based_wire_type : generic_wire_type {
    using generic_wire_type::generic_wire_type;

    raw_layout wire_layout(const module& mod, const name& instantiation) const override {
        return instantiate(mod, instantiation)->wire_layout(mod);
    }

    YAML::Node bin2yaml(const module& mod,
                        const name& instantiation,
                        ibinary_reader& span) const override {
        return instantiate(mod, instantiation)->bin2yaml(mod, span);
    }

    int yaml2bin(const module& mod,
                 const name& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override {
        return instantiate(mod, instantiation)->yaml2bin(mod, node, writer);
    }

    type_categories category(const module& mod,
                             const name& instantiation) const override {
        return instantiate(mod, instantiation)->category(mod);
    }

    virtual std::unique_ptr<wire_type> instantiate(const module& mod,
                                                   const name& instantiation) const = 0;
};

struct generic_view_type : generic_type {
    using generic_type::generic_type;

    type_categories category(const module& mod,
                             const name& instantiation) const override {
        return type_categories::view;
    }

    virtual std::optional<name> get_wire_type(const module& mod,
                                              const name& instantiation) const = 0;
};

struct instance_based_view_type : generic_view_type {
    using generic_view_type::generic_view_type;

    std::optional<name> get_wire_type(const module& mod,
                                      const name& instantiation) const override {
        return instantiate(mod, instantiation)->get_wire_type(mod);
    }

    virtual std::unique_ptr<view_type> instantiate(const module& mod,
                                                   const name& instantiation) const = 0;
};

struct generic_structure : instance_based_wire_type {
    using instance_based_wire_type::instance_based_wire_type;

    std::unique_ptr<wire_type> instantiate(const module& mod,
                                           const name& ins) const override;

    std::unique_ptr<structure> struct_;
};

struct generic_union : instance_based_wire_type {
    using instance_based_wire_type::instance_based_wire_type;

    std::unique_ptr<wire_type> instantiate(const module& mod,
                                           const name& ins) const override;

    std::unique_ptr<union_type> union_;
};

struct pointer_type : generic_wire_type {
    pointer_type();

    type_categories category(const module& mod, const name& instantiation) const override;

    virtual raw_layout wire_layout(const module& mod, const name&) const override;

    int yaml2bin(const module& mod,
                 const name& instantiation,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;

    YAML::Node bin2yaml(const module& mod,
                        const name& instantiation,
                        ibinary_reader& span) const override;
};
struct basic_generic_instantiation {
    explicit basic_generic_instantiation(name use)
        : args(std::move(use)) {
        generic = dynamic_cast<const basic_generic*>(get_symbol(args.base));
    }

    name args;

    virtual ~basic_generic_instantiation() = default;

    const basic_generic* get_generic() const {
        return this->generic;
    }

protected:
    const basic_generic* generic;
};

struct generic_view_type_instantiation
    : view_type
    , basic_generic_instantiation {
    using basic_generic_instantiation::basic_generic_instantiation;

    const generic_view_type* get_generic() const {
        return static_cast<const generic_view_type*>(this->generic);
    }

    std::optional<name> get_wire_type(const module& mod) const override {
        return get_generic()->get_wire_type(mod, args);
    }
};
struct generic_wire_type_instantiation
    : wire_type
    , basic_generic_instantiation {
    using basic_generic_instantiation::basic_generic_instantiation;

    const generic_wire_type* get_generic() const {
        return static_cast<const generic_wire_type*>(this->generic);
    }

    type_categories category(const module& mod) const override;
    raw_layout wire_layout(const module& mod) const override;
    YAML::Node bin2yaml(const module& module, ibinary_reader& reader) const override;
    int yaml2bin(const module& mod,
                 const YAML::Node& node,
                 ibinary_writer& writer) const override;
};
} // namespace lidl
