#include <lidl/lidl.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace lidl::py {
lidl::string* create_string(lidl::message_builder& builder, const std::string& str) {
    return &lidl::create_string(builder, str);
}
}


enum class types : intptr_t {};

template <class T>
types type() {
    static int x;
    return static_cast<types>(reinterpret_cast<intptr_t>(&x));
}

namespace py = pybind11;

template <class T>
void add_types(T&& t) {
    t.value("u8", type<uint8_t>());
}

void create(lidl::message_builder& builder, types t, py::object val) {
    if (t == type<uint8_t>()) {
        lidl::emplace_raw<uint8_t>(builder, val.cast<uint8_t>());
    }
}

PYBIND11_MODULE(pylidl, m) {
    py::class_<lidl::string>(m, "string")
        .def("get", &lidl::string::unsafe_string_view);

    py::class_<lidl::buffer>(m, "buffer")
        .def("get", [](const lidl::buffer& buf) {
            auto span = buf.get_buffer();
            return py::bytes(reinterpret_cast<const char*>(span.data()), span.size());
        });

    add_types(py::enum_<types>(m, "types"));

    py::class_<lidl::message_builder>(m, "message_builder")
        .def(py::init([](int32_t max_size) {
            auto data = new std::vector<uint8_t>(max_size);
            return lidl::message_builder(*data);
        }))
        .def_property_readonly("buffer", &lidl::message_builder::get_buffer)
        .def("create_string", &lidl::py::create_string,
        py::return_value_policy::reference)
        .def("create_i8", &lidl::emplace_raw<int8_t, int8_t>)
        .def("create_i16", &lidl::emplace_raw<int16_t, int16_t>)
        .def("create_i32", &lidl::emplace_raw<int32_t, int32_t>)
        .def("create_i64", &lidl::emplace_raw<int64_t, int64_t>)
        .def("create_u8", &lidl::emplace_raw<uint8_t, uint8_t>)
        .def("create_u16", &lidl::emplace_raw<uint16_t, uint16_t>)
        .def("create_u32", &lidl::emplace_raw<uint32_t, uint32_t>)
        .def("create_u64", &lidl::emplace_raw<uint64_t, uint64_t>)
        .def("create", &create)
    ;
}