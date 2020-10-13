set(PLUGIN "quicklaunch")

set(HEADERS
    lxqtquicklaunchplugin.h
    lxqtquicklaunch.h
    quicklaunchbutton.h
    quicklaunchaction.h
)

set(SOURCES
    lxqtquicklaunchplugin.cpp
    lxqtquicklaunch.cpp
    quicklaunchbutton.cpp
    quicklaunchaction.cpp
)

set(LIBRARIES
    Qt5Xdg
)

include_directories(
    ${LXQT_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/../panel"
)

BUILD_LXQT_PLUGIN(${PLUGIN})
