function(create_translatorsinfo_file _infoFile)
    file(GLOB IN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translatorsinfo/*.info")

    set(_outFile "${CMAKE_CURRENT_BINARY_DIR}/translators.info")
    file(WRITE "${_outFile}" "")

    foreach (_file ${IN_FILES})
        get_filename_component(_name "${_file}" NAME)
        file(APPEND "${_outFile}" "[${_name}]\n")

        file(READ "${_file}" _content)
        file(APPEND "${_outFile}" "${_content}\n")
    endforeach(_file)

    set("${_infoFile}" "${_outFile}" PARENT_SCOPE)
endfunction()

function(get_translatorsinfo_qrc _qrcFile)
    create_translatorsinfo_file(TRANSLATORS_INFO_FILE)
    configure_file(
        translatorsinfo/translatorsinfo.qrc.in
        "${CMAKE_CURRENT_BINARY_DIR}/translatorsinfo.qrc"
    )
    set(${_qrcFile} "${CMAKE_CURRENT_BINARY_DIR}/translatorsinfo.qrc" PARENT_SCOPE)
endfunction()
