set(PLUGIN "desktopswitch")

set(HEADERS
    desktopswitch.h
    desktopswitchbutton.h
    desktopswitchconfiguration.h
)

set(SOURCES
    desktopswitch.cpp
    desktopswitchbutton.cpp
    desktopswitchconfiguration.cpp
)

set(UIS
    desktopswitchconfiguration.ui
)


set(LIBRARIES ${LIBRARIES} lxqt lxqt-globalkeys)

BUILD_LXQT_PLUGIN(${PLUGIN})
