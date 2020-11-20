#pragma once

#include <lidl/basic.hpp>
#include <lidl/module.hpp>
#include <string>
#include <vector>

namespace lidl::js {
struct section {
    std::string body;
};

struct sections {
    std::vector<section> sects;

    void merge_before(sections&& other) {
        sects.insert(sects.end(),
                     std::make_move_iterator(other.sects.begin()),
                     std::make_move_iterator(other.sects.end()));
    }
};

class generator_base {
public:
    virtual sections generate() = 0;
    virtual ~generator_base()   = default;
};

template<class T>
class generator : public generator_base {
public:
    generator(const module& mod,
              const symbol_handle& sym,
              std::string_view name,
              const T& t)
        : m_obj{&t}
        , m_mod{&mod}
        , m_symbol(sym)
        , m_name{name} {
    }

protected:
    std::string_view name() const {
        return m_name;
    }

    const T& get() const {
        return *m_obj;
    }

    const module& mod() const {
        return *m_mod;
    }

private:
    const module* m_mod;
    const T* m_obj;
    symbol_handle m_symbol;
    std::string_view m_name;
};
} // namespace lidl::js