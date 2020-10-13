project(lxqt-powermanagement)

set(QTX_LIBRARIES Qt5::Widgets Qt5::DBus Qt5::Svg Qt5::X11Extras)

set(SOURCES
    watcher.cpp
    batteryhelper.cpp
    batteryinfoframe.cpp
    batteryinfodialog.cpp
    batterywatcher.cpp
    idlenesswatcher.cpp
    lid.cpp
    lidwatcher.cpp
    main.cpp
    powermanagementd.cpp
    trayicon.cpp
    iconproducer.cpp
    powerbutton.cpp
    ../config/powermanagementsettings.cpp
)

set(UI_FILES
    batteryinfoframe.ui
    batteryinfodialog.ui
)

set(DESKTOP_FILES_IN
    lxqt-powermanagement.desktop.in
)

lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${SOURCES}
        ${UI_FILES}
    INSTALL_DIR
        ${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}
)

lxqt_app_translation_loader(lxqt-powermanagement_QM_LOADER ${PROJECT_NAME})

include_directories(
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)

add_executable(lxqt-powermanagement
    ${SOURCES}
    ${RSCS}
    ${QM_FILES}
    ${lxqt-powermanagement_QM_LOADER}
    ${DESKTOP_FILES}
)

target_link_libraries(lxqt-powermanagement
    lxqt
    ${QTX_LIBRARIES}
    KF5::Solid
    KF5::IdleTime
    lxqt-globalkeys
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
