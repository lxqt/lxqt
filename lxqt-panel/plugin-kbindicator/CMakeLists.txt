set(PLUGIN "kbindicator")

set(HEADERS
    src/kbdstate.h
    src/settings.h
    src/content.h
    src/kbdlayout.h
    src/kbdinfo.h
    src/kbdkeeper.h
    src/kbdwatcher.h
    src/controls.h
    src/kbdstateconfig.h
)

set(SOURCES
    kbindicator-plugin.cpp
    src/kbdstate.cpp
    src/settings.cpp
    src/content.cpp
    src/kbdkeeper.cpp
    src/kbdwatcher.cpp
    src/kbdstateconfig.cpp
)

set(UIS
    src/kbdstateconfig.ui
)

set(LIBRARIES
)

find_package(XCB REQUIRED COMPONENTS xcb xcb-xkb)
find_package(XKBCommon REQUIRED COMPONENTS XKBCommon X11)
find_package(Qt5 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS X11Extras Xml)

include_directories(${XCB_INCLUDE_DIRS})

set(HEADERS
    ${HEADERS}
    src/x11/kbdlayout.h
)

set(SOURCES
    ${SOURCES}
    src/x11/kbdlayout.cpp
)

set(LIBRARIES
    ${LIBRARIES}
    ${XCB_LIBRARIES}
    XKBCommon::XKBCommon
    XKBCommon::X11
    Qt5::Xml
)

add_definitions(-DX11_ENABLED)

BUILD_LXQT_PLUGIN(${PLUGIN})
