add_subdirectory(GSL EXCLUDE_FROM_ALL)
add_subdirectory(yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(fmt EXCLUDE_FROM_ALL)

if (BUILD_TOOLS)
  add_subdirectory(Lyra-1.3 EXCLUDE_FROM_ALL)
endif()

if (ENABLE_PYBINDGEN OR ENABLE_LIDLPY)
  add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
endif()

