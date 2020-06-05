#pragma once

#include "cppgen.hpp"
#include "emitter.hpp"
#include "generator_base.hpp"

#include <lidl/structure.hpp>

namespace lidl::cpp {
/**
 * Raw struct generator is used to generate C++ structs that contain only the data
 * members.
 *
 * While it works, it's very low level. For instance, for strings, you'll only have a
 * pointer rather than something high level. It won't have constructors either.
 *
 * Therefore, the output of this generator is wrapped with something that supplies
 * accessors.
 */
class raw_struct_gen : generator_base<structure> {
public:
    using generator_base::generator_base;

    sections generate() override;

    static void
    generate_field(std::string_view name, std::string_view type_name, std::ostream& str) {
        str << fmt::format("{} {};\n", type_name, name);
    }

private:
    void generate_raw_constructor();
    struct {
        std::stringstream pub, ctor;
    } raw_sections;
};

class struct_body_gen {
public:
    struct_body_gen(const module& mod,
                    const symbol_handle& sym,
                    std::string_view name,
                    const structure& str)
        : m_module{&mod}
        , m_symbol{sym}
        , m_name{name}
        , m_struct{&str} {
    }

    sections generate();

private:
    void generate_constructor();

    void generate_struct_field(std::string_view name, const member& mem) {
        m_sections.pub << generate_getter(name, mem, true) << '\n';
        m_sections.pub << generate_getter(name, mem, false) << '\n';
    }

    std::string
    generate_getter(std::string_view member_name, const member& mem, bool is_const);

    const module& mod() {
        return *m_module;
    }

    const structure& str() {
        return *m_struct;
    }

    struct {
        std::stringstream pub, priv, ctor;
        std::stringstream traits;
    } m_sections;

    const module* m_module;
    symbol_handle m_symbol;
    std::string_view m_name;
    const structure* m_struct;
};
} // namespace lidl::cpp