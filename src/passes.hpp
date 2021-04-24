#pragma once

#include <lidl/module.hpp>

namespace lidl {
using pass = bool (*)(module&);

bool reference_type_pass(module& m);
bool service_proc_pass(module& mod);
bool service_pass(module& m);

inline bool run_all_passes(module& m) {
    constexpr std::pair<std::string_view, pass> passes[] = {
        {"Service procedure pass", &service_proc_pass},
        {"Service pass", &service_pass},
        {"Reference type pass", &reference_type_pass},
    };

    bool changed = false;
    for (auto& [name, pass] : passes) {
        std::cerr << "Running pass " << name << '\n';
        try {
            changed |= pass(m);
        } catch (std::exception& err) {
            std::cerr << "Exception while running pass " << name << '\n';
            throw;
        }
    }
    return changed;
}

inline void run_passes_until_stable(module& m) {
    while (run_all_passes(m))
        ;
}
} // namespace lidl