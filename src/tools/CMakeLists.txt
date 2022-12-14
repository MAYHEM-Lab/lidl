add_executable(lidlc lidlc_main.cpp)
target_link_libraries(lidlc PUBLIC lidl_core lidl_codegen BFG::Lyra)
target_include_directories(lidlc PRIVATE "..")

add_executable(lidlq lidlq_main.cpp)
target_link_libraries(lidlq PUBLIC lidl_core BFG::Lyra)
target_include_directories(lidlq PRIVATE "..")

if(UNIX AND NOT APPLE)
  #target_link_options(lidlc PRIVATE -static-libgcc -static-libstdc++ -static)
endif()

add_executable(lidl2yaml bin2yaml_main.cpp)
target_link_libraries(lidl2yaml PUBLIC lidl_core lidl_rt)
target_include_directories(lidl2yaml PRIVATE "..")

add_executable(yaml2lidl yaml2lidl_main.cpp)
target_link_libraries(yaml2lidl PUBLIC lidl_core lidl_rt)
target_include_directories(yaml2lidl PRIVATE "..")

if (TARGET lidl_yaml)
  target_link_libraries(lidlc PUBLIC lidl_yaml)
  target_link_libraries(lidlq PUBLIC lidl_yaml)
  target_link_libraries(lidl2yaml PUBLIC lidl_yaml)
  target_link_libraries(yaml2lidl PUBLIC lidl_yaml)
endif()

if (TARGET lidl_frontend)
  target_link_libraries(lidlc PUBLIC lidl_frontend)
  target_link_libraries(lidlq PUBLIC lidl_frontend)
  target_link_libraries(lidl2yaml PUBLIC lidl_frontend)
  target_link_libraries(yaml2lidl PUBLIC lidl_frontend)
endif()

if (TARGET lidl_cppgen)
  target_link_libraries(lidlc PRIVATE lidl_cppgen)
  target_compile_definitions(lidlc PRIVATE ENABLE_CPP)
endif()

if (TARGET lidl_jsgen)
  target_link_libraries(lidlc PRIVATE lidl_jsgen)
  target_compile_definitions(lidlc PRIVATE ENABLE_JS)
endif()

if (TARGET lidl_pygen)
  target_link_libraries(lidlc PRIVATE lidl_pygen)
  target_compile_definitions(lidlc PRIVATE ENABLE_PY)
endif()

