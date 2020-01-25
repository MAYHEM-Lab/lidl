#pragma once

#include "structure.hpp"
#include "types.hpp"

#include <include/gsl/span>
#include <lidl/layout.hpp>
#include <string>
#include <vector>

namespace lidl {
using generic_argument = std::variant<const symbol*, int64_t>;

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

    [[nodiscard]] int32_t arity() const {
        return m_params.size();
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

std::shared_ptr<const generic_declaration>
    make_generic_declaration(std::vector<std::pair<std::string, std::string>>);

class module;
struct generic {
    explicit generic(std::shared_ptr<const generic_declaration> decl)
        : declaration(std::move(decl)) {
    }

    virtual bool is_raw(const module& mod, const struct generic_instantiation&) const {
        return false;
    }

    virtual bool is_reference(const module& mod, const struct generic_instantiation&) const = 0;

    virtual raw_layout wire_layout(const module& mod,
                                   const struct generic_instantiation&) const = 0;

    virtual ~generic() = 0;

    std::shared_ptr<const generic_declaration> declaration;
};

struct generic_type_parameter : type {
    virtual raw_layout wire_layout(const module& mod) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a generic type parameter!");
    }

    virtual bool is_reference_type(const module&) const override {
        return false;
    }
};

class module;
struct generic_structure {
    std::shared_ptr<const generic_declaration> declaration;
    structure struct_;

    structure instantiate(const module& m, const std::vector<generic_argument>&);
};

struct user_defined_generic : generic {
    explicit user_defined_generic(const generic_structure& str)
        : generic(str.declaration)
        , m_structure(&str) {
    }
    virtual raw_layout wire_layout(const module& mod,
                                   const struct generic_instantiation&) const override {
        throw std::runtime_error("Wire layout shouldn't be called on a generic!");
    }
    bool is_reference(const module& mod,
                      const struct generic_instantiation& instantiation) const override {
        throw std::runtime_error("Is reference shouldn't be called on a generic!");
    }
    const generic_structure* m_structure;
};

class generic_instantiation : public type {
public:
    generic_instantiation(const generic& actual, std::vector<generic_argument> args)
        : m_args(std::move(args))
        , m_actual(&actual) {
    }

    virtual bool is_raw(const module& mod) const override {
        return m_actual->is_raw(mod, *this);
    }

    virtual bool is_reference_type(const module& mod) const override {
        return m_actual->is_reference(mod, *this);
    }

    virtual raw_layout wire_layout(const module& mod) const override {
        return m_actual->wire_layout(mod, *this);
    }

    gsl::span<const generic_argument> arguments() const {
        return m_args;
    }

private:
    std::vector<generic_argument> m_args;
    const generic* m_actual;
};

namespace detail {
struct forward_decl : generic {
    using generic::generic;

    virtual raw_layout wire_layout(const module& mod,
                                   const struct generic_instantiation&) const override {
        throw std::runtime_error(
            "Wire layout shouldn't be called on a forward declaration!");
    }

    bool is_reference(const module& mod,
                      const struct generic_instantiation& instantiation) const override {
        throw std::runtime_error(
            "Is reference shouldn't be called on a forward declaration!");
    }

    bool is_raw(const module& mod,
                const struct generic_instantiation& instantiation) const override {
        throw std::runtime_error(
            "Is raw shouldn't be called on a forward declaration!");
    }
};
} // namespace detail

} // namespace lidl
