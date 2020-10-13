project(lxqt-config-file-associations)

set(H_FILES
    mimetypedata.h
    mimetypeviewer.h
    applicationchooser.h
)

set(MOC_FILES
    mimetypedata.h
    mimetypeviewer.h
    applicationchooser.h
)

set(CPP_FILES
    mimetypedata.cpp
    mimetypeviewer.cpp
    applicationchooser.cpp
    main.cpp
)

set(UI_FILES
    mimetypeviewer.ui
    applicationchooser.ui
)

set(QRC_FILES "")

set(LIBRARIES
    lxqt
)

# Translations **********************************
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${H_FILES}
        ${CPP_FILES}
        ${UI_FILES}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES ${PROJECT_NAME}.desktop.in)

#************************************************

add_executable(${PROJECT_NAME}
    ${CPP_FILES}
    ${RESOURCES}
    ${QRC_FILES}
    ${QM_FILES}
    ${QM_LOADER}
    ${DESKTOP_FILES}
)

target_link_libraries(${PROJECT_NAME}
    KF5::WindowSystem
    Qt5::Xml
    Qt5::DBus
    Qt5::Widgets
    ${LIBRARIES}
)

install(TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
