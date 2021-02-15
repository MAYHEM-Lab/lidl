//
// Created by fatih on 1/24/20.
//

#pragma once

#include "scope.hpp"

#include <cassert>
#include <deque>
#include <lidl/inheritance.hpp>
#include <lidl/structure.hpp>
#include <lidl/types.hpp>
#include <lidl/union.hpp>

namespace lidl {
enum class param_flags
{
    in     = 1,
    out    = 2,
    in_out = 3
};

struct parameter : public base {
    using base::base;
    lidl::name type;
    param_flags flags = param_flags::in;
};

struct procedure : public base {
    using base::base;
    std::deque<name> return_types;
    std::deque<std::pair<std::string, parameter>> parameters;

    void add_parameter(std::string name, parameter proc) {
        parameters.emplace_back(std::move(name), std::move(proc));
        define(get_scope(), parameters.back().first, &parameters.back().second);
    }

    std::unique_ptr<structure> params_struct;
    std::unique_ptr<structure> results_struct;
    name params_struct_name;
    name results_struct_name;
};

struct property : member {
    using member::member;
};

struct service
    : public base
    , public extendable<service> {
    using base::base;

    void add_procedure(std::string name, std::unique_ptr<procedure> proc) {
        procedures.emplace_back(std::move(name), std::move(proc));
        define(get_scope(), procedures.back().first, procedures.back().second.get());
    }

    std::deque<std::pair<std::string, property>> properties;
    std::optional<union_type> procedure_params_union;
    std::optional<union_type> procedure_results_union;

    auto& own_procedures() {
        return procedures;
    }

    auto& own_procedures() const {
        return procedures;
    }

    std::vector<std::pair<std::string_view, const procedure*>> all_procedures() const {
        std::vector<std::pair<std::string_view, const procedure*>> res;
        for (auto& s : inheritance_list()) {
            for (auto& [name, proc] : s->procedures) {
                res.emplace_back(name, proc.get());
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

    template<class FnT>
    void for_each_proc(module& mod, const FnT& fn) const {
        if (extends) {
            auto base_sym  = get_symbol(extends->base);
            auto base_serv = dynamic_cast<const service*>(base_sym);
            base_serv->for_each_proc(mod, fn);
        }

        for (auto& [name, proc] : procedures) {
            fn(*this, name, proc);
        }
    }

private:
    std::deque<std::pair<std::string, std::unique_ptr<procedure>>> procedures;
};

inline const service common_base{};
} // namespace lidl