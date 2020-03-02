add_library(pybindgen pybindgen.cpp)
target_link_libraries(pybindgen PUBLIC cppgen)
