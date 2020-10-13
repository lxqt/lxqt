project("lxqt-session")

if (WITH_LIBUDEV)
    find_package(UDev REQUIRED)
    include_directories(${LIBUDEV_INCLUDE_DIR})
    add_definitions(-DWITH_LIBUDEV_MONITOR)
endif()

include_directories(
    ${X11_INCLUDE_DIR}
)

set(lxqt-session_HDRS "")

set(lxqt-session_SRCS
    src/main.cpp
    src/lxqtmodman.cpp
    src/wmselectdialog.cpp
    src/windowmanager.cpp
    src/sessionapplication.cpp
    src/sessiondbusadaptor.h
    src/lockscreenmanager.cpp
    src/lockscreenmanager.h
    src/numlock.cpp
    src/numlock.h
    src/log.cpp
)
if (WITH_LIBUDEV)
    list(APPEND lxqt-session_SRCS src/UdevNotifier.cpp)
endif()

set(lxqt-session_UI
    src/wmselectdialog.ui
)

# Translations
lxqt_translate_ts(lxqt-session_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-session_HDRS}
        ${lxqt-session_SRCS}
        ${lxqt-session_UI}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)
lxqt_app_translation_loader(lxqt-session_QM_LOADER ${PROJECT_NAME})

add_executable(lxqt-session
    ${lxqt-session_SRCS}
    ${RSCS}
    ${lxqt-session_QM_FILES}
    ${lxqt-session_QM_LOADER}
)

target_link_libraries(lxqt-session
    Qt5::Widgets
    Qt5::DBus
    Qt5::X11Extras
    lxqt
    ${X11_LIBRARIES}
    KF5::WindowSystem
)

if (WITH_LIBUDEV)
    target_link_libraries(lxqt-session ${UDEV_LIBS})
endif()

INSTALL(TARGETS
    lxqt-session
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    man/lxqt-session.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
