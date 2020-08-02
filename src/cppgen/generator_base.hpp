#pragma once

#include "generator.hpp"

#include <lidl/module.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace lidl::cpp {
template<class Type>
class generator_base : public generator {
public:
    generator_base(const module& mod,
                   symbol_handle sym,
                   std::string_view name,
                   std::string_view ctor_name,
                   std::string_view abs_name,
                   const Type& elem)
        : m_module{&mod}
        , m_symbol(std::move(sym))
        , m_name{name}
        , m_ctor_name(ctor_name)
        , m_abs_name(abs_name)
        , m_elem{&elem} {
    }

    generator_base(const module& mod,
                   symbol_handle sym,
                   std::string_view name,
                   std::string_view abs_name,
                   const Type& elem)
        : generator_base(mod, std::move(sym), name, name, abs_name, elem) {
    }

protected:
    const module& mod() const {
        return *m_module;
    }

    const Type& get() const {
        return *m_elem;
    }

    std::string_view name() const {
        return m_name;
    }

    std::string_view ctor_name() const {
        return m_ctor_name;
    }

    std::string_view absolute_name() const {
        return m_abs_name;
    }

    const symbol_handle& symbol() const {
        return m_symbol;
    }

    section_key_t def_key() {
        if (symbol().is_valid()) {
            return section_key_t{symbol(), section_type::definition};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::definition};
        }
    }

    section_key_t decl_key() {
        if (symbol().is_valid()) {
            return section_key_t{symbol(), section_type::declaration};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::declaration};
        }
    }

    section_key_t misc_key() {
        if (symbol().is_valid()) {
            return section_key_t{symbol(), section_type::misc};
        } else {
            return section_key_t{std::string(absolute_name()), section_type::misc};
        }
    }

private:
    symbol_handle m_symbol;
    const module* m_module;
    std::string m_name;
    std::string m_ctor_name;
    std::string m_abs_name;
    const Type* m_elem;
};
} // namespace lidl::cpp