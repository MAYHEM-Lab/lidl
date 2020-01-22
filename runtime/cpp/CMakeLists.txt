add_library(lidl_rt INTERFACE)
target_include_directories(lidl_rt INTERFACE include)
target_compile_features(lidl_rt INTERFACE cxx_std_17)

find_program(LIDLC_BIN
    NAMES lidlc
    HINTS ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/Debug)

if (NOT ${LIDLC_BIN} MATCHES "NOTFOUND")
    message(STATUS "Found lidlc")

    function(add_lidlc Name)
        set(LIDLC_OUTPUTS)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        foreach(FILE ${ARGN})
            get_filename_component(LIDLC_OUTPUT ${FILE} NAME_WE)
            set(LIDLC_OUTPUT
                    "${CMAKE_CURRENT_BINARY_DIR}/${LIDLC_OUTPUT}_generated.hpp")
            list(APPEND LIDLC_OUTPUTS ${LIDLC_OUTPUT})
            SET_SOURCE_FILES_PROPERTIES(${LIDLC_OUTPUT} PROPERTIES GENERATED 1)

            add_custom_command(OUTPUT ${LIDLC_OUTPUT}
                    COMMAND ${LIDLC_BIN}
                    ARGS ${FILE} ${LIDLC_OUTPUT}
                    DEPENDS ${FILE}
                    COMMENT "Building C++ header for ${FILE}"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        endforeach()
        add_custom_target(${Name}_IMPL DEPENDS ${LIDLC_OUTPUTS})

        add_library(${Name} INTERFACE)
        target_include_directories(${Name} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
        target_link_libraries(${Name} INTERFACE lidl_rt)
        add_dependencies(${Name} ${Name}_IMPL)
    endfunction()
else()
    function(add_lidlc Name)
        message(STATUS "lidlc not found!")
    endfunction()
endif()
