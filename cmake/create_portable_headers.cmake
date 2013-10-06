# Creates portable headers; e.g.:
#     Creates XdgAction from xdgaction.h
#     XdgAction contents:
#     #include "xdgaction.h"
#
# Use:
# set(PUBLIC_CLASSES MyClass YourClass)
# create_portable_headers(PORTABLE_HEADERS ${PUBLIC_CLASSES})
# PORTABLE_HEADER is an return value that contains the full name of the
#   generated headers.

function(create_portable_headers outfiles)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)

    cmake_parse_arguments(_CREATE_PORTABLE_HEADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(class_list ${_CREATE_PORTABLE_HEADERS_UNPARSED_ARGUMENTS})
    foreach(f ${class_list})
        string(TOLOWER "${f}.h" _filename)

        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/portableHeders/${f}
            "#include \"${_filename}\"")

        list(APPEND ${outfiles} ${CMAKE_CURRENT_BINARY_DIR}/portableHeders/${f})
    endforeach()

    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

