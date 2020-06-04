#pragma once

#include <string>
#include <set>
#include <vector>

namespace lidl::cpp {
enum class section_type {
    definition,
    declaration,
    misc
};

struct section_key_t {
    name entity_name;
    section_type type;
};

struct section {
    std::string name_space;
    section_key_t key;

    std::string definition;

    std::vector<section_key_t> depends_on;

    void add_dependency(section_key_t key) {
        depends_on.push_back(std::move(key));
    }
};

struct sections {
    std::vector<section> m_sections;

    void add(section sect) {
        m_sections.emplace_back(std::move(sect));
    }

    void merge_before(sections rhs) {
        for (auto& s : rhs.m_sections) {
            m_sections.emplace_back(std::move(s));
        }
    }
};

class generator {
public:
    virtual sections generate() = 0;
};
}