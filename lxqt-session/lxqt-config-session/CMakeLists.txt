project("lxqt-config-session")


set(lxqt-sessioncfg_HDRS
    sessionconfigwindow.h
    basicsettings.h
    modulemodel.h
    autostartpage.h
    autostartmodel.h
    autostartitem.h
    autostartedit.h
    autostartutils.h
    defaultappspage.h
    environmentpage.h
    userlocationspage.h
)

set(lxqt-sessioncfg_SRCS
    main.cpp
    sessionconfigwindow.cpp
    basicsettings.cpp
    modulemodel.cpp
    autostartpage.cpp
    autostartmodel.cpp
    autostartitem.cpp
    autostartedit.cpp
    autostartutils.cpp
    defaultappspage.cpp
    environmentpage.cpp
    userlocationspage.cpp
    ../lxqt-session/src/windowmanager.cpp
)

set(lxqt-sessioncfg_UI_FILES
    basicsettings.ui
    autostartpage.ui
    autostartedit.ui
    defaultappspage.ui
    environmentpage.ui
)

# Translations
lxqt_translate_ts(lxqt-sessioncfg_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-sessioncfg_HDRS}
        ${lxqt-sessioncfg_SRCS}
        ${lxqt-sessioncfg_UI_FILES}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(lxqt-sessioncfg_QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(lxqt-sessioncfg_DESKTOP_FILES SOURCES lxqt-config-session.desktop.in)

add_executable(lxqt-config-session
    ${lxqt-sessioncfg_SRCS}
    ${lxqt-sessioncfg_DESKTOP_FILES}
    ${lxqt-sessioncfg_QM_FILES}
    ${lxqt-sessioncfg_QM_LOADER})

target_link_libraries(lxqt-config-session
    KF5::WindowSystem
    Qt5::Widgets
    Qt5::DBus
    Qt5::X11Extras
    lxqt
)

INSTALL(TARGETS
    lxqt-config-session
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)

install(FILES
        ${lxqt-sessioncfg_DESKTOP_FILES}
    DESTINATION
         "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
install(FILES
    man/lxqt-config-session.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
