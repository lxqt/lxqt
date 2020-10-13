project(lxqt-config-monitor)

find_package(KF5Screen ${KF5SCREEN_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Svg ${QT_MINIMUM_VERSION} REQUIRED)

set(HEADERS
    monitorsettingsdialog.h
    monitor.h
    monitorwidget.h
    monitorpicture.h
    loadsettings.h
    settingsdialog.h
    managesavedsettings.h
    fastmenu.h
    kscreenutils.h
)

set(SOURCES
    main.cpp
    monitorsettingsdialog.cpp
    monitor.cpp
    monitorwidget.cpp
    timeoutdialog.cpp
    monitorpicture.cpp
    loadsettings.cpp
    settingsdialog.cpp
    managesavedsettings.cpp
    fastmenu.cpp
    kscreenutils.cpp
)

set(UIS
    monitorsettingsdialog.ui
    monitorwidget.ui
    timeoutdialog.ui
    monitorpicture.ui
    managesavedsettings.ui
    fastmenu.ui
)

set(ICONS
	icons/monitor.svg
)

# Config file
configure_file(resources/configure.in configure.h)

# Translations **********************************
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${HEADERS}
        ${SOURCES}
        ${UIS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES "resources/${PROJECT_NAME}.desktop.in")

#************************************************

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${RESOURCES}
    ${QRC_SOURCES}
    ${DESKTOP_FILES}
    ${QM_FILES}
    ${QM_LOADER}
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::X11Extras
    Qt5::Svg
    KF5::Screen
    lxqt
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/applications")
install(FILES ${ICONS} DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/lxqt/icons")

