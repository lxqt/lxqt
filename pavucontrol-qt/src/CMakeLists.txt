include_directories(
    ${PULSE_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

set(pavucontrol-qt_HDRS
    pavucontrol.h
    mainwindow.h
    cardwidget.h
    channel.h
    devicewidget.h
    minimalstreamwidget.h
    rolewidget.h
    sinkinputwidget.h
    sinkwidget.h
    sourceoutputwidget.h
    sourcewidget.h
    streamwidget.h
    elidinglabel.h
)

set(pavucontrol-qt_SRCS
    pavucontrol.cc
    mainwindow.cc
    cardwidget.cc
    channel.cc
    devicewidget.cc
    minimalstreamwidget.cc
    rolewidget.cc
    sinkinputwidget.cc
    sinkwidget.cc
    sourceoutputwidget.cc
    sourcewidget.cc
    streamwidget.cc
    elidinglabel.cc
)

set(pavucontrol-qt_UI
    cardwidget.ui
    devicewidget.ui
    mainwindow.ui
    streamwidget.ui
)

# Translations
lxqt_translate_ts(pavucontrol-qt_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${pavucontrol-qt_HDRS}
        ${pavucontrol-qt_SRCS}
        ${pavucontrol-qt_UI}
    INSTALL_DIR
        "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
    pavucontrol-qt.desktop.in
)

add_executable(pavucontrol-qt
    ${pavucontrol-qt_SRCS}
    ${pavucontrol-qt_QM_FILES}
    ${pavucontrol-qt_QM_LOADER}
    ${DESKTOP_FILES}
)
set_property(
     TARGET pavucontrol-qt APPEND
     PROPERTY COMPILE_DEFINITIONS
     PAVUCONTROL_QT_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}"
)

target_link_libraries(pavucontrol-qt
    Qt5::Widgets
    Qt5::DBus
    ${PULSE_LDFLAGS}
    ${GLIB_LDFLAGS}
)

install(TARGETS
    pavucontrol-qt
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)

install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
