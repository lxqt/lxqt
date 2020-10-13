set(PLUGIN "sensors")

set(HEADERS
    lxqtsensorsplugin.h
    chip.h
    feature.h
    lxqtsensors.h
    lxqtsensorsconfiguration.h
    sensors.h
)

set(SOURCES
    lxqtsensorsplugin.cpp
    chip.cpp
    feature.cpp
    lxqtsensors.cpp
    lxqtsensorsconfiguration.cpp
    sensors.cpp
)

set(UIS
    lxqtsensorsconfiguration.ui
)

set(LIBRARIES ${SENSORS_LIB})

BUILD_LXQT_PLUGIN(${PLUGIN})
