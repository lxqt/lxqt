set(PLUGIN "statusnotifier")


find_package(dbusmenu-qt5 REQUIRED)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Concurrent)

set(HEADERS
    statusnotifier.h
    statusnotifierconfiguration.h
    dbustypes.h
    statusnotifierbutton.h
    statusnotifieriteminterface.h
    statusnotifierwatcher.h
    statusnotifierwidget.h
    sniasync.h
)

set(SOURCES
    statusnotifier.cpp
    statusnotifierconfiguration.cpp
    dbustypes.cpp
    statusnotifierbutton.cpp
    statusnotifieriteminterface.cpp
    statusnotifierwatcher.cpp
    statusnotifierwidget.cpp
    sniasync.cpp
)

set(UIS
    statusnotifierconfiguration.ui
)

qt5_add_dbus_adaptor(DBUS_SOURCES
    org.kde.StatusNotifierItem.xml
    statusnotifieriteminterface.h
    StatusNotifierItemInterface
)

set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

list(APPEND SOURCES "${DBUS_SOURCES}")

set(LIBRARIES
    dbusmenu-qt5
    Qt5::Concurrent
)

BUILD_LXQT_PLUGIN(${PLUGIN})
