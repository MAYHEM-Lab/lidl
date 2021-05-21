if (NOT DISABLE_FRONTEND)
  add_subdirectory(frontend)
endif()

add_subdirectory(codegen)

list(FIND ENABLE_BACKENDS "cpp" CPP_BACKEND_ENABLED)
if (CPP_BACKEND_ENABLED GREATER_EQUAL 0)
  message(STATUS "Enabling C++ backend")
  add_subdirectory(cppgen)
endif()

list(FIND ENABLE_BACKENDS "js" CPP_BACKEND_ENABLED)
if (CPP_BACKEND_ENABLED GREATER_EQUAL 0)
  message(STATUS "Enabling Typescript backend")
  add_subdirectory(jsgen)
endif()

if (NOT DISABLE_YAML)
  add_library(lidl_yaml yaml.cpp yaml.hpp)
  target_link_libraries(lidl_yaml PUBLIC lidl_core yaml-cpp)
endif()

if(BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(ENABLE_PYBINDGEN)
  add_subdirectory(pybindgen)
endif()

if(ENABLE_LIDLPY)
  add_subdirectory(lidlpy)
endif()

if(BUILD_TESTS)
  enable_testing()
  add_library(test_main test_main.cpp)
  target_include_directories(test_main PUBLIC ".")
  target_compile_features(test_main PUBLIC cxx_std_17)

  add_executable(layout_test layout_test.cpp)
  target_link_libraries(layout_test PUBLIC lidl_core test_main)
  add_test(layout_test layout_test)

#  add_executable(bin2yaml_test bin2yaml_test.cpp)
#  target_link_libraries(bin2yaml_test PUBLIC lidl_core test_main)
#  add_test(bin2yaml_test bin2yaml_test)
#
#  add_executable(yaml2bin_test yaml2bin_test.cpp)
#  target_link_libraries(yaml2bin_test PUBLIC lidl_core test_main)
#  add_test(yaml2bin_test yaml2bin_test)

  add_custom_target(
    check
    COMMAND ${CMAKE_CTEST_COMMAND}
    DEPENDS layout_test scope_test reference_type_pass_test bin2yaml_test yaml2bin_test
  )
endif()
