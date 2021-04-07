add_library(
  lidl_cppgen
  cppgen.cpp
  get_identifier.cpp
  struct_gen.hpp
  struct_bodygen.hpp
  enum_gen.hpp
  enum_gen.cpp
  union_gen.cpp
  union_gen.hpp
  struct_gen.cpp
  struct_gen.hpp
  generic_gen.cpp
  generic_gen.hpp
  struct_bodygen.cpp
  service_stub_gen.hpp
  raw_union_gen.cpp
  raw_union_gen.hpp
  service_gen.cpp
)
target_link_libraries(lidl_cppgen PUBLIC lidl_core lidl_codegen)

if(BUILD_TESTS)
  enable_testing()
  add_executable(lidl_cppgen_get_identifier_test get_identifier_test.cpp)
  target_link_libraries(lidl_cppgen_get_identifier_test PUBLIC lidl_cppgen test_main)
  add_test(lidl_cppgen_get_identifier_test lidl_cppgen_get_identifier_test)
endif()
