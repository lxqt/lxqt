# - Find the LXQt include and library dirs and define a some macros
#
# The module defines the following functions
#
# razor_translate_ts(qm_files
#                    SOURCES sources ...
#                    [TRANSLATION_DIR] translation_directory
#                    [INSTALLATION_DIR] qm_install_directory
#                   )
#     out: qm_files
#     generates commands to create .ts.src and .qm files from sources.
#     The generated filenames can be found in qm_files.
#
#     in: sources
#     List of the h, cpp and ui files
#
#     in: translation_directory
#     A relative path to the directory with .ts files, it is relative
#     to the CMakeList.txt. By default is "translations"
#
#     in: qm_install_directory
#     A full path to the directory n which will be installed .qm files.
#     By default is "${CMAKE_INSTALL_PREFIX}/share/razor/${PROJECT_NAME}"
#
# razor_translate_desktop(desktop_files
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
#                          /etc/xdg/razor location. Unfortunately QSettings we are using internally
#                          can be overriden in the Qt compilation time to use different path for
#                          system-wide configs. (for example configure ... -sysconfdir /etc/settings ...)
#                          This path can be found calling Qt4's qmake:
#                             qmake -query QT_INSTALL_CONFIGURATION
#
#
#

include_directories(${LXQT_INCLUDE_DIRS})

