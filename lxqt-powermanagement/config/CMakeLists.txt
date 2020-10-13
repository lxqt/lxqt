project(lxqt-config-powermanagement)

set(QTX_LIBRARIES Qt5::Widgets Qt5::DBus Qt5::Svg)

set(SOURCES
    main.cpp
    mainwindow.cpp
    lidwatchersettings.cpp
    batterywatchersettings.cpp
    idlenesswatchersettings.cpp
    powermanagementsettings.cpp
    powerkeyssettings.cpp
    helpers.cpp
    ../src/iconproducer.cpp
)

set(UI_FILES
    lidwatchersettings.ui
    batterywatchersettings.ui
    idlenesswatchersettings.ui
    powerkeyssettings.ui
)

set(ICON_FILES
    icons/laptop-lid.svg
)

set(DESKTOP_FILES_IN
    lxqt-config-powermanagement.desktop.in
)

# Translations **********************************

lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${SOURCES}
        ${UI_FILES}
    INSTALL_DIR
        ${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}
)

lxqt_app_translation_loader(config-powermanagement_QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES ${DESKTOP_FILES_IN})

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${DESKTOP_FILES}
    ${QM_FILES}
    ${config-powermanagement_QM_LOADER}
)

target_link_libraries(lxqt-config-powermanagement
    lxqt
    ${QTX_LIBRARIES}
    KF5::Solid
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
    ${ICON_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/devices"
    COMPONENT Runtime
)
