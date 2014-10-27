#=============================================================================
# The lxqt_translate_desktop() function was copied from the the
# LXQt LxQtTranste.cmake
#
# Original Author: Alexander Sokolov <sokoloff.a@gmail.com>
#
#=============================================================================

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


        # TX file ***********************************************
        set(_txFile "${CMAKE_BINARY_DIR}/tx/${_fileName}${_fileExt}.tx.sh")
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_translationDir ${_translationDir})
        string(REPLACE "${CMAKE_SOURCE_DIR}/" "" _tx_inFile ${_inFile})
        string(REPLACE "." "" _fileType ${_fileExt})

        file(WRITE ${_txFile}
            "[ -f ${_inFile} ] || exit 0\n"
            "echo '[lxde-qt.${_fileName}_${_fileType}]'\n"
            "echo 'type = DESKTOP'\n"
            "echo 'source_lang = en'\n"
            "echo 'source_file = ${_tx_inFile}'\n"
            "echo 'file_filter = ${_tx_translationDir}/${_fileName}_<lang>${_fileExt}'\n"
            "echo ''\n"
        )

    endforeach()

    set(${_RESULT} ${__result} PARENT_SCOPE)
endfunction(lxqt_translate_desktop)
