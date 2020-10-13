set(PLUGIN "cpuload")

set(HEADERS
    lxqtcpuloadplugin.h
    lxqtcpuload.h
    lxqtcpuloadconfiguration.h
)

set(SOURCES
    lxqtcpuloadplugin.cpp
    lxqtcpuload.cpp
    lxqtcpuloadconfiguration.cpp
)

set(UIS
    lxqtcpuloadconfiguration.ui
)

set(LIBRARIES ${STATGRAB_LIB})

BUILD_LXQT_PLUGIN(${PLUGIN})
