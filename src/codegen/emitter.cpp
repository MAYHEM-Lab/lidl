#include "emitter.hpp"

#include "lidl/scope.hpp"
#include "sections.hpp"

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
            m_this_pass[sect.name_space()].push_back(std::move(sect));
            m_not_generated.erase(m_not_generated.begin() + i);
        } else {
            ++i;
        }
    }

#ifdef LIDL_VERBOSE_LOG
    std::cerr << "Pass\n";
#endif
    for (auto& [ns, sects] : m_this_pass) {
        if (!ns.empty()) {
#ifdef LIDL_VERBOSE_LOG
            std::cerr << fmt::format("  Namespace {}\n", ns);
#endif
            m_stream << fmt::format("namespace {} {{\n", ns);
        }

        for (auto& sect : sects) {
            m_stream << sect.definition << '\n';

            for (auto& key : sect.keys()) {
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
            std::vector<std::string> key_names(sect.keys().size());
            std::transform(sect.keys().begin(),
                           sect.keys().end(),
                           key_names.begin(),
                           [&](auto& key) { return key.to_string(*m_module); });
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
        for (auto& sym_handle : decl_mod.symbols().all_handles()) {
            auto sym = get_symbol(sym_handle);
            if (dynamic_cast<const scope*>(sym) || dynamic_cast<const module*>(sym) ||
                sym == &forward_decl) {
                continue;
            }

            auto key = section_key_t{sym,
                                     sym->is_generic() ? section_type::generic_declaration
                                                       : section_type::definition};
#ifdef LIDL_VERBOSE_LOG
            std::cerr << fmt::format("Marking {}\n", key.to_string(decl_mod));
#endif
            mark_satisfied(key);

            key = section_key_t{sym, section_type::lidl_traits};
#ifdef LIDL_VERBOSE_LOG
            std::cerr << fmt::format("Marking {}\n", key.to_string(decl_mod));
#endif
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

bool emitter::is_satisfied(const section& sect) {
    return std::all_of(
        sect.depends_on.begin(), sect.depends_on.end(), [&](const section_key_t& n) {
            if (std::find(m_satisfied.begin(), m_satisfied.end(), n) !=
                m_satisfied.end()) {
                return true;
            }
            if (auto mod = find_parent_module(n.symbol()); mod && mod->imported()) {
                return true;
            }
            return false;
        });
}
} // namespace lidl::codegen