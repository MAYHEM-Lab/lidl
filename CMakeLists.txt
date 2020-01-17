cmake_minimum_required(VERSION 3.10)
project(proto)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(proto src/main.cpp src/cppgen.cpp include/proto/basic.hpp)
target_compile_features(proto PUBLIC cxx_std_20)
target_include_directories(proto PUBLIC include)
