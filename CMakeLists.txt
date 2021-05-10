cmake_minimum_required(VERSION 3.10)
project(lidl VERSION 0.1.4)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_PYBINDGEN "Enable the experimental pybindgen backend" OFF)
option(BUILD_TOOLS "Enable the build of accompanying tools. Disable to only get lidl as a library" ON)
option(DISABLE_FRONTEND "Disable the lidl frontend" OFF)
option(DISABLE_YAML "Disable the yaml frontend" OFF)

set(ENABLE_BACKENDS "cpp;js")

set(CMAKE_CXX_FLAGS "-fsanitize=address,undefined")

add_library(
  lidl_core
  src/basic_types.cpp
  include/lidl/basic.hpp
  include/lidl/types.hpp
  include/lidl/generics.hpp
  src/generics.cpp
  src/module.cpp
  include/lidl/service.hpp
  src/service.cpp
  include/lidl/errors.hpp
  src/scope.cpp
  src/name.cpp
  src/structure.cpp
  include/lidl/union.hpp
  include/lidl/member.hpp
  src/union.cpp
  src/enumeration.cpp
  include/lidl/union.hpp
  include/lidl/enumeration.hpp
  src/instantiation_pass.cpp
  include/lidl/basic_types.hpp
  src/layout.cpp
  include/lidl/binary_writer.hpp
  include/lidl/view_types.hpp
  src/types.cpp
  include/lidl/source_info.hpp
  include/lidl/base.hpp
  include/lidl/loader.hpp
  src/loader.cpp
  include/lidl/inheritance.hpp
  src/inheritance.cpp
  src/base.cpp
  include/lidl/algorithm.hpp
  include/lidl/queue.hpp
  include/lidl/error.hpp
)
target_compile_features(lidl_core PUBLIC cxx_std_20)
target_include_directories(lidl_core PUBLIC include)
target_link_libraries(lidl_core PUBLIC fmt GSL yaml-cpp)
target_compile_definitions(lidl_core PUBLIC LIDL_VERSION_STRING=\"${PROJECT_VERSION}\")
if(UNIX AND NOT APPLE)
  target_compile_options(lidl_core PUBLIC -Wno-deprecated-declarations -Werror=return-type)
  target_compile_options(lidl_core PUBLIC -ffunction-sections -fdata-sections)
  target_link_options(lidl_core PUBLIC -Wl,--gc-sections)
endif()

add_subdirectory(3rd_party EXCLUDE_FROM_ALL)

if(BUILD_TESTS)
  include(CTest)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/runtime/cpp")

add_subdirectory(runtime/cpp)
add_subdirectory(examples)
add_subdirectory(src)
