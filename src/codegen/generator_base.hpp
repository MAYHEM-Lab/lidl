#pragma once

#include "generator.hpp"

#include <lidl/module.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace lidl::codegen {
template<class Type>
class generator_base : public codegen::generator<Type> {
public:
    generator_base(const module& mod,
                   std::string_view name,
                   std::string_view ctor_name,
                   std::string_view abs_name,
                   const Type& elem)
        : codegen::generator<Type>(mod, elem)
        , m_name{name}
        , m_ctor_name(ctor_name)
        , m_abs_name(abs_name) {
    }

    generator_base(const module& mod,
                   std::string_view name,
                   std::string_view abs_name,
                   const Type& elem)
        : generator_base(mod, name, name, abs_name, elem) {
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
        return section_key_t{this->symbol(), section_type::definition};
    }

private:
    std::string m_name;
    std::string m_ctor_name;
    std::string m_abs_name;
};
} // namespace lidl::cpp