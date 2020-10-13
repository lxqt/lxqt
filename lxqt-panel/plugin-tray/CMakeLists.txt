set(PLUGIN "tray")

include(CheckLibraryExists)

find_package(XCB REQUIRED COMPONENTS xcb xcb-util xcb-damage)
find_package(PkgConfig)

find_package(X11 REQUIRED)
pkg_check_modules(XCOMPOSITE REQUIRED xcomposite)
pkg_check_modules(XDAMAGE REQUIRED xdamage)
pkg_check_modules(XRENDER REQUIRED xrender)

set(HEADERS
    lxqttrayconfiguration.h
    lxqttrayplugin.h
    lxqttray.h
    trayicon.h
    xfitman.h
)

set(SOURCES
    lxqttrayconfiguration.cpp
    lxqttrayplugin.cpp
    lxqttray.cpp
    trayicon.cpp
    xfitman.cpp
)

set(UIS
    lxqttrayconfiguration.ui
)

set(LIBRARIES
    ${X11_LIBRARIES}
    ${XCOMPOSITE_LDFLAGS}
    ${XDAMAGE_LIBRARIES}
    ${XRENDER_LIBRARIES}
    ${XCB_LIBRARIES}
)

BUILD_LXQT_PLUGIN(${PLUGIN})
