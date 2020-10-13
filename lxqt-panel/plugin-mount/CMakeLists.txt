set(PLUGIN "mount")

set(HEADERS
    lxqtmountplugin.h
    configuration.h
    button.h
    menudiskitem.h
    popup.h
    actions/deviceaction.h
    actions/deviceaction_info.h
    actions/deviceaction_menu.h
    actions/deviceaction_nothing.h
)

set(SOURCES
    lxqtmountplugin.cpp
    configuration.cpp
    button.cpp
    menudiskitem.cpp
    popup.cpp
    actions/deviceaction.cpp
    actions/deviceaction_info.cpp
    actions/deviceaction_menu.cpp
    actions/deviceaction_nothing.cpp
)

set(UIS
    configuration.ui
)

find_package(KF5Solid ${QT_MINIMUM_VERSION} REQUIRED)
set(LIBRARIES Qt5Xdg KF5::Solid)

BUILD_LXQT_PLUGIN(${PLUGIN})
