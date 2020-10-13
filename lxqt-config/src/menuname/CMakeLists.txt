file(GLOB DIRECTORY_FILES_IN desktop-directories/*.directory.in)

# Translations **********************************
lxqt_translate_desktop(DIRECTORY_FILES
    SOURCES
        ${DIRECTORY_FILES_IN}
)
add_custom_target(desktop_directories_files ALL DEPENDS ${DIRECTORY_FILES})
#************************************************

install(FILES
    ${DIRECTORY_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/desktop-directories"
    COMPONENT Runtime
)
