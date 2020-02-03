add_library(lidl_cppgen cppgen.cpp get_identifier.cpp)
target_link_libraries(lidl_cppgen PUBLIC lidl_core)

if (BUILD_TESTS)
    enable_testing()
    add_executable(lidl_cppgen_get_identifier_test get_identifier_test.cpp)
    target_link_libraries(lidl_cppgen_get_identifier_test PUBLIC lidl_cppgen test_main)
    add_test(lidl_cppgen_get_identifier_test lidl_cppgen_get_identifier_test)
endif()