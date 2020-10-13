set(SOURCES
    proc.cpp
    qpsapp.cpp
    qps.cpp
    pstable.cpp
    uidstr.cpp
    ttystr.cpp
    dialogs.cpp
    infobar.cpp
    fieldsel.cpp
    wchan.cpp
    prefs.cpp
    lookup.cpp
    details.cpp
    command.cpp
    commandutils.cpp
    commandmodel.cpp
    commanddialog.cpp
    checkboxdelegate.cpp
    execwindow.cpp
    watchcond.cpp
    listmodel.cpp
    watchdogdialog.cpp
    misc.cpp
    trayicon.cpp
    htable.cpp
    qttableview.cpp
)

SET(HEADERS
    qpsapp.h
    qps.h
    pstable.h
    dialogs.h
    fieldsel.h
    prefs.h
    infobar.h
    lookup.h
    details.h
    tablefield.h
    command.h
    commandutils.h
    commandmodel.h
    commanddialog.h
    checkboxdelegate.h
    execwindow.h
    watchcond.h
    listmodel.h
    watchdogdialog.h
    misc.h
    trayicon.h
    htable.h
    qttableview.h
    svec.h
)

set(UIS
    watchdog.ui
    message.ui
)

set(RESOURCES "../qps.qrc")
#-----------------------------------------------------------------------------
# Translations
#-----------------------------------------------------------------------------
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${SOURCES} ${HEADERS} ${UIS}
    INSTALL_DIR "${QPS_TRANSLATIONS_DIR}"
)

qt5_translation_loader(
    QM_LOADER
    "${QPS_TRANSLATIONS_DIR}"
    ${PROJECT_NAME}
)

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES "${CMAKE_SOURCE_DIR}/qps.desktop.in"
)
#-----------------------------------------------------------------------------
# End of translations
#-----------------------------------------------------------------------------


add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    ${QM_FILES}
    ${QM_LOADER}
    ${DESKTOP_FILES}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    lxqt
    Qt5::Widgets
    Qt5::X11Extras
    Qt5::DBus
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
)
