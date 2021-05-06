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

struct parameter : public cbase<base::categories::other> {
    using cbase::cbase;
    lidl::name type;
    param_flags flags = param_flags::in;


    void finalize(const module& mod) {
        type.finalize(mod);
    }
};

struct procedure : public cbase<base::categories::procedure> {
    using cbase::cbase;
    std::deque<name> return_types;
    std::deque<std::pair<std::string, parameter>> parameters;

    void add_parameter(std::string name, parameter proc) {
        if (!structs_dirty) {
            throw std::runtime_error("Procedure cannot be modified anymore!");
        }
        parameters.emplace_back(std::move(name), std::move(proc));
        define(get_scope(), parameters.back().first, &parameters.back().second);
    }

    void add_return_type(name type) {
        if (!structs_dirty) {
            throw std::runtime_error("Procedure cannot be modified anymore!");
        }

        return_types.push_back(type);
    }

    void finalize(const module& mod) {
        generate_structs_if_dirty(mod);
        for (auto& ret : return_types) {
            ret.finalize(mod);
        }
        for (auto& param : parameters) {
            param.second.finalize(mod);
        }
        m_params_struct->finalize(mod);
        m_results_struct->finalize(mod);
        params_struct_name.finalize(mod);
        results_struct_name.finalize(mod);
    }

    service& get_service() const;

    structure& params_struct(const module& mod) const;
    structure& results_struct(const module& mod) const;
    mutable name params_struct_name;
    mutable name results_struct_name;
    std::string m_name;

private:
    void generate_structs_if_dirty(const module& mod) const;

    mutable bool structs_dirty = true;

    mutable std::unique_ptr<structure> m_params_struct;
    mutable std::unique_ptr<structure> m_results_struct;
};

struct property : member {
    using member::member;
};

struct service
    : public cbase<base::categories::service>
    , public extendable<service> {
    service(base* parent = nullptr, std::optional<source_info> p_src_info = {})
        : cbase(parent, std::move(p_src_info)) {
        get_scope().declare("call_union");
        get_scope().declare("return_union");
    }

    void add_procedure(std::string name, std::unique_ptr<procedure> proc) {
        if (!unions_dirty) {
            throw std::runtime_error("Service cannot be modified anymore!");
        }
        procedures.emplace_back(name, std::move(proc));
        procedures.back().second->m_name = std::move(name);
        define(get_scope(), procedures.back().first, procedures.back().second.get());
    }

    std::deque<std::pair<std::string, property>> properties;

    union_type& procedure_params_union(const module& mod) const;
    union_type& procedure_results_union(const module& mod) const;

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

    void finalize(const module& mod) {
        generate_unions_if_dirty(mod);
        for (auto& proc : procedures) {
            proc.second->finalize(mod);
        }
        m_procedure_params_union->finalize(mod);
        m_procedure_params_union->finalize(mod);
    }

private:
    void generate_unions_if_dirty(const module& mod) const;

    mutable std::optional<union_type> m_procedure_params_union;
    mutable std::optional<union_type> m_procedure_results_union;

    mutable bool unions_dirty = true;

    std::deque<std::pair<std::string, std::unique_ptr<procedure>>> procedures;
};

inline const service common_base{};
} // namespace lidl