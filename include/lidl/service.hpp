//
// Created by fatih on 1/24/20.
//

#pragma once

#include "scope.hpp"

#include <deque>
#include <lidl/types.hpp>
#include <lidl/union.hpp>
#include <cassert>

namespace lidl {
enum class param_flags {
    in = 1,
    out = 2,
    in_out = 3
};

struct parameter {
    lidl::name type;
    param_flags flags = param_flags::in;
    std::optional<source_info> src_info;
};

struct procedure {
    std::deque<name> return_types;
    std::deque<std::pair<std::string, parameter>> parameters;
    const structure* params_struct = nullptr;
    const structure* results_struct = nullptr;
    name params_struct_name;
    name results_struct_name;

    std::optional<source_info> src_info;
};

struct property : member {
    using member::member;
};

struct service {
    std::optional<name> extends;
    std::deque<std::pair<std::string, property>> properties;
    std::deque<std::pair<std::string, procedure>> procedures;
    const union_type* procedure_params_union = nullptr;
    const union_type* procedure_results_union = nullptr;

    std::vector<std::pair<std::string_view, const procedure*>> all_procedures() const {
        std::vector<std::pair<std::string_view, const procedure*>> res;
        for (auto& s : inheritance_list()) {
            for (auto& [name, proc] : s->procedures) {
                res.emplace_back(name, &proc);
            }
        }
        return res;
    }

    std::optional<int> proc_index(const procedure& proc) const {
        auto all_proc = all_procedures();
        auto iter = std::find_if(all_proc.begin(), all_proc.end(), [&proc](auto& pair) {
            return pair.second == &proc;
        });
        if (iter == all_proc.end()) {
            return {};
        }
        return std::distance(all_proc.begin(), iter);
    }

    std::vector<const service*> inheritance_list() const {
        std::vector<const service*> inheritance;

        for (const service* s = this; s;) {
            inheritance.emplace_back(s);

            if (s->extends) {
                assert(s->extends->args.empty());

                auto base_sym = get_symbol(s->extends->base);

                s = std::get<const lidl::service*>(base_sym);
            } else {
                s = nullptr;
            }
        }
        std::reverse(inheritance.begin(), inheritance.end());

        return inheritance;
    }

    std::optional<source_info> src_info;

    template <class FnT>
    void for_each_proc(module& mod, const FnT& fn) const {
        if (extends) {
            auto base_sym = get_symbol(extends->base);
            auto base_serv = std::get<const service*>(base_sym);
            base_serv->for_each_proc(mod, fn);
        }

        for (auto& [name, proc] : procedures) {
            fn(*this, name, proc);
        }
    }
};

inline const service common_base {};
} // namespace lidl