#include <lidl/module.hpp>
#include <lidl/service.hpp>
#include <lidl/structure.hpp>
#include <lidl/view_types.hpp>

namespace lidl {
structure procedure_params_struct(const module& mod,
                                  std::string_view serv,
                                  std::string_view name,
                                  const procedure& proc) {
    structure s;
    for (auto& [name, param] : proc.parameters) {
        member m;
        if (auto vt = dynamic_cast<const view_type*>(get_type(mod, param)); vt) {
            /**
             * While procedures may have view types in their parameters, view types do not
             * have representations on the wire. The struct we are generating is only used
             * when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = vt->m_wire_type;
        } else {
            m.type_ = param;
        }
        s.members.emplace_back(name, std::move(m));
    }
    s.attributes.add(std::make_unique<procedure_params_attribute>(
        std::string(serv), std::string(name), proc));
    return s;
}

structure procedure_results_struct(const module& mod,
                                   std::string_view serv,
                                   std::string_view name,
                                   const procedure& proc) {
    structure s;
    for (auto& param : proc.return_types) {
        member m;
        if (auto vt = dynamic_cast<const view_type*>(get_type(mod, param)); vt) {
            /**
             * While procedures may have view types in their return values, view types do
             * not have representations on the wire. The struct we are generating is only
             * used when a request needs to be serialized. Therefore, we replace the view
             * type with it's wire type.
             * For instance, `string_view` in the procedure becomes `string` in the wire.
             */
            m.type_ = vt->m_wire_type;
        } else {
            m.type_ = param;
        }
        s.members.emplace_back(fmt::format("ret{}", s.members.size()), std::move(m));
    }
    s.attributes.add(std::make_unique<procedure_params_attribute>(
        std::string(serv), std::string(name), proc));
    return s;
}

void service_pass(module& mod) {
    for (auto& service : mod.services) {
        std::string service_name(nameof(*mod.symbols->definition_lookup(&service)));
        union_type procedure_params;
        union_type procedure_results;
        for (auto& [proc_name, proc] : service.procedures) {
            mod.structs.emplace_back(
                procedure_params_struct(mod, service_name, proc_name, proc));
            auto handle = define(*mod.symbols,
                                 fmt::format("{}_{}_params", service_name, proc_name),
                                 &mod.structs.back());
            procedure_params.members.emplace_back(proc_name, member{name{handle}});
            proc.params_struct = &mod.structs.back();

            mod.structs.emplace_back(
                procedure_results_struct(mod, service_name, proc_name, proc));
            auto res_handle =
                define(*mod.symbols,
                       fmt::format("{}_{}_results", service_name, proc_name),
                       &mod.structs.back());
            procedure_results.members.emplace_back(proc_name, member{name{res_handle}});
            proc.results_struct = &mod.structs.back();
        }
        mod.unions.push_back(std::move(procedure_params));
        auto handle = define(*mod.symbols, service_name + "_call", &mod.unions.back());
        service.procedure_params_union = &mod.unions.back();

        mod.unions.push_back(std::move(procedure_results));
        auto res_handle =
            define(*mod.symbols, service_name + "_return", &mod.unions.back());
        service.procedure_results_union = &mod.unions.back();
    }
}
} // namespace lidl