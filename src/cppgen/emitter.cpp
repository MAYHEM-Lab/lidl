#include "emitter.hpp"

#include <fmt/format.h>
#include <iostream>
#include <lidl/module.hpp>

namespace lidl::cpp {

bool emitter::pass() {
    bool changed = false;

    for (int i = 0; i < m_not_generated.size();) {
        auto& sect = m_not_generated[i];
        if (is_satisfied(sect)) {
            std::cerr << "Emitting " << sect.key.to_string(*m_module) << '\n';
            if (!sect.name_space.empty()) {
                m_stream << fmt::format("namespace {} {{\n", sect.name_space);
            }
            m_stream << sect.definition << '\n';
            if (!sect.name_space.empty()) {
                m_stream << fmt::format("}}\n", sect.name_space);
            }
            changed = true;
            m_satisfied.emplace_back(sect.key);
            m_generated.emplace_back(std::move(sect));
            m_not_generated.erase(m_not_generated.begin() + i);
        } else {
            ++i;
        }
    }

    return changed;
}
std::string emitter::emit() {
    while (pass())
        ;
    if (!m_not_generated.empty()) {
        std::cerr << "The following dependencies could not be resolved:\n";
        for (auto& sect : m_not_generated) {
            std::cerr << sect.key.to_string(*m_module) << ":\n";
            for (auto& dep : sect.depends_on) {
                std::cerr << " + " << dep.to_string(*m_module) << '\n';
            }
        }
        throw std::runtime_error("Cyclic or unsatisfiable dependency!");
    }
    return m_stream.str();
}

emitter::emitter(const module& mod, sections all)
    : m_module{&mod}
    , m_not_generated(std::move(all.m_sections)) {
    //    mark_satisfied("int8_t_def");
    for (auto mod_ptr = &mod; mod_ptr; mod_ptr = mod_ptr->parent) {
        if (mod_ptr->basic_types.empty()) {
            continue;
        }

        for (auto& t : mod_ptr->basic_types) {
            auto sym = recursive_definition_lookup(*mod_ptr->symbols, t.get()).value();
            mark_satisfied({sym, section_type::definition});
        }

        for (auto& t : mod_ptr->basic_generics) {
            auto sym = recursive_definition_lookup(*mod_ptr->symbols, t.get()).value();
            mark_satisfied({sym, section_type::definition});
        }
    }
}
} // namespace lidl::cpp