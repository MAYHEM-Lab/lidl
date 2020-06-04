#pragma once

#include <vector>
#include <string>
#include <string_view>
#include "generator.hpp"
#include <lidl/module.hpp>

namespace lidl::cpp {
template<class Type>
class generator_base : public generator {
public:
    generator_base(const module& mod,
                   std::string_view name,
                   std::string_view ctor_name,
                   const Type& elem)
        : m_module{&mod}
        , m_name{name}
        , m_ctor_name(ctor_name)
        , m_elem{&elem} {
    }

    generator_base(const module& mod, std::string_view name, const Type& elem)
        : generator_base(mod, name, name, elem) {
    }

protected:
    const module& mod() {
        return *m_module;
    }

    const Type& get() {
        return *m_elem;
    }

    std::string_view name() {
        return m_name;
    }

    std::string_view ctor_name() {
        return m_ctor_name;
    }

private:
    const module* m_module;
    std::string m_name;
    std::string m_ctor_name;
    const Type* m_elem;
};
} // namespace lidl::cpp