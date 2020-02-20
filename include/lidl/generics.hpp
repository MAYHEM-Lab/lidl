#pragma once

#include "structure.hpp"
#include "types.hpp"

#include <gsl/span>
#include <lidl/basic.hpp>
#include <lidl/layout.hpp>
#include <lidl/scope.hpp>
#include <string>
#include <vector>


namespace lidl {
struct generic_parameter {
    virtual ~generic_parameter() = default;
};

std::unique_ptr<generic_parameter> get_generic_parameter_for_type(std::string_view type);

struct generic_declaration {
public:
    explicit generic_declaration(
        std::vector<std::pair<std::string, std::unique_ptr<generic_parameter>>>&& params)
        : m_params{std::move(params)} {
    }

    generic_declaration(const generic_declaration&) = delete;
    generic_declaration(generic_declaration&&) noexcept = default;
    generic_declaration& operator=(generic_declaration&&) noexcept = default;

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

generic_declaration
    make_generic_declaration(std::vector<std::pair<std::string, std::string>>);

struct module;
class generic_instantiation;
struct generic {
    explicit generic(generic_declaration decl)
        : declaration(std::move(decl)) {
    }

    generic(generic&&) noexcept = default;
    generic& operator=(generic&&) noexcept = default;

    virtual bool is_reference(const module& mod,
                              const generic_instantiation&) const = 0;

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const = 0;

    virtual std::pair<YAML::Node, size_t> bin2yaml(const module&,
                                                   const generic_instantiation&,
                                                   gsl::span<const uint8_t>) const = 0;

    virtual ~generic() = 0;

    generic_declaration declaration;
};

struct generic_type_parameter : type {
    virtual raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a generic type parameter!");
    }

    virtual bool is_reference_type(const module&) const override {
        throw std::runtime_error(
            "Is reference type shouldn't be called on a generic type parameter!");
    }
};

struct generic_structure : generic {
    explicit generic_structure(generic_declaration decl, structure s)
        : generic(std::move(decl))
        , struct_(static_cast<structure&&>(s)) {
    }

    virtual raw_layout wire_layout(const module& mod,
                                   const generic_instantiation&) const override {
        throw std::runtime_error("Wire layout shouldn't be called on a generic!");
    }
    bool is_reference(const module& mod,
                      const generic_instantiation& instantiation) const override {
        throw std::runtime_error("Is reference shouldn't be called on a generic!");
    }
    std::pair<YAML::Node, size_t>
    bin2yaml(const module& module,
             const generic_instantiation& instantiation,
             gsl::span<const uint8_t> span) const override {
        throw std::runtime_error("bin2yaml shouldn't be called on a generic!");
    }

    structure struct_;
};

class generic_instantiation : public type {
public:
    generic_instantiation(const generic& actual, std::vector<generic_argument> args)
        : m_args(std::move(args))
        , m_actual(&actual) {
    }

    virtual bool is_reference_type(const module& mod) const override {
        return m_actual->is_reference(mod, *this);
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        return m_actual->wire_layout(mod, *this);
    }

    std::pair<YAML::Node, size_t> bin2yaml(const module& module,
                                           gsl::span<const uint8_t> span) const override {
        return m_actual->bin2yaml(module, *this, span);
    }

    gsl::span<const generic_argument> arguments() const {
        return m_args;
    }

    gsl::span<generic_argument> arguments() {
        return m_args;
    }

    const generic& generic_type() const {
        return *m_actual;
    }

private:
    std::vector<generic_argument> m_args;
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

    std::pair<YAML::Node, size_t>
    bin2yaml(const module& module,
             const generic_instantiation& instantiation,
             gsl::span<const uint8_t> span) const override {
        throw std::runtime_error(
            "bin2yaml shouldn't be called on a forward declaration!");
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

    std::pair<YAML::Node, size_t>
    bin2yaml(const module& module,
             const generic_instantiation& instantiation,
             gsl::span<const uint8_t> span) const override;
};
} // namespace lidl
