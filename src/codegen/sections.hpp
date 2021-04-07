#pragma once

#include <lidl/scope.hpp>
#include <string>
#include <variant>

namespace lidl::codegen {
enum class section_type
{
    definition,

    wire_types,
    service_params_union,
    service_return_union,
    service_descriptor,
    sync_server,
    async_server,
    stub,
    async_stub,
    zerocopy_stub,
    async_zerocopy_stub,

    generic_declaration,

    lidl_traits,
    std_traits,

    eq_operator,
    validator
};

using section_entity_t = const base*;
struct section_key_t {
    std::string to_string(const module& mod);

    section_key_t(const base* sym, section_type t)
        : symbol{sym}
        , type{t} {
    }

    section_key_t(symbol_handle handle, section_type t)
            : symbol{get_symbol(handle)}
            , type{t} {
    }
    friend bool operator==(const section_key_t& left, const section_key_t& right) {
        return left.type == right.type && left.symbol == right.symbol;
    }

    section_type type;

private:
    section_entity_t symbol;
};

// Computes the list of section keys that are depended on by the given name.
inline std::vector<section_key_t> def_keys_from_name(const module& mod, const name& nm) {
    std::vector<section_key_t> all;
    all.emplace_back(nm.base, nm.args.empty() ? section_type::definition : section_type::generic_declaration);
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
    std::vector<section_key_t> keys;

    void add_key(section_key_t key) {
        keys.push_back(std::move(key));
    }

    std::string definition;

    std::vector<section_key_t> depends_on;

    void add_dependency(section_key_t key) {
        depends_on.push_back(std::move(key));
    }
};

struct sections {
    sections() = default;
    sections(std::vector<section> sects)
        : m_sections{std::move(sects)} {
    }

    void add(section sect) {
        m_sections.emplace_back(std::move(sect));
    }

    void merge_before(sections rhs) {
        for (auto& s : rhs.m_sections) {
            m_sections.emplace_back(std::move(s));
        }
    }

    std::vector<section>& get_sections() {
        return m_sections;
    }

private:
    std::vector<section> m_sections;
};
} // namespace lidl::codegen