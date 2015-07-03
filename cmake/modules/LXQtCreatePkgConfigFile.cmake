#=============================================================================
# Copyright 2015 Luís Pereira <luis.artur.pereira@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================#

# lxqt_create_pkgconfig_file(PACKAGE_NAME <package_name>
#                         VERSION <version>
#                         [PREFIX <path>]
#                         [EXEC_PREFIX <path>]
#                         [INCLUDEDIR_PREFIX <path>]
#                         [INCLUDEDIRS <path1> <path2> ... <path3>]
#                         [LIBDIR_PREFIX <path>]
#                         [DESCRIPTIVE_NAME <name>]
#                         [DESCRIPTION <description>]
#                         [URL <url>]
#                         [REQUIRES <dep1> <dep2> ... <dep3>]
#                         [REQUIRES_PRIVATE <dep1> <dep2> ... <dep3>]
#                         [LIB_INSTALLDIR <dir>]
#                         [CFLAGS <cflags>]
#                         [PATH <path>]
#                         [INSTALL])
#
#
# PACKAGE_NAME and VERSION are mandatory. Everything else is optional

include(CMakeParseArguments)
include(GNUInstallDirs)

function(lxqt_create_pkgconfig_file)
    set(options INSTALL)
    set(oneValueArgs
            PACKAGE_NAME
            PREFIX
            EXEC_PREFIX
            INCLUDEDIR_PREFIX
            LIBDIR_PREFIX
            DESCRIPTIVE_NAME
            DESCRIPTION
            URL
            VERSION
            PATH
    )
    set(multiValueArgs
            INCLUDEDIRS
            REQUIRES
            REQUIRES_PRIVATE
            CONFLICTS
            CFLAGS
            LIBS
            LIBS_PRIVATE
    )

    cmake_parse_arguments(USER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (USER_UNPARSED_ARGUMENTS)
      message(FATAL_ERROR "Unknown keywords given to create_pkgconfig_file(): \"${USER_UNPARSED_ARGUMENTS}\"")
    endif()

    # Check for mandatory args. Abort if not set
    if (NOT DEFINED USER_PACKAGE_NAME)
        message(FATAL_ERROR "Required argument PACKAGE_NAME missing in generate_pkgconfig_file() call")
    else()
        set(_PKGCONFIG_PACKAGE_NAME "${USER_PACKAGE_NAME}")
    endif()

    if (NOT DEFINED USER_VERSION)
        message(FATAL_ERROR "Required argument VERSION missing in generate_pkgconfig_file() call")
    else()
        set(_PKGCONFIG_VERSION "${USER_VERSION}")
    endif()


    # Optional args
    if (NOT DEFINED USER_PREFIX)
        set(_PKGCONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")
    endif()

    if (NOT DEFINED USER_EXEC_PREFIX)
        set(_PKGCONFIG_EXEC_PREFIX "\${prefix}")
    endif()

    if (NOT DEFINED USER_INCLUDEDIR_PREFIX)
        set(_PKGCONFIG_INCLUDEDIR_PREFIX "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    endif()

    if (NOT DEFINED USER_LIBDIR_PREFIX)
        set(_PKGCONFIG_LIBDIR_PREFIX "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
    endif()

    if (NOT DEFINED USER_DESCRIPTIVE_NAME)
        set(_PKGCONFIG_DESCRIPTIVE_NAME "")
    else()
        set(_PKGCONFIG_DESCRIPTIVE_NAME "${USER_DESCRIPTIVE_NAME}")
    endif()

    if (DEFINED USER_INCLUDEDIRS)
        set(tmp "")
        foreach(dir ${USER_INCLUDEDIRS})
            if (NOT IS_ABSOLUTE "${dir}")
                list(APPEND tmp "-I\${includedir}/${dir}")
            endif()
        endforeach()
        string(REPLACE ";" " " _INCLUDEDIRS "${tmp}")
    endif()

    if (DEFINED USER_REQUIRES)
        string(REPLACE ";" ", " _PKGCONFIG_REQUIRES "${USER_REQUIRES}")
    endif()

    if (DEFINED USER_REQUIRES_PRIVATE)
        string(REPLACE ";" ", " _PKGCONFIG_REQUIRES_PRIVATE "${USER_REQUIRES_PRIVATE}")
    else()
        set(_PKGCONFIG_REQUIRES_PRIVATE "")
    endif()

    if (NOT DEFINED USER_CFLAGS)
        set(_PKGCONFIG_CFLAGS "-I\${includedir} ${_INCLUDEDIRS}")
    endif()

    if (NOT DEFINED USER_LIBS)
        set(_PKGCONFIG_LIBS "-L\${libdir}")
    else()
        set(tmp "-L\${libdir}")
        set(_libs "${USER_LIBS}")
        foreach(lib ${_libs})
                list(APPEND tmp "-l${lib}")
        endforeach()
        string(REPLACE ";" " " _PKGCONFIG_LIBS "${tmp}")
    endif()

    if (NOT DEFINED USER_LIBS_PRIVATE)
        set(PKGCONFIG_LIBS "-L\${libdir}")
    else()
        set(tmp "")
        set(_libs "${USER_LIBS_PRIVATE}")
        foreach(lib ${_libs})
                list(APPEND tmp "-l${lib}")
        endforeach()
        string(REPLACE ";" " " _PKGCONFIG_LIBS_PRIVATE "${tmp}")
    endif()

    if (DEFINED USER_DESCRIPTION)
        set(_PKGCONFIG_DESCRIPTION "${USER_DESCRIPTION}")
    else()
        set(_PKGCONFIG_DESCRIPTION "")
    endif()

    if (DEFINED USER_URL)
        set(_PKFCONFIG_URL "${USER_URL}")
    else()
        set(_PKGCONFIG_URL "")
    endif()

    if (NOT DEFINED USER_PATH)
        set(_PKGCONFIG_FILE "${PROJECT_BINARY_DIR}/${_PKGCONFIG_PACKAGE_NAME}.pc")
    else()
        if (IS_ABSOLUTE "${USER_PATH}")
            set(_PKGCONFIG_FILE "${USER_PATH}/${_PKGCONFIG_PACKAGE_NAME}.pc")
        else()
            set(_PKGCONFIG_FILE "${PROJECT_BINARY_DIR}/${USER_PATH}/${_PKGCONFIG_PACKAGE_NAME}.pc")
        endif()
    endif()

    # Write the .pc file
    FILE(WRITE "${_PKGCONFIG_FILE}"
        "# file generated by create_pkgconfig_file()\n"
        "prefix=${_PKGCONFIG_PREFIX}\n"
        "exec_prefix=${_PKGCONFIG_EXEC_PREFIX}\n"
        "libdir=${_PKGCONFIG_LIBDIR_PREFIX}\n"
        "includedir=${_PKGCONFIG_INCLUDEDIR_PREFIX}\n"
        "\n"
        "Name: ${_PKGCONFIG_DESCRIPTIVE_NAME}\n"
    )

    if (NOT "${_PKGCONFIG_DESCRIPTION}" STREQUAL "")
        FILE(APPEND ${_PKGCONFIG_FILE}
            "Description: ${_PKGCONFIG_DESCRIPTION}\n"
        )
    endif()

    if (NOT "${_PKGCONFIG_URL}" STREQUAL "")
        FILE(APPEND ${_PKGCONFIG_FILE} "URL: ${_PKGCONFIG_URL}\n")
    endif()

    FILE(APPEND ${_PKGCONFIG_FILE} "Version: ${_PKGCONFIG_VERSION}\n")

    if (NOT "${_PKGCONFIG_REQUIRES}" STREQUAL "")
        FILE(APPEND ${_PKGCONFIG_FILE} "Requires: ${_PKGCONFIG_REQUIRES}\n")
    endif()

    if (NOT "${_PKGCONFIG_REQUIRES_PRIVATE}" STREQUAL "")
        FILE(APPEND ${_PKGCONFIG_FILE}
            "Requires.private: ${_PKGCONFIG_REQUIRES_PRIVATE}\n"
        )
    endif()


    FILE(APPEND ${_PKGCONFIG_FILE}
        "Cflags: ${_PKGCONFIG_CFLAGS}\n"
        "Libs: ${_PKGCONFIG_LIBS}\n"
    )

    if (NOT "${_PKGCONFIG_LIBS_PRIVATE}" STREQUAL "")
        FILE(APPEND ${_PKGCONFIG_FILE}
            "Libs.private: ${_PKGCONFIG_REQUIRES_PRIVATE}\n"
        )
    endif()

    if (DEFINED USER_INSTALL)
        # FreeBSD loves to install files to different locations
        # http://www.freebsd.org/doc/handbook/dirstructure.html
        if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
            set(_PKGCONFIG_INSTALL_DESTINATION "libdata/pkgconfig")
        else()
            set(_PKGCONFIG_INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
        endif()

        # Make the COMPONENT an parameter ?
        install(FILES "${_PKGCONFIG_FILE}"
                DESTINATION "${_PKGCONFIG_INSTALL_DESTINATION}"
                COMPONENT Devel)
    endif()
endfunction()
