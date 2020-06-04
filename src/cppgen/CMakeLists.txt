add_library(lidl_cppgen cppgen.cpp get_identifier.cpp generator_base.hpp generator.hpp struct_gen.hpp emitter.hpp struct_bodygen.hpp enum_gen.hpp enum_gen.cpp union_gen.cpp union_gen.hpp struct_gen.cpp struct_gen.hpp generic_gen.cpp generic_gen.hpp struct_bodygen.cpp emitter.cpp)
target_link_libraries(lidl_cppgen PUBLIC lidl_core)

if (BUILD_TESTS)
    enable_testing()
    add_executable(lidl_cppgen_get_identifier_test get_identifier_test.cpp)
    target_link_libraries(lidl_cppgen_get_identifier_test PUBLIC lidl_cppgen test_main)
    add_test(lidl_cppgen_get_identifier_test lidl_cppgen_get_identifier_test)

    add_executable(lidl_cppgen_emitter_test emitter_test.cpp)
    target_link_libraries(lidl_cppgen_emitter_test PUBLIC test_main)
    add_test(lidl_cppgen_emitter_test lidl_cppgen_emitter_test)

    add_executable(lidl_cppgen_struct_bodygen_test struct_bodygen_test.cpp get_identifier.cpp)
    target_link_libraries(lidl_cppgen_struct_bodygen_test PUBLIC test_main lidl_core)
    add_test(lidl_cppgen_struct_bodygen_test lidl_cppgen_struct_bodygen_test)
endif()