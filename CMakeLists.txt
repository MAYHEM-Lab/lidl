cmake_minimum_required(VERSION 3.10)
project(lidl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BUILD_TESTS ON)

add_library(lidl_core
        src/basic_types.cpp
        include/lidl/basic.hpp
        include/lidl/types.hpp
        include/lidl/generics.hpp
        src/generics.cpp
        src/module.cpp
        include/lidl/service.hpp
        src/service.cpp
        include/lidl/errors.hpp
        src/reference_type_pass.cpp
        src/scope.cpp
        src/name.cpp
        src/attributes.cpp
        src/structure.cpp
        include/lidl/union.hpp
        include/lidl/member.hpp
        src/union.cpp
        src/enumeration.cpp
        include/lidl/union.hpp
        include/lidl/enumeration.hpp
        src/union_enum_pass.cpp
        src/instantiation_pass.cpp
        src/service_pass.cpp
        include/lidl/basic_types.hpp
        src/layout.cpp
        include/lidl/binary_writer.hpp
        include/lidl/types/pointer.hpp
        include/lidl/view_types.hpp
        src/types.cpp)
target_compile_features(lidl_core PUBLIC cxx_std_17)
target_include_directories(lidl_core PUBLIC include)
target_link_libraries(lidl_core PUBLIC fmt GSL yaml-cpp)
target_compile_options(lidl_core PUBLIC -Wno-deprecated-declarations -Werror=return-type)

add_subdirectory(3rd_party EXCLUDE_FROM_ALL)

if (BUILD_TESTS)
    include(CTest)
endif()

add_subdirectory(runtime/cpp)
add_subdirectory(examples)
add_subdirectory(src)