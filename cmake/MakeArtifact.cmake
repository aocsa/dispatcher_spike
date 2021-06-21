function (make_artifact)
    set (OPTIONS OPTIONAL VIRTUAL)
    set (ONE_VALUE_ARGS PREFIX TARGET)
    set (MULTI_VALUE_ARGS SOURCES INCLUDES LIBRARIES PUBLIC_HEADERS TESTS)
    cmake_parse_arguments (ARG "${OPTIONS}" "${ONE_VALUE_ARGS}"
            "${MULTI_VALUE_ARGS}" ${ARGN})
    if (ARG_UNPARSED_ARGUMENTS)
        message (SEND_ERROR
                "Error: unrecognized arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif ()

    if (${ARG_VIRTUAL})
        set (TYPE OBJECT)
    endif ()

    if (ARG_SOURCES)
        add_library (${ARG_TARGET} ${TYPE} ${ARG_SOURCES})
    endif()

    if (ARG_LIBRARIES)
        target_link_libraries (${ARG_TARGET} ${ARG_LIBRARIES})
    endif ()

    if (ARG_INCLUDES)
        target_include_directories (${ARG_TARGET} ${ARG_INCLUDES})
    endif ()

    if (ARG_LIBRARY_PATHS)
        target_link_libraries (${ARG_TARGET} ${ARG_LIBRARY_PATHS})
    endif ()

    install (TARGETS ${ARG_TARGET}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)

    if (ARG_PUBLIC_HEADERS)
        message (STATUS "Log: public headers ${ARG_PUBLIC_HEADERS}")
        install(DIRECTORY ${ARG_PUBLIC_HEADERS} DESTINATION include)
    endif ()

endfunction ()
