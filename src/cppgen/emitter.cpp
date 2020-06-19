#include "emitter.hpp"

#include "lidl/scope.hpp"

#include <fmt/format.h>
#include <iostream>
#include <lidl/module.hpp>
#include <string>
#include <unordered_map>


namespace lidl::cpp {

bool emitter::pass() {
    bool changed = false;

    std::unordered_map<std::string, std::vector<section>> m_this_pass;

    for (int i = 0; i < m_not_generated.size();) {
        auto& sect = m_not_generated[i];
        if (is_satisfied(sect)) {
            changed = true;
            m_this_pass[sect.name_space].push_back(std::move(sect));
            m_not_generated.erase(m_not_generated.begin() + i);
        } else {
            ++i;
        }
    }

    std::cerr << "Pass\n";
    for (auto& [ns, sects] : m_this_pass) {
        if (!ns.empty()) {
            std::cerr << fmt::format("Start namespace {}\n", ns);
            m_stream << fmt::format("namespace {} {{\n", ns);
        }

        for (auto& sect : sects) {
            std::cerr << "Emitting " << sect.key.to_string(*m_module) << '\n';

            m_stream << sect.definition << '\n';

            m_satisfied.emplace_back(sect.key);
            m_generated.emplace_back(std::move(sect));
        }

        if (!ns.empty()) {
            m_stream << fmt::format("}} // {} \n", ns);
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
                if (std::find(m_satisfied.begin(), m_satisfied.end(), dep) !=
                    m_satisfied.end()) {
                    continue;
                }
                std::cerr << " + " << dep.to_string(*m_module) << '\n';
            }
        }
        throw std::runtime_error("Cyclic or unsatisfiable dependency!");
    }
    return m_stream.str();
}

void emitter::mark_module(const module& decl_mod) {
    if (&decl_mod != m_module) {
        for (auto& sym : decl_mod.symbols->all_handles()) {
            std::cerr << fmt::format("Marking {}\n", fmt::join(absolute_name(sym), "::"));
            mark_satisfied({sym, section_type::definition});
        }
    }
    for (auto& [name, child] : decl_mod.children) {
        mark_module(*child);
    }
}

emitter::emitter(const module& root_mod, const module& mod, sections all)
    : m_module{&mod}
    , m_not_generated(std::move(all.m_sections)) {

    mark_module(root_mod);
}
} // namespace lidl::cpp