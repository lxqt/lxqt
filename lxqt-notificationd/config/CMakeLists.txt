project(lxqt-config-notificationd)

set(NOTIFICATIONS_CONF_SRC
    advancedsettings.cpp
    basicsettings.cpp
    mainwindow.cpp
    main.cpp
)

set(NOTIFICATIONS_CONF_UI
    advancedsettings.ui
    basicsettings.ui
)


# Translations **********************************
lxqt_translate_ts(NOTIFICATIONS_CONF_QM
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
    ${NOTIFICATIONS_CONF_MOC}
    ${NOTIFICATIONS_CONF_SRC}
    ${NOTIFICATIONS_CONF_UI}
    INSTALL_DIR "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES lxqt-config-notificationd.desktop.in)
#************************************************


add_executable(${PROJECT_NAME}
    ${NOTIFICATIONS_CONF_SRC}
    ${NOTIFICATIONS_CONF_QM}
    ${DESKTOP_FILES}
    ${QM_LOADER}
)

target_link_libraries(${PROJECT_NAME}
    KF5::WindowSystem
    Qt5::Widgets
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
