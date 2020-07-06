#pragma once

#include <lidl/module.hpp>

namespace lidl {
using pass = bool (*)(module&);

bool union_enum_pass(module& m);
bool reference_type_pass(module& m);
bool service_pass(module& m);

inline bool run_all_passes(module& m) {
    pass passes[] = {&union_enum_pass, &reference_type_pass, &service_pass};

    bool changed = false;
    for (auto& pass : passes) {
        changed |= pass(m);
    }
    return changed;
}

inline void run_passes_until_stable(module& m) {
    while (run_all_passes(m));
}
} // namespace lidl