#include "pybind.hpp"

#include <lidl/lidl.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <service_generated.hpp>

using namespace lidl::pybind;
namespace py = pybind11;

std::vector<std::unique_ptr<type_wrapper>> wrapped_types;

template<class T>
void add_types(T&& t) {
    create_basic_types<uint8_t,
                       int8_t,
                       uint16_t,
                       int16_t,
                       uint32_t,
                       int32_t,
                       uint64_t,
                       int64_t,
                       float,
                       double,
                       bool>(
        "u8", "i8", "u16", "i16", "u32", "i32", "u64", "i64", "f32", "f64", "bool");
    wrapped_types.emplace_back(std::make_unique<string_wrapper>());
    wrapped_types.emplace_back(std::make_unique<vector_wrapper>());

    int i = 0;
    for (auto& val : wrapped_types) {
        t.value(val->name().c_str(), types(i));
    }
}

template<class T>
void add_creators(py::module& m, T&& builder) {
    bind_all(m, lidl::module_traits<mod>::symbols{});

    for (auto& val : wrapped_types) {
        val->add_creator(m, builder);
    }
}

PYBIND11_MODULE(pylidl, m) {
    py::class_<vector_base>(m, "vector").def("get", &vector_base::get);
    py::class_<lidl::string>(m, "string").def("get", &lidl::string::string_view);

    py::class_<lidl::buffer>(m, "buffer").def("get", [](const lidl::buffer& buf) {
        auto span = buf.get_buffer();
        return py::bytes(reinterpret_cast<const char*>(span.data()), span.size());
    });

    add_types(py::enum_<types>(m, "types"));
    add_creators(
        m,
        py::class_<lidl::message_builder>(m, "message_builder")
            .def(py::init([](int32_t max_size) {
                auto data = new std::vector<uint8_t>(max_size);
                return lidl::message_builder(*data);
            }))
            .def_property_readonly("buffer", &lidl::message_builder::get_buffer));
}