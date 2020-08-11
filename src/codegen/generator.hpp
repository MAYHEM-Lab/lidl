#pragma once

#include "sections.hpp"

namespace lidl::codegen {
template<class Type>
class generator {
public:
    virtual sections generate() = 0;
    virtual ~generator()        = default;

    generator(symbol_handle sym, const module& mod, const Type& elem)
        : m_symbol{sym}
        , m_module{&mod}
        , m_elem{&elem} {
    }

protected:
    const module& mod() const {
        return *m_module;
    }

    const Type& get() const {
        return *m_elem;
    }

    const symbol_handle& symbol() const {
        return m_symbol;
    }

private:
    symbol_handle m_symbol;
    const module* m_module;
    const Type* m_elem;
};
}