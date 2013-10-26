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

function(create_portable_headers outfiles outDir)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs)

    cmake_parse_arguments(_CREATE_PORTABLE_HEADERS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(class_list ${_CREATE_PORTABLE_HEADERS_UNPARSED_ARGUMENTS})
    foreach(f ${class_list})
        string(TOLOWER "${f}.h" _filename)

        file(WRITE ${outDir}/${f}
            "#include \"lxqt${_filename}\"\n")

        list(APPEND ${outfiles} ${outDir}/${f})
    endforeach()

    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()



function(check_portable_headers)
    cmake_parse_arguments(__ARGS "" "" "H_FILES;LINKS" ${ARGN})

    foreach(f ${__ARGS_LINKS})
        file(READ ${f} content)

        set(found False)
        foreach(line ${content})
            string(REGEX MATCH "#include \"(.*)\"" v ${line})
            set(hFile ${CMAKE_MATCH_1})

            string(REGEX MATCH "[;/]${hFile};" v ";${__ARGS_H_FILES};")

            if(NOT v)
                set(found True)
            endif()
        endforeach()


        if(found)
            message(FATAL_ERROR "Incorrect portable header: '${f}'")
        endif()

    endforeach()
endfunction()

