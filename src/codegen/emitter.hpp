#pragma once

#include "generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <sections.hpp>

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

    bool is_satisfied(const section& sect) {
        return std::all_of(sect.depends_on.begin(), sect.depends_on.end(), [&](auto& n) {
            return std::find(m_satisfied.begin(), m_satisfied.end(), n) !=
                   m_satisfied.end();
            //            return m_satisfied.find(n) != m_satisfied.end();
        });
    }

    const module* m_module;
    std::stringstream m_stream;
    std::vector<section> m_not_generated;
    std::vector<section> m_generated;
    std::vector<section_key_t> m_satisfied;
};
} // namespace lidl::cpp