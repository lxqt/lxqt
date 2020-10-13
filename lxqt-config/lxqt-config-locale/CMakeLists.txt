project(lxqt-config-locale)

set(H_FILES
    localeconfig.h
)


set(CPP_FILES
    main.cpp
    localeconfig.cpp
)

set(UI_FILES
    localeconfig.ui
)

qt5_wrap_cpp(COMBO_HEADER combobox.h)

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
    ${COMBO_HEADER}
    ${RESOURCES}
    ${QRC_SOURCES}
    ${QM_FILES}
    ${DESKTOP_FILES}
    ${QM_LOADER}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Xml
    lxqt
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
