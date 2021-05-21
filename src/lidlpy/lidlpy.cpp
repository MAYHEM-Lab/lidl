#include <lidl/basic.hpp>
#include <lidl/loader.hpp>
#include <lidl/module.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace lidl;

PYBIND11_MODULE(lidlpy, m) {
    py::class_<symbol_handle>(m, "symbol_handle")
        .def("get_scope", &symbol_handle::get_scope)
        .def("get_id", &symbol_handle::get_id);
    m.def("recursive_definition_lookup", &recursive_definition_lookup);
    m.def("local_name", &local_name);
    m.def("absolute_name", &absolute_name);
    m.def("find_parent_module", &find_parent_module, py::return_value_policy::reference);
    m.def("root_module", &root_module, py::return_value_policy::reference);
    m.def("root_scope", &root_scope, py::return_value_policy::reference);
    m.def(
        "cerr",
        []() -> std::ostream& { return std::cerr; },
        py::return_value_policy::reference);
    py::class_<scope>(m, "scope").def("nameof", &scope::nameof).def("dump", &scope::dump);
    py::class_<base>(m, "base")
        .def("parent", py::overload_cast<>(&base::parent, py::const_))
        .def("is_view", &base::is_view)
        .def("get_scope",
             py::overload_cast<>(&base::get_scope, py::const_),
             py::return_value_policy::reference);
    py::class_<module, base>(m, "module")
        .def(
            "services",
            [](module& mod) {
                std::vector<service*> res(mod.services.size());
                std::transform(mod.services.begin(),
                               mod.services.end(),
                               res.begin(),
                               [](auto& ptr) { return ptr.get(); });
                return res;
            },
            py::return_value_policy::reference);
    py::class_<procedure, base>(m, "procedure");
    py::class_<service, base>(m, "service")
        .def("all_procedures",
             &service::all_procedures,
             py::return_value_policy::reference);
    py::class_<module_meta>(m, "module_meta");
    py::class_<module_loader>(m, "module_loader")
        .def("load", &module_loader::load)
        .def("get_module",
             &module_loader::get_module_ptr,
             py::return_value_policy::reference)
        .def("get_metadata", &module_loader::get_metadata);
    py::class_<import_resolver, std::shared_ptr<import_resolver>>(m, "import_resolver")
        .def("resolve_import", &import_resolver::resolve_import);
    py::class_<path_resolver, import_resolver, std::shared_ptr<path_resolver>>(
        m, "path_resolver")
        .def(py::init<>())
        .def("add_import_path", &path_resolver::add_import_path);
    py::class_<load_context>(m, "load_context")
        .def(py::init<>())
        .def("get_root", &load_context::get_root, py::return_value_policy::reference)
        .def("do_import", &load_context::do_import, py::return_value_policy::reference)
        .def("set_importer", &load_context::set_importer);
    py::class_<std::ostream>(m, "ostream");
}
