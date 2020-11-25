add_library(lidl_jsgen jsgen.cpp struct_gen.cpp struct_gen.hpp get_identifier.cpp jsgen.hpp union_gen.cpp union_gen.hpp enum_gen.cpp enum_gen.hpp)
target_link_libraries(lidl_jsgen PUBLIC lidl_core lidl_codegen)

if(BUILD_TESTS)
    enable_testing()
    add_executable(lidl_jsgen_get_identifier_test get_identifier_test.cpp)
    target_link_libraries(lidl_jsgen_get_identifier_test PUBLIC lidl_jsgen test_main)
    add_test(lidl_jsgen_get_identifier_test lidl_jsgen_get_identifier_test)
endif()
