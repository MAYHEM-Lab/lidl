add_library(lidl_frontend ast.hpp loader.cpp parser.cpp parser.hpp)
target_link_libraries(lidl_frontend PUBLIC lidl_core)
target_link_libraries(lidl_frontend PRIVATE foonathan::lexy)

add_executable(frontend frontend.cpp)
target_link_libraries(frontend PUBLIC lidl_frontend)