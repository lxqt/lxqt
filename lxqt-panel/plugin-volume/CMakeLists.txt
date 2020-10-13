set(PLUGIN "volume")

set(HEADERS
    lxqtvolume.h
    volumebutton.h
    volumepopup.h
    audiodevice.h
    lxqtvolumeconfiguration.h
    audioengine.h
)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/panel/lxqtpanelpluginconfigdialog.cpp
    lxqtvolume.cpp
    volumebutton.cpp
    volumepopup.cpp
    audiodevice.cpp
    lxqtvolumeconfiguration.cpp
    audioengine.cpp
    ossengine.cpp
)

set(UIS
    lxqtvolumeconfiguration.ui
)

set(LIBRARIES
    ${LIBRARIES}
    lxqt-globalkeys
    Qt5Xdg
)

if(PULSEAUDIO_FOUND)
    add_definitions(-DUSE_PULSEAUDIO)
    include_directories(${PULSEAUDIO_INCLUDE_DIR})
    set(HEADERS ${HEADERS} pulseaudioengine.h)
    set(SOURCES ${SOURCES} pulseaudioengine.cpp)
    set(MOCS ${MOCS} pulseaudioengine.h)
    set(LIBRARIES ${LIBRARIES} ${PULSEAUDIO_LIBRARY})
endif()

if(ALSA_FOUND)
    add_definitions(-DUSE_ALSA)
    set(HEADERS ${HEADERS} alsaengine.h alsadevice.h)
    set(SOURCES ${SOURCES} alsaengine.cpp alsadevice.cpp)
    set(MOCS ${MOCS} alsaengine.h alsadevice.h)
    set(LIBRARIES ${LIBRARIES} ${ALSA_LIBRARIES})
endif()

BUILD_LXQT_PLUGIN(${PLUGIN})
