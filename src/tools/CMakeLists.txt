add_executable(lidlc lidlc_main.cpp)
target_link_libraries(lidlc PUBLIC lidl_core lidl_yaml lidl_cppgen BFG::Lyra)
target_include_directories(lidlc PRIVATE "..")

add_executable(lidl2yaml bin2yaml_main.cpp)
target_link_libraries(lidl2yaml PUBLIC lidl_core yaml-cpp lidl_yaml lidl_rt)
target_include_directories(lidl2yaml PRIVATE "..")