add_executable(vec3f_example vec3f_example.cpp)
target_link_libraries(vec3f_example PUBLIC lidl_rt vec3f)
add_lidlc(vec3f vec3f.yaml)