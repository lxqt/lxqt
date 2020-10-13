set(PLUGIN "directorymenu")

set(HEADERS
    directorymenu.h
    directorymenuconfiguration.h
)

set(SOURCES
    directorymenu.cpp
    directorymenuconfiguration.cpp
)

set(UIS
    directorymenuconfiguration.ui
)

set(LIBRARIES
    ${LIBRARIES}
    Qt5Xdg
)

include ("../cmake/BuildPlugin.cmake")
BUILD_LXQT_PLUGIN(${PLUGIN})
