# - Find the LXQt include and library dirs and define a some macros
#
# The module defines the following functions
#
# lxqt_translate_desktop(desktop_files
#                         SOURCES sources ...
#                         [TRANSLATION_DIR] translation_directory
#                        )
#     out: desktop_files
#     generates commands to create .desktop files from sources.
#     The generated filenames can be found in desktop_files.
#
#     in: sources
#     List of the desktop.in files
#
#     in: translation_directory
#     A relative path to the directory with translations files, it is
#     relative to the CMakeList.txt. By default is "translations"
#
#
# lxqt_set_default_value(VAR_NAME VAR_VALUE)
#
#
#
# The module defines the following definitions
#
#  -DLXQT_SHARE_DIR      - This allows to install and read the configs from non-standard locations
#
#  -DLXQT_ETC_XDG_DIR    - XDG standards expects system-wide configuration files in the
#                          /etc/xdg/lxqt location. Unfortunately QSettings we are using internally
#                          can be overriden in the Qt compilation time to use different path for
#                          system-wide configs. (for example configure ... -sysconfdir /etc/settings ...)
#                          This path can be found calling Qt4's qmake:
#                             qmake -query QT_INSTALL_CONFIGURATION
#
#
#

include_directories(${LXQT_INCLUDE_DIRS})

link_directories(${LXQT_LIBRARY_DIRS})

#cmake_policy(SET CMP0005 NEW)
add_definitions(-DLXQT_SHARE_DIR=\"${LXQT_SHARE_DIR}\")
add_definitions(-DLXQT_ETC_XDG_DIR=\"${LXQT_ETC_XDG_DIR}\")
add_definitions(-DLXQT_VERSION=\"${LXQT_VERSION}\")

# for backward compatability ----->
add_definitions(-DLXQT_SHARE_DIR=\"${LXQT_SHARE_DIR}\")
add_definitions(-DLXQT_ETC_XDG_DIR=\"${LXQT_ETC_XDG_DIR}\")
add_definitions(-DLXQT_VERSION=\"${LXQT_VERSION}\")
# for backward compatability <-----

find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

#**********************************************************
# DESKTOP files
#**********************************************************

function(lxqt_translate_desktop _RESULT)
    set(_translationDir "translations")

    # Parse arguments ***************************************
    set(_state "")
    foreach (_arg ${ARGN})
        if (
            ("${_arg}_I_HATE_CMAKE" STREQUAL "SOURCES_I_HATE_CMAKE") OR
            ("${_arg}_I_HATE_CMAKE" STREQUAL "TRANSLATION_DIR_I_HATE_CMAKE")
           )

            set(_state ${_arg})

        else()
            if("${_state}" STREQUAL "SOURCES")
                get_filename_component (__file ${_arg} ABSOLUTE)
                set(_sources  ${_sources} ${__file})
                #set(_sources  ${_sources} ${_arg})

            elseif("${_state}" STREQUAL "TRANSLATION_DIR")
                set(_translationDir ${_arg})
                set(_state "")

            else()
                MESSAGE(FATAL_ERROR
                  "Unknown argument '${_arg}'.\n"
                  "See ${CMAKE_CURRENT_LIST_FILE} for more information.\n"
                )
            endif()
        endif()
    endforeach(_arg)

    get_filename_component (_translationDir ${_translationDir} ABSOLUTE)

    foreach (_inFile ${_sources})
        get_filename_component(_inFile   ${_inFile} ABSOLUTE)
        get_filename_component(_fileName ${_inFile} NAME_WE)
        #Extract the real extension ............
        get_filename_component(_fileExt  ${_inFile} EXT)
        string(REPLACE ".in" "" _fileExt ${_fileExt})
        #.......................................
        set(_outFile "${CMAKE_CURRENT_BINARY_DIR}/${_fileName}${_fileExt}")

        file(GLOB _translations
            ${_translationDir}/${_fileName}_*${_fileExt}
            ${_translationDir}/local/${_fileName}_*${_fileExt}
        )

        set(_pattern "'\\[.*]\\s*='")
        if (_translations)
            add_custom_command(OUTPUT ${_outFile}
                COMMAND grep -v "'#TRANSLATIONS_DIR='" ${_inFile} > ${_outFile}
                COMMAND grep --no-filename ${_pattern} ${_translations} >> ${_outFile}
                COMMENT "Generating ${_fileName}${_fileExt}"
            )
        else()
            add_custom_command(OUTPUT ${_outFile}
                COMMAND grep -v "'#TRANSLATIONS_DIR='" ${_inFile} > ${_outFile}
                COMMENT "Generating ${_fileName}${_fileExt}"
            )
        endif()

        set(__result ${__result} ${_outFile})
    endforeach()

    set(${_RESULT} ${__result} PARENT_SCOPE)
endfunction()


macro(lxqt_set_default_value VAR_NAME VAR_VALUE)
    if (NOT DEFINED ${VAR_NAME})
        set (${VAR_NAME} ${VAR_VALUE})
    endif ()
endmacro()
