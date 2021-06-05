add_library(lidl_pygen pygen.cpp get_identifier.cpp get_identifier.hpp)
target_link_libraries(lidl_pygen PUBLIC lidl_codegen)