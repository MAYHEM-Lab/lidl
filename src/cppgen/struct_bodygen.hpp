#pragma once

#include "generator_base.hpp"
#include "emitter.hpp"
#include <lidl/structure.hpp>
#include "cppgen.hpp"

namespace lidl::cpp {
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
    struct_body_gen(const module& mod, std::string_view name, const structure& str)
        : m_module{&mod}
        , m_name{name}
        , m_struct{&str} {
    }

    void generate(std::ostream& stream);

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
    std::string_view m_name;
    const structure* m_struct;
};
}