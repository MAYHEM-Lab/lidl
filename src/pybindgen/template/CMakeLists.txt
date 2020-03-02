pybind11_add_module(pylidl python.cpp)
target_link_libraries(pylidl PUBLIC lidl_rt service)