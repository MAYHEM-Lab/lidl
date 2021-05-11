add_library(lidl_frontend ast.hpp loader.cpp parser.hpp lexer.cpp lexer.hpp generator.hpp parser2.hpp parser2.cpp)
target_link_libraries(lidl_frontend PUBLIC lidl_core)
target_compile_features(lidl_frontend PUBLIC cxx_std_20)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(lidl_frontend INTERFACE -fcoroutines)
endif()

add_executable(frontend frontend.cpp)
target_link_libraries(frontend PUBLIC lidl_frontend)