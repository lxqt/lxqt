set(PLUGIN "showdesktop")

set(HEADERS
    showdesktop.h
)

set(SOURCES
    showdesktop.cpp
)

set(LIBRARIES
    ${LIBRARIES}
    lxqt-globalkeys
    Qt5Xdg
)

BUILD_LXQT_PLUGIN(${PLUGIN})
