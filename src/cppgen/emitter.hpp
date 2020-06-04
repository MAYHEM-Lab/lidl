#pragma once

#include "generator.hpp"

#include <algorithm>
#include <sstream>
#include <string>

namespace lidl::cpp {
struct emitter {
public:
    explicit emitter(sections all);

    std::string emit();

    void mark_satisfied(std::string s) {
        m_satisfied.emplace(std::move(s));
    }

private:
    bool pass();

    bool is_satisfied(const section& sect) {
        return std::all_of(sect.depends_on.begin(), sect.depends_on.end(), [&](auto& n) {
            return m_satisfied.find(n) != m_satisfied.end();
        });
    }

    std::stringstream m_stream;
    std::vector<section> m_not_generated;
    std::vector<section> m_generated;
    std::set<std::string> m_satisfied;
};
} // namespace lidl::cpp