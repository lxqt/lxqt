project(lxqt-config-cursor)

find_package(X11 REQUIRED)
find_package(XCB REQUIRED xcb)
include_directories(${XCB_INCLUDE_DIRS})
link_libraries(${XCB_LIBRARIES})

include_directories (
    ${X11_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/xcr
)

set(lxqt-config-cursor_HDRS
    thememodel.h
    previewwidget.h
    itemdelegate.h
    selectwnd.h
    warninglabel.h
)

set(lxqt-config-cursor_SRCS
    crtheme.cpp
    selectwnd.cpp
    xcr/xcrimg.cpp
    xcr/xcrxcur.cpp
    xcr/xcrthemefx.cpp
    xcr/xcrtheme.cpp
    xcr/xcrthemexp.cpp
    cfgfile.cpp
    previewwidget.cpp
    itemdelegate.cpp
    thememodel.cpp
    warninglabel.cpp
)

set(lxqt-config-cursor_UIS
    selectwnd.ui
    warninglabel.ui
)

find_package(ZLIB REQUIRED)

# Translations **********************************
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-config-cursor_HDRS}
        ${lxqt-config-cursor_SRCS}
        ${lxqt-config-cursor_UIS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(lxqt-config-cursor_QM_LOADER ${PROJECT_NAME})

#************************************************

add_library(lxqt-config-cursor
    SHARED
        ${lxqt-config-cursor_SRCS}
        ${DESKTOP_FILES}
        ${QM_FILES}
        ${lxqt-config-cursor_QM_LOADER}
)

target_link_libraries(lxqt-config-cursor
    Qt5::X11Extras
    Qt5::DBus
    Qt5::Xml
    ${X11_X11_LIB}
    ${X11_Xcursor_LIB}
    lxqt
    ${ZLIB_LIBRARY}
    ${X11_Xfixes_LIB}
)
# not needed probably ${X11_Xfixes_LIB})

install(TARGETS
    lxqt-config-cursor
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/${LXQT_CONFIG_PROJECT}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDOR}/applications"
    COMPONENT Runtime
)

install(FILES
    man/lxqt-config-mouse.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
