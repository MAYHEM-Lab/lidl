//
// Created by fatih on 1/24/20.
//

#include <lidl/module.hpp>
#include <lidl/service.hpp>

namespace lidl {
namespace {
std::unique_ptr<structure> procedure_params_struct(const module& mod,
                                                   service& servic,
                                                   std::string_view name,
                                                   const procedure& proc) {
    auto s = std::make_unique<structure>(&servic, proc.src_info);
    for (auto& [name, param] : proc.parameters) {
        member m(s.get(), param.src_info);
        auto param_t = get_type(mod, param.type);
        if (param_t->is_view(mod)) {
            /**
             * While procedures may have view types in their parameters, view types do not
             * have representations on the wire. The struct we are generating is only used
             * when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = param_t->get_wire_type(mod).value();
            add_pointer_to_name_if_needed(mod, m.type_);
        } else {
            static auto string = dynamic_cast<const type*>(
                get_symbol(*lidl::recursive_name_lookup(mod.symbols(), "string")));

            if (param_t->is_reference_type(mod) &&
                get_type(mod, param.type.args[0].as_name()) == string) {
                std::cerr << fmt::format("Warning at {}: Prefer using string_view rather "
                                         "than string in procedure parameters.\n",
                                         to_string(*proc.src_info));
            }

            m.type_ = param.type;
            add_pointer_to_name_if_needed(mod, m.type_);
        }
        s->add_member(name, std::move(m));
    }

    s->params_info = procedure_params_info{&servic, std::string(name), &proc};

    return s;
}

std::unique_ptr<structure> procedure_results_struct(const module& mod,
                                                    service& servic,
                                                    std::string_view name,
                                                    const procedure& proc) {
    auto s = std::make_unique<structure>(&servic, proc.src_info);
    for (auto& param : proc.return_types) {
        member m(s.get(), proc.src_info);
        auto param_t = get_type(mod, param);
        if (param_t->is_view(mod)) {
            /**
             * While procedures may have view types in their return values, view types do
             * not have representations on the wire. The struct we are generating is only
             * used when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = *param_t->get_wire_type(mod);
            add_pointer_to_name_if_needed(mod, m.type_);
        } else {
            m.type_ = param;
            add_pointer_to_name_if_needed(mod, m.type_);
        }
        s->add_member(fmt::format("ret{}", s->all_members().size()), std::move(m));

        s->return_info = procedure_params_info{&servic, std::string(name), &proc};
    }

    return s;
}
} // namespace
structure& procedure::params_struct(const module& mod) const {
    if (structs_dirty) {
        m_params_struct  = procedure_params_struct(mod, get_service(), m_name, *this);
        m_results_struct = procedure_results_struct(mod, get_service(), m_name, *this);

        auto handle     = define(get_service().get_scope(),
                             fmt::format("{}_params", m_name),
                             m_params_struct.get());
        auto res_handle = define(get_service().get_scope(),
                                 fmt::format("{}_results", m_name),
                                 m_results_struct.get());

        params_struct_name = name{handle};
        add_pointer_to_name_if_needed(mod, params_struct_name);
        results_struct_name = name{res_handle};
        add_pointer_to_name_if_needed(mod, results_struct_name);

        structs_dirty = false;
    }
    return *m_params_struct;
}

structure& procedure::results_struct(const module& mod) const {
    if (structs_dirty) {
        m_params_struct  = procedure_params_struct(mod, get_service(), m_name, *this);
        m_results_struct = procedure_results_struct(mod, get_service(), m_name, *this);

        auto handle     = define(get_service().get_scope(),
                             fmt::format("{}_params", m_name),
                             m_params_struct.get());
        auto res_handle = define(get_service().get_scope(),
                                 fmt::format("{}_results", m_name),
                                 m_results_struct.get());

        params_struct_name = name{handle};
        add_pointer_to_name_if_needed(mod, params_struct_name);
        results_struct_name = name{res_handle};
        add_pointer_to_name_if_needed(mod, results_struct_name);

        structs_dirty = false;
    }
    return *m_results_struct;
}

service& procedure::get_service() const {
    return *static_cast<service*>(parent());
}

union_type& service::procedure_params_union(const module& mod) const {
    generate_unions_if_dirty(mod);
    return *m_procedure_params_union;
}

union_type& service::procedure_results_union(const module& mod) const {
    generate_unions_if_dirty(mod);
    return *m_procedure_results_union;
}

void service::generate_unions_if_dirty(const module& mod) const {
    if (!unions_dirty) {
        return;
    }

    m_procedure_params_union.emplace(const_cast<service*>(this), src_info);
    auto& procedure_params = *m_procedure_params_union;

    m_procedure_results_union.emplace(const_cast<service*>(this), src_info);
    auto& procedure_results = *m_procedure_results_union;

    for (auto& s : inheritance_list()) {
        for (auto& [proc_name, proc] : s->own_procedures()) {
            proc->params_struct(mod);
            assert(proc->params_struct_name.base.is_valid());
            assert(proc->results_struct_name.base.is_valid());
            procedure_params.add_member(
                proc_name,
                member{proc->params_struct_name, &procedure_params, proc->src_info});
            procedure_results.add_member(
                proc_name,
                member{proc->results_struct_name, &procedure_results, proc->src_info});
        }
    }

    procedure_params.call_for = this;
    define(get_scope(), "call_union", &procedure_params);

    procedure_results.return_for = this;
    define(get_scope(), "return_union", &procedure_results);

    unions_dirty = false;
}
} // namespace lidl