set(NOTIFICATIONS_SRC
    main.cpp
    notificationarea.cpp
    notification.cpp
    notificationlayout.cpp
    notificationwidgets.cpp
    notifyd.cpp
)

set(NOTIFICATIONS_UI
    notification.ui
)

qt5_add_dbus_adaptor(NOTIFICATIONS_DBUS_SRC
    org.freedesktop.Notifications.xml
    notifyd.h Notifyd
)

set_property(SOURCE ${NOTIFICATIONS_DBUS_SRC} PROPERTY SKIP_AUTOGEN ON)
list(APPEND NOTIFICATIONS_SRC "${NOTIFICATIONS_DBUS_SRC}")

# Translations **********************************
lxqt_translate_ts(NOTIFICATIONS_QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
    ${NOTIFICATIONS_SRC}
    ${NOTIFICATIONS_UI}
    INSTALL_DIR "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
#************************************************

add_executable(lxqt-notificationd
    ${NOTIFICATIONS_SRC}
    ${NOTIFICATIONS_QM_FILES}
    ${QM_LOADER}
)

target_link_libraries(lxqt-notificationd
    Qt5::Widgets
    Qt5::DBus
    lxqt
    KF5::WindowSystem
)

install(TARGETS
    lxqt-notificationd
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
