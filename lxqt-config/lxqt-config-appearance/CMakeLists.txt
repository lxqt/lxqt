project(lxqt-config-appearance)

include_directories(
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../liblxqt-config-cursor"
)

set(H_FILES
    iconthemeinfo.h
)

set(MOC_FILES
    iconthemeconfig.h
    lxqtthemeconfig.h
    fontsconfig.h
    styleconfig.h
    fontconfigfile.h
    configothertoolkits.h
)

set(CPP_FILES
    main.cpp
    iconthemeconfig.cpp
    iconthemeinfo.cpp
    lxqtthemeconfig.cpp
    fontsconfig.cpp
    styleconfig.cpp
    fontconfigfile.cpp
    configothertoolkits.cpp
    colorLabel.cpp
)

set(UI_FILES
    iconthemeconfig.ui
    lxqtthemeconfig.ui
    fontsconfig.ui
    styleconfig.ui
)

set(QRC_FILES "")

set(LIBRARIES
    lxqt
)


# Translations **********************************
lxqt_translate_ts(QM_FILES
        UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
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
    Qt5::Widgets
    Qt5::X11Extras
    Qt5::Xml
    ${LIBRARIES}
    lxqt-config-cursor
)

set_target_properties("${PROJECT_NAME}"
    PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/${LXQT_CONFIG_PROJECT}"
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

install(FILES
    man/lxqt-config-appearance.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
