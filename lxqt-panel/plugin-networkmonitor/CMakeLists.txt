set(PLUGIN "networkmonitor")

set(HEADERS
    lxqtnetworkmonitorplugin.h
    lxqtnetworkmonitor.h
    lxqtnetworkmonitorconfiguration.h
)

set(SOURCES
    lxqtnetworkmonitorplugin.cpp
    lxqtnetworkmonitor.cpp
    lxqtnetworkmonitorconfiguration.cpp
)

set(UIS
    lxqtnetworkmonitorconfiguration.ui
)

set(RESOURCES
    resources.qrc
)

set(LIBRARIES ${STATGRAB_LIB})

BUILD_LXQT_PLUGIN(${PLUGIN})
