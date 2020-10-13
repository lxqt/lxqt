set(PLUGIN "backlight")

set(HEADERS
    backlight.h
    sliderdialog.h
)

set(SOURCES
    backlight.cpp
    sliderdialog.cpp
)

set(LIBRARIES
    ${LIBRARIES}
)

BUILD_LXQT_PLUGIN(${PLUGIN})
