add_library(lidl_codegen "")
target_link_libraries(lidl_codegen PUBLIC lidl_core)
target_include_directories(lidl_codegen PUBLIC .)
target_sources(lidl_codegen PRIVATE
    generator_base.hpp
    codegen.cpp
    codegen.hpp
    sections.hpp
    sections.cpp
    emitter.cpp
    emitter.hpp)


if(BUILD_TESTS)
    enable_testing()
endif()
