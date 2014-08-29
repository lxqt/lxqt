set(TMP $ENV{USE_QT5})

if (TMP)
    set(_QT_VERSION "5")
else()
    set(_QT_VERSION "4")
endif()

file(WRITE use_qt_config ${_QT_VERSION})
