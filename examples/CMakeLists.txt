add_executable(vec3f_example vec3f_example.cpp)
target_link_libraries(vec3f_example PUBLIC lidl_rt vec3f)
add_lidlc(vec3f vec3f.yaml)

add_executable(strings_example strings.cpp)
target_link_libraries(strings_example PUBLIC lidl_rt strings)
add_lidlc(strings strings.yaml)

add_executable(tests_example tests_example.cpp)
target_link_libraries(tests_example PUBLIC lidl_rt tests)
add_lidlc(tests tests.yaml)


add_executable(service_example service.cpp)
target_link_libraries(service_example PUBLIC lidl_rt service)
add_lidlc(service service.yaml)