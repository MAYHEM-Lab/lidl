pybind11_add_module(lidlpy lidlpy.cpp)
target_link_libraries(lidlpy PUBLIC lidl_core)
target_include_directories(lidlpy PRIVATE "..")
#target_link_options(lidlpy PUBLIC -fsanitize=address -static-libasan)

if (TARGET lidl_yaml)
    target_link_libraries(lidlpy PUBLIC lidl_yaml)
endif()

if (TARGET lidl_frontend)
    target_link_libraries(lidlpy PUBLIC lidl_frontend)
endif()