#pragma once

#include "generator.hpp"

#include <lidl/module.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace lidl::cpp {
using codegen::section;
using codegen::section_key_t;
using codegen::section_type;

template<class Type>
class generator_base : public codegen::generator<Type> {
public:
    generator_base(const module& mod,
                   symbol_handle sym,
                   std::string_view name,
                   std::string_view ctor_name,
                   std::string_view abs_name,
                   const Type& elem)
        : codegen::generator<Type>(std::move(sym), mod, elem)
        , m_name{name}
        , m_ctor_name(ctor_name)
        , m_abs_name(abs_name) {
    }

    generator_base(const module& mod,
                   symbol_handle sym,
                   std::string_view name,
                   std::string_view abs_name,
                   const Type& elem)
        : generator_base(mod, std::move(sym), name, name, abs_name, elem) {
    }

protected:
    std::string_view name() const {
        return m_name;
    }

    std::string_view ctor_name() const {
        return m_ctor_name;
    }

    std::string_view absolute_name() const {
        return m_abs_name;
    }

    section_key_t def_key() {
        if (this->symbol().is_valid()) {
            return section_key_t{this->symbol(), section_type::definition};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::definition};
        }
    }

    section_key_t decl_key() {
        if (this->symbol().is_valid()) {
            return section_key_t{this->symbol(), section_type::declaration};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::declaration};
        }
    }

    section_key_t misc_key() {
        if (this->symbol().is_valid()) {
            return section_key_t{this->symbol(), section_type::misc};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::misc};
        }
    }

private:
    std::string m_name;
    std::string m_ctor_name;
    std::string m_abs_name;
};
} // namespace lidl::cpp