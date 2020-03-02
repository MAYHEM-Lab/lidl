add_subdirectory(cppgen)
add_subdirectory(tools)
add_subdirectory(pybindgen)

add_library(lidl_yaml yaml.cpp yaml.hpp)
target_link_libraries(lidl_yaml PUBLIC lidl_core yaml-cpp)

if (BUILD_TESTS)
    enable_testing()
    add_library(test_main test_main.cpp)
    target_include_directories(test_main PUBLIC ".")

    add_executable(layout_test layout_test.cpp)
    target_link_libraries(layout_test PUBLIC lidl_core test_main)
    add_test(layout_test layout_test)

    add_executable(scope_test scope_test.cpp)
    target_link_libraries(scope_test PUBLIC lidl_core test_main)
    add_test(scope_test scope_test)

    add_executable(module_test module_test.cpp)
    target_link_libraries(module_test PUBLIC lidl_core test_main)
    add_test(module_test module_test)

    add_executable(reference_type_pass_test reference_type_pass_test.cpp)
    target_link_libraries(reference_type_pass_test PUBLIC lidl_core test_main)
    add_test(reference_type_pass_test reference_type_pass_test)

    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                    DEPENDS 
                    layout_test
                    scope_test
                    module_test
                    )
endif()
