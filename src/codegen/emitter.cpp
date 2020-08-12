#include "emitter.hpp"

#include "lidl/scope.hpp"

#include <fmt/format.h>
#include <iostream>
#include <lidl/module.hpp>
#include <string>
#include <unordered_map>


namespace lidl::codegen {
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
            std::cerr << fmt::format("  Namespace {}\n", ns);
            m_stream << fmt::format("namespace {} {{\n", ns);
        }

        for (auto& sect : sects) {
            std::vector<std::string> key_names(sect.keys.size());
            std::transform(sect.keys.begin(), sect.keys.end(), key_names.begin(), [&](auto& key) {
              return key.to_string(*m_module);
            });
            std::cerr << "    Emitting " << fmt::format("{}\n", fmt::join(key_names, "\n             "));

            m_stream << sect.definition << '\n';

            for (auto& key : sect.keys) {
                m_satisfied.emplace_back(key);
            }
            m_generated.emplace_back(std::move(sect));
        }

        if (!ns.empty()) {
            m_stream << fmt::format("}} // namespace {} \n", ns);
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
            std::vector<std::string> key_names(sect.keys.size());
            std::transform(sect.keys.begin(), sect.keys.end(), key_names.begin(), [&](auto& key) {
                return key.to_string(*m_module);
            });
            std::cerr << fmt::format("{}:\n", fmt::join(key_names, "\n"));
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
        for (auto& sym_handle : decl_mod.symbols->all_handles()) {
            auto sym = get_symbol(sym_handle);
            if (std::get_if<const scope*>(&sym) || std::get_if<forward_decl>(&sym)) {
                continue;
            }

            auto key = section_key_t{sym_handle, section_type::definition};
            std::cerr << fmt::format("Marking {}\n", key.to_string(decl_mod));
            mark_satisfied(key);
        }
    }
    for (auto& [name, child] : decl_mod.children) {
        mark_module(*child);
    }
}

emitter::emitter(const module& root_mod, const module& mod, sections all)
    : m_module{&mod}
    , m_not_generated(std::move(all.get_sections())) {

    mark_module(root_mod);
}
} // namespace lidl::cpp