#pragma once

#include "generator.hpp"

#include <algorithm>
#include <sections.hpp>
#include <sstream>
#include <string>

namespace lidl::codegen {
struct emitter {
public:
    explicit emitter(const module& root, const module& mod, sections all);

    std::string emit();

    void mark_satisfied(section_key_t s) {
        m_satisfied.emplace_back(std::move(s));
    }

private:
    void mark_module(const module& decl_mod);

    bool pass();

    bool is_defined(const section& sect) {
        return std::all_of(sect.keys().begin(), sect.keys().end(), [&](auto& n) {
            return std::find(m_satisfied.begin(), m_satisfied.end(), n) !=
                   m_satisfied.end();
        });
    }

    bool is_satisfied(const section& sect);

    const module* m_module;
    std::stringstream m_stream;
    std::vector<section> m_not_generated;
    std::vector<section> m_generated;
    std::vector<section_key_t> m_satisfied;
};
} // namespace lidl::codegen