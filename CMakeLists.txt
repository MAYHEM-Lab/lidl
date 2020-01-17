cmake_minimum_required(VERSION 3.10)
project(proto)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(proto_core src/basic_types.cpp include/proto/basic.hpp include/proto/types.hpp)
target_compile_features(proto_core PUBLIC cxx_std_20)
target_include_directories(proto_core PUBLIC include)
target_link_libraries(proto_core PUBLIC fmt GSL)

add_library(proto_yaml src/yaml.cpp include/proto/yaml.hpp)
target_link_libraries(proto_yaml PUBLIC proto_core yaml-cpp)

add_library(proto_cppgen src/cppgen.cpp)
target_link_libraries(proto_cppgen PUBLIC proto_core)

add_executable(protoc src/main.cpp)
target_link_libraries(protoc PUBLIC proto_core proto_yaml proto_cppgen)

add_subdirectory(3rd_party EXCLUDE_FROM_ALL)