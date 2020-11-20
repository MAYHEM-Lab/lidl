add_library(lidl_jsgen jsgen.cpp struct_gen.cpp struct_gen.hpp generator.hpp get_identifier.cpp)
target_link_libraries(lidl_jsgen PUBLIC lidl_core lidl_codegen)