add_library(lidl_frontend ast.hpp loader.cpp parser.hpp lexer.cpp lexer.hpp parser2.cpp)
target_link_libraries(lidl_frontend PUBLIC lidl_core)

add_executable(frontend frontend.cpp)
target_link_libraries(frontend PUBLIC lidl_frontend)