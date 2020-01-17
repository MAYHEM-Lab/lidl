cmake_minimum_required(VERSION 3.10)
project(lidl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(lidl_core src/basic_types.cpp include/lidl/basic.hpp include/lidl/types.hpp)
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