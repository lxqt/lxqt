set(PLUGIN "worldclock")

set(HEADERS
    lxqtworldclock.h
    lxqtworldclockconfiguration.h
    lxqtworldclockconfigurationtimezones.h
    lxqtworldclockconfigurationmanualformat.h
)

set(SOURCES
    lxqtworldclock.cpp
    lxqtworldclockconfiguration.cpp
    lxqtworldclockconfigurationtimezones.cpp
    lxqtworldclockconfigurationmanualformat.cpp
)

set(UIS
    lxqtworldclockconfiguration.ui
    lxqtworldclockconfigurationtimezones.ui
    lxqtworldclockconfigurationmanualformat.ui
)

BUILD_LXQT_PLUGIN(${PLUGIN})
