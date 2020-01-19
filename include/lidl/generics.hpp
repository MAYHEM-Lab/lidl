#pragma once

#include "structure.hpp"
#include "types.hpp"

#include <include/gsl/span>
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
struct generic_type {
    explicit generic_type(std::shared_ptr<const generic_declaration> decl)
        : declaration(std::move(decl)) {
    }

    virtual bool is_raw(const module& mod, const struct generic_instantiation&) const {
        return false;
    }

    virtual ~generic_type() = 0;

    std::shared_ptr<const generic_declaration> declaration;
};

struct generic_type_parameter : type {
};

class module;
struct generic_structure {
    std::shared_ptr<const generic_declaration> declaration;
    structure struct_;

    structure instantiate(const module& m, const std::vector<std::string>&);
};

namespace detail {
std::string make_name_for_instantiation(std::string_view name,
                                        const std::vector<std::string>& args);
} // namespace detail

class generic_instantiation : public type {
public:
    generic_instantiation(const generic_type& actual,
                          std::vector<generic_argument> args)
        : m_args(std::move(args))
        , m_actual(&actual) {
    }

    virtual bool is_raw(const module& mod) const override {
        return m_actual->is_raw(mod, *this);
    }

    gsl::span<const generic_argument> arguments() const {
        return m_args;
    }

private:
    std::vector<generic_argument> m_args;
    const generic_type* m_actual;
};

namespace detail {
struct forward_decl : generic_type {
    using generic_type::generic_type;
};
} // namespace detail

struct user_defined_generic : generic_type {
    explicit user_defined_generic(const generic_structure& str)
        : generic_type(str.declaration)
        , m_structure(&str) {
    }
    const generic_structure* m_structure;
};
} // namespace lidl
