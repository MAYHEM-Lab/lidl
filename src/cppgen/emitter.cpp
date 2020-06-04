#include "emitter.hpp"

#include <fmt/format.h>
#include <iostream>

namespace lidl::cpp {

bool emitter::pass() {
    bool changed = false;

    for (int i = 0; i < m_not_generated.size();) {
        auto& sect = m_not_generated[i];
        if (is_satisfied(sect)) {
            if (!sect.name_space.empty()) {
                m_stream << fmt::format("namespace {} {{\n", sect.name_space);
            }
            m_stream << sect.definition << '\n';
            if (!sect.name_space.empty()) {
                m_stream << fmt::format("}}\n", sect.name_space);
            }
            changed = true;
            m_generated.emplace_back(std::move(sect));
            m_not_generated.erase(m_not_generated.begin() + i);
            m_satisfied.emplace(sect.name);
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
            std::cerr << sect.name << ":\n";
            for (auto& dep : sect.depends_on) {
                std::cerr << " + " << dep << '\n';
            }
        }
        throw std::runtime_error("Cyclic or unsatisfiable dependency!");
    }
    return m_stream.str();
}

emitter::emitter(sections all)
    : m_not_generated(std::move(all.m_sections)) {
    mark_satisfied("int8_t_def");
}
}