#pragma once

#include <lidl/basic.hpp>
#include <set>
#include <string>
#include <vector>
#include <variant>

namespace lidl::cpp {
enum class section_type
{
    definition,
    declaration,
    misc
};

struct section_key_t {
    std::variant<std::monostate, std::string, symbol_handle> symbol;
    section_type type;

    std::string to_string(const module& mod);

    section_key_t()
        : type{section_type::misc} {
    }

    section_key_t(std::string sym, section_type t = section_type::misc)
        : symbol{sym}
        , type{t} {
    }

    section_key_t(const symbol_handle& sym, section_type t = section_type::misc)
        : symbol{sym}
        , type{t} {
    }

    friend bool operator==(const section_key_t& left, const section_key_t& right) {
        return left.type == right.type && left.symbol == right.symbol;
    }
};

inline std::vector<section_key_t> def_keys_from_name(const module& mod, const name& nm) {
    std::vector<section_key_t> all;
    all.emplace_back(nm.base, section_type::definition);
    for (auto& arg : nm.args) {
        if (auto n = std::get_if<name>(&arg.get_variant()); n) {
            auto sub = def_keys_from_name(mod, *n);
            all.insert(all.end(), sub.begin(), sub.end());
        }
    }
    return all;
}

struct section {
    /**
     * If this member is not empty, it specifies the namespace in which the definition
     * will be emitted in.
     */
    std::string name_space;

    /**
     * This member is used as the key for the definition in dependency resolution.
     */
    section_key_t key;
    std::vector<section_key_t> keys;

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
} // namespace lidl::cpp