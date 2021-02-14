include(lidlc)

add_executable(vec3f_example vec3f_example.cpp)
target_link_libraries(vec3f_example PUBLIC lidl_rt vec3f)
add_lidlc(vec3f vec3f.yaml)

add_executable(strings_example strings.cpp)
target_link_libraries(strings_example PUBLIC lidl_rt strings)
add_lidlc(strings strings.yaml)

add_executable(tests_example tests_example.cpp)
target_link_libraries(tests_example PUBLIC lidl_rt tests)
add_lidlc(tests tests.yaml)

# This example needs to store vector<identifier> in identifier and is not supported yet!
#add_executable(lidl_example lidl_example.cpp)
#target_link_libraries(lidl_example PUBLIC lidl_rt lidl_gen)
#add_lidlc(lidl_gen lidl.yaml)

add_executable(service_example service.cpp tcp_server.cpp)
target_link_libraries(service_example PUBLIC lidl_rt service)
add_lidlc(service service.yaml)

add_executable(union_example union.cpp)
target_link_libraries(union_example PUBLIC lidl_rt union)
add_lidlc(union union.yaml)

add_executable(generics_example generic_example.cpp)
target_link_libraries(generics_example PUBLIC lidl_rt generics)
add_lidlc(generics generics.yaml)

#pybind11_add_module(pylidl python.cpp)
#target_link_libraries(pylidl PUBLIC lidl_rt service)