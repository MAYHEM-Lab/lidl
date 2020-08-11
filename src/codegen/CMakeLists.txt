add_library(lidl_codegen "")
target_link_libraries(lidl_codegen PUBLIC lidl_core)
target_include_directories(lidl_codegen PUBLIC .)
target_sources(lidl_codegen PRIVATE
    codegen.cpp
    codegen.hpp
    sections.hpp
    sections.cpp
    emitter.cpp
    emitter.hpp)


if(BUILD_TESTS)
    enable_testing()
    add_executable(lidl_codegen_emitter_test emitter_test.cpp)
    target_link_libraries(lidl_codegen_emitter_test PUBLIC test_main lidl_core lidl_codegen)
    add_test(lidl_codegen_emitter_test lidl_cppgen_emitter_test)
endif()
