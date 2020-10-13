project(lxqt-config)
add_subdirectory(qcategorizedview)
add_subdirectory(menuname)

include_directories (
    "${CMAKE_CURRENT_SOURCE_DIR}/qcategorizedview"
)

set(lxqt-config_HDRS "")

set(lxqt-config_SRCS
    main.cpp
    mainwindow.cpp
)

set(lxqt-config_MOCS
    mainwindow.h
)

set(lxqt-config_UIS
    mainwindow.ui
)

# Translations **********************************
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-config_HDRS}
        ${lxqt-config_SRCS}
        ${lxqt-config_UIS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES lxqt-config.desktop.in)

#************************************************

add_executable(lxqt-config
    ${lxqt-config_SRCS}
    ${DESKTOP_FILES}
    ${QM_FILES}
    ${QM_LOADER}
)

target_link_libraries(lxqt-config
    KF5::WindowSystem
    Qt5::Widgets
    Qt5::Xml
    lxqt
)

# helper static lib
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/qcategorizedview)
target_link_libraries(lxqt-config qcategorizedview)

install(TARGETS
    lxqt-config
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
install(FILES lxqt-config.menu
    DESTINATION "${LXQT_ETC_XDG_DIR}/menus"
    COMPONENT Runtime
)
