cmake_minimum_required(VERSION 3.10)
project(lidl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(lidl_core src/basic_types.cpp include/lidl/basic.hpp include/lidl/types.hpp include/lidl/generics.hpp src/generics.cpp src/module.cpp include/lidl/symbol_table.hpp src/symbol_table.cpp include/lidl/service.hpp src/service.cpp)
target_compile_features(lidl_core PUBLIC cxx_std_20)
target_include_directories(lidl_core PUBLIC include)
target_link_libraries(lidl_core PUBLIC fmt GSL)

add_library(lidl_yaml src/yaml.cpp include/lidl/yaml.hpp)
target_link_libraries(lidl_yaml PUBLIC lidl_core yaml-cpp)

add_library(lidl_cppgen src/cppgen.cpp)
target_link_libraries(lidl_cppgen PUBLIC lidl_core)

add_executable(lidlc src/main.cpp)
target_link_libraries(lidlc PUBLIC lidl_core lidl_yaml lidl_cppgen)

add_subdirectory(3rd_party EXCLUDE_FROM_ALL)

add_subdirectory(runtime/cpp)
add_subdirectory(examples)

enable_testing()
add_library(test_main src/test_main.cpp)
target_include_directories(test_main PUBLIC include)

add_executable(layout_test src/layout_test.cpp)
target_link_libraries(layout_test PUBLIC lidl_core test_main)
add_test(layout_test layout_test)