#cmake_policy(SET CMP0005 NEW)
add_definitions(-DLXQT_SHARE_DIR=\"${LXQT_SHARE_DIR}\")
add_definitions(-DLXQT_ETC_XDG_DIR=\"${LXQT_ETC_XDG_DIR}\")
add_definitions(-DLXQT_VERSION=\"${LXQT_VERSION}\")

# for backward compatability ----->
add_definitions(-DRAZOR_SHARE_DIR=\"${LXQT_SHARE_DIR}\")
add_definitions(-DRAZOR_ETC_XDG_DIR=\"${LXQT_ETC_XDG_DIR}\")
add_definitions(-DRAZOR_VERSION=\"${LXQT_VERSION}\")
# for backward compatability <-----

find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

if(NOT TARGET UpdateTsFiles)
  add_custom_target(UpdateTsFiles DEPENDS)
endif()

MACRO(QT4_ADD_TRANSLATION_FIXED _qm_files)
  FOREACH (_current_FILE ${ARGN})
    GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
    GET_FILENAME_COMPONENT(qm ${_abs_FILE} NAME)
    #Extract the real extension ............
    STRING(REPLACE ".ts" "" qm ${qm})
    GET_SOURCE_FILE_PROPERTY(output_location ${_abs_FILE} OUTPUT_LOCATION)
    IF(output_location)
      FILE(MAKE_DIRECTORY "${output_location}")
      SET(qm "${output_location}/${qm}.qm")
    ELSE(output_location)
      SET(qm "${CMAKE_CURRENT_BINARY_DIR}/${qm}.qm")
    ENDIF(output_location)

    ADD_CUSTOM_COMMAND(OUTPUT ${qm}
       COMMAND ${QT_LRELEASE_EXECUTABLE}
       ARGS ${_abs_FILE} -qm ${qm}
       DEPENDS ${_abs_FILE}
    )
    SET(${_qm_files} ${${_qm_files}} ${qm})
  ENDFOREACH (_current_FILE)
ENDMACRO(QT4_ADD_TRANSLATION_FIXED)


function(razor_translate_ts _qmFiles)
    set(_translationDir "translations")
    set(_installDir "${CMAKE_INSTALL_PREFIX}/share/razor/${PROJECT_NAME}")

    # Parse arguments ***************************************
    set(_state "")
    foreach (_arg ${ARGN})
        if (
            ("${_arg}_I_HATE_CMAKE" STREQUAL "SOURCES_I_HATE_CMAKE") OR
            ("${_arg}_I_HATE_CMAKE" STREQUAL "TRANSLATION_DIR_I_HATE_CMAKE") OR
            ("${_arg}_I_HATE_CMAKE" STREQUAL "INSTALLATION_DIR_I_HATE_CMAKE") OR
            ("${_arg}_I_HATE_CMAKE" STREQUAL "TS_SRC_FILE_I_HATE_CMAKE")
           )
            set(_state ${_arg})

        else()
            if("${_state}" STREQUAL "SOURCES")
                get_filename_component (__file ${_arg} ABSOLUTE)
                set(_sources  ${_sources} ${__file})
                set(_sourcesSpace  "${_sourcesSpace} ${__file}")

            elseif("${_state}" STREQUAL "TRANSLATION_DIR")
                set(_translationDir ${_arg})
                set(_state "")

            elseif("${_state}" STREQUAL "INSTALLATION_DIR")
                set(_installDir ${_arg})
                set(_state "")

            elseif("${_state}" STREQUAL "TS_SRC_FILE")
                set(_tsSrcFile ${_arg})
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
    if ("${_tsSrcFile}" STREQUAL "")
        set(_tsSrcFile  "${_translationDir}/${PROJECT_NAME}.ts.src")
    endif()

    get_filename_component (_tsSrcFile  ${_tsSrcFile} ABSOLUTE)
    get_filename_component (_tsSrcFileName  ${_tsSrcFile} NAME)
    get_filename_component (_tsSrcFileNameWE  ${_tsSrcFile} NAME_WE)

    # TS.SRC file *******************************************
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/updateTsFile.sh
        "#/bin/sh\n"
        "\n"
        "mkdir -p ${_translationDir} 2>/dev/null\n"
        "cd ${_translationDir} && "
        "${QT_LUPDATE_EXECUTABLE} -locations none -target-language en_US ${_sourcesSpace} -ts ${_tsSrcFile}.ts &&"
        "mv ${_tsSrcFile}.ts ${_tsSrcFile}\n"
        "grep -q 'source' '${_tsSrcFile}' || rm '${_tsSrcFile}'\n"
    )

    add_custom_target(Update_${_tsSrcFileName}
        COMMAND sh ${CMAKE_CURRENT_BINARY_DIR}/updateTsFile.sh
        DEPENDS ${_sources}
        VERBATIM
    )

    add_dependencies(UpdateTsFiles Update_${_tsSrcFileName})

    # TX file ***********************************************
    set(_txFile "${CMAKE_BINARY_DIR}/tx/${_tsSrcFileName}.tx.sh")
    string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_translationDir ${_translationDir})
    string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_tsSrcFile ${_tsSrcFile})

    file(WRITE ${_txFile}
        "[ -f ${_tsSrcFile} ] || exit 0\n"
        "echo '[razor-qt.${_tsSrcFileNameWE}]'\n"
        "echo 'type = QT'\n"
        "echo 'source_lang = en'\n"
        "echo 'source_file = ${_tx_tsSrcFile}'\n"
        "echo 'file_filter = ${_tx_translationDir}/${_tsSrcFileNameWE}_<lang>.ts'\n"
        "echo ''\n"
    )

    # translate.h file *************************************
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/razortranslate.h
        "#ifndef RAZOR_TRANSLATE_H\n"
        "#include <QtCore/QLocale>\n"
        "#include <QtCore/QTranslator>\n"
        "#include <QtCore/QLibraryInfo>\n"
        "class RazorTranslator {\n"
        "public:\n"
        "  static void translate()\n"
        "  {\n"
        "    QString locale = QLocale::system().name();\n"

        "    QTranslator *qtTranslator = new QTranslator(qApp);\n"
        "    qtTranslator->load(\"qt_\" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));\n"
        "    qApp->installTranslator(qtTranslator);\n"

        "    QTranslator *appTranslator = new QTranslator(qApp);\n"
        "    appTranslator->load(QString(\"${_installDir}/${PROJECT_NAME}_%1.qm\").arg(locale));\n"
        "    qApp->installTranslator(appTranslator);\n"
        "  }\n"
        "};\n"

        "#define TRANSLATE_APP  RazorTranslator::translate();\n"
        "#endif // RAZOR_TRANSLATE_H\n"
    )

    # QM files **********************************************
    file(GLOB _tsFiles ${_translationDir}/${_tsSrcFileNameWE}_*.ts)
    QT4_ADD_TRANSLATION_FIXED(_qmFilesLocal ${_tsFiles})
    install(FILES ${_qmFilesLocal} DESTINATION ${_installDir})

    set(${_qmFiles} ${_qmFilesLocal} PARENT_SCOPE)
endfunction(razor_translate_ts)


#**********************************************************
# DESKTOP files
#**********************************************************

function(razor_translate_desktop _RESULT)
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


        # TX file ***********************************************
        set(_txFile "${CMAKE_BINARY_DIR}/tx/${_fileName}${_fileExt}.tx.sh")
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_translationDir ${_translationDir})
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_inFile ${_inFile})
        string(REPLACE "." "" _fileType ${_fileExt})

        file(WRITE ${_txFile}
            "[ -f ${_inFile} ] || exit 0\n"
            "echo '[razor-qt.${_fileName}_${_fileType}]'\n"
            "echo 'type = DESKTOP'\n"
            "echo 'source_lang = en'\n"
            "echo 'source_file = ${_tx_inFile}'\n"
            "echo 'file_filter = ${_tx_translationDir}/${_fileName}_<lang>${_fileExt}'\n"
            "echo ''\n"
        )

    endforeach()

    set(${_RESULT} ${__result} PARENT_SCOPE)
endfunction(razor_translate_desktop)


macro(razor_summary_line _RESULT _MODULE _DESCRIPTION)
    set(_str "  ${_MODULE} ")
    string(LENGTH ${_str} _len)
    while(_len LESS 30)
        set(_str "${_str} ")
        string(LENGTH ${_str} _len)
    endwhile()
    set(${_RESULT} "${_str} ${_DESCRIPTION}")
endmacro()


macro(lxqt_set_default_value VAR_NAME VAR_VALUE)
    if (NOT DEFINED ${VAR_NAME})
        set (${VAR_NAME} ${VAR_VALUE})
    endif ()
endmacro()
