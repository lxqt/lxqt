set(PLUGIN "taskbar")

set(HEADERS
    lxqttaskbar.h
    lxqttaskbutton.h
    lxqttaskbarconfiguration.h
    lxqttaskbarplugin.h
    lxqttaskgroup.h
    lxqtgrouppopup.h
)

set(SOURCES
    lxqttaskbar.cpp
    lxqttaskbutton.cpp
    lxqttaskbarconfiguration.cpp
    lxqttaskbarplugin.cpp
    lxqttaskgroup.cpp
    lxqtgrouppopup.cpp
)

set(UIS
    lxqttaskbarconfiguration.ui
)

set(LIBRARIES
    lxqt
    lxqt-globalkeys
    Qt5Xdg
)

BUILD_LXQT_PLUGIN(${PLUGIN})
