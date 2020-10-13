project(lxqt-config-brightness)

find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(XCB REQUIRED xcb xcb-randr)

set(QTX_LIBRARIES Qt5::Widgets Qt5::X11Extras)


include_directories (
    ${XCB_INCLUDE_DIRS}
)

set(H_FILES
    brightnesswatcher.h
    brightnesssettings.h
	xrandrbrightness.h
    monitorinfo.h
    outputwidget.h
)

set(CPP_FILES
    main.cpp
    brightnesswatcher.cpp
    brightnesssettings.cpp
    xrandrbrightness.cpp
    monitorinfo.cpp
    outputwidget.cpp
)

set(UI_FILES
    brightnesssettings.ui
    outputwidget.ui
)

qt5_wrap_ui(UI_HEADERS ${UI_FILES})


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
lxqt_translate_desktop(DESKTOP_FILES SOURCES "resources/${PROJECT_NAME}.desktop.in")


add_executable(${PROJECT_NAME} ${CPP_FILES} ${UI_FILES} ${QM_FILES} ${DESKTOP_FILES} ${QM_LOADER})

target_link_libraries(
    ${PROJECT_NAME}
    ${QTX_LIBRARIES}
    ${XCB_LIBRARIES}
    lxqt
)

add_definitions(
    -DICON_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/48x48/apps"
)


install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${DESKTOP_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
install(
    FILES icons/brightnesssettings.svg
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps"
)
    
