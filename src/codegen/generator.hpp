#pragma once

#include "sections.hpp"

namespace lidl::codegen {
template<class Type>
class generator {
public:
    virtual sections generate() = 0;
    virtual ~generator()        = default;

    generator(const module& mod, const Type& elem)
        : m_module{&mod}
        , m_elem{&elem} {
    }

protected:
    const module& mod() const {
        return *m_module;
    }

    const Type& get() const {
        return *m_elem;
    }

    const Type* symbol() const {
        return m_elem;
    }

private:
    const module* m_module;
    const Type* m_elem;
};
}