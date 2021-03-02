add_executable(frontend frontend.cpp)
target_link_libraries(frontend PUBLIC lidl_core foonathan::lexy)