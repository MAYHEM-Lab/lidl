#include <cassert>
#include <lidl/module.hpp>
#include <lidl/service.hpp>
#include <lidl/structure.hpp>
#include <lidl/view_types.hpp>
#include <stack>

namespace lidl {
namespace {
structure procedure_params_struct(const module& mod,
                                  const service& servic,
                                  std::string_view name,
                                  const procedure& proc) {
    structure s;
    for (auto& [name, param] : proc.parameters) {
        member m;
        auto param_t = get_type(mod, param.type);
        if (param_t->is_view(mod)) {
            /**
             * While procedures may have view types in their parameters, view types do not
             * have representations on the wire. The struct we are generating is only used
             * when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = param_t->get_wire_type(mod);
        } else {
            static auto string = std::get<const type*>(
                get_symbol(*lidl::recursive_name_lookup(*mod.symbols, "string")));

            if (param_t->is_reference_type(mod) &&
                get_type(mod, param.type.args[0].as_name()) == string) {
                std::cerr << fmt::format("Warning at {}: Prefer using string_view rather "
                                         "than string in procedure parameters.\n",
                                         to_string(*proc.src_info));
            }

            m.type_ = param.type;
        }
        s.members.emplace_back(name, std::move(m));
    }

    s.params_info = procedure_params_info{&servic, std::string(name), &proc};

    return s;
}

structure procedure_results_struct(const module& mod,
                                   const service& servic,
                                   std::string_view name,
                                   const procedure& proc) {
    structure s;
    for (auto& param : proc.return_types) {
        member m;
        auto param_t = get_type(mod, param);
        if (param_t->is_view(mod)) {
            /**
             * While procedures may have view types in their return values, view types do
             * not have representations on the wire. The struct we are generating is only
             * used when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = param_t->get_wire_type(mod);
        } else {
            m.type_ = param;
        }
        s.members.emplace_back(fmt::format("ret{}", s.members.size()), std::move(m));

        s.return_info = procedure_params_info{&servic, std::string(name), &proc};
    }

    return s;
}

bool do_proc_pass(module& mod,
                  service& serv,
                  std::string_view proc_name,
                  procedure& proc) {
    if (proc.results_struct && proc.params_struct) {
        return false;
    }

    mod.structs.emplace_back(procedure_params_struct(mod, serv, proc_name, proc));
    proc.params_struct = &mod.structs.back();

    mod.structs.emplace_back(procedure_results_struct(mod, serv, proc_name, proc));
    proc.results_struct = &mod.structs.back();

    return true;
}

bool do_service_pass(module& mod, service& serv) {
    if (serv.procedure_params_union) {
        return false;
    }
    std::string service_name(local_name(*mod.symbols->definition_lookup(&serv)));
    std::cerr << "Pass for service " << service_name << '\n';

    union_type procedure_params;
    union_type procedure_results;

    for (auto& s : serv.inheritance_list()) {
        for (auto& [proc_name, proc] : s->procedures) {
            assert(proc.params_struct_name.base.is_valid());
            assert(proc.results_struct_name.base.is_valid());
            procedure_params.members.emplace_back(proc_name,
                                                  member{name{proc.params_struct_name}});
            procedure_results.members.emplace_back(proc_name,
                                                   member{name{proc.results_struct_name}});
        }
    }

    mod.unions.push_back(std::move(procedure_params));
    auto handle =
        define(*mod.symbols, fmt::format("{}_call", service_name), &mod.unions.back());
    mod.unions.back().call_for  = &serv;
    serv.procedure_params_union = &mod.unions.back();

    mod.unions.push_back(std::move(procedure_results));
    auto res_handle =
        define(*mod.symbols, fmt::format("{}_return", service_name), &mod.unions.back());
    mod.unions.back().return_for = &serv;
    serv.procedure_results_union = &mod.unions.back();

    return true;
}
} // namespace

bool service_pass(module& mod) {
    bool changed = false;

    for (auto& service : mod.services) {
        changed |= do_service_pass(mod, service);
    }

    return changed;
}

bool service_proc_pass(module& mod) {
    bool changed = false;

    for (auto& serv : mod.services) {
        std::string service_name(local_name(*mod.symbols->definition_lookup(&serv)));

        for (auto& [proc_name, proc] : serv.procedures) {
            if (proc.results_struct && proc.params_struct) {
                continue;
            }

            std::cerr << "Generating param and result structs for " << service_name << "::" << proc_name << '\n';
            auto proc_res = do_proc_pass(mod, serv, proc_name, proc);
            assert(proc_res);
            assert(proc.results_struct && proc.params_struct);

            auto handle = define(*mod.symbols,
                                 fmt::format("{}_{}_params", service_name, proc_name),
                                 proc.params_struct);
            auto res_handle =
                define(*mod.symbols,
                       fmt::format("{}_{}_results", service_name, proc_name),
                       proc.results_struct);

            proc.params_struct_name  = name{handle};
            proc.results_struct_name = name{res_handle};
            changed = true;
        }
    }

    return changed;
}
} // namespace lidl