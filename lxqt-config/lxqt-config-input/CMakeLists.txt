project(lxqt-config-input)

find_package(X11 REQUIRED)

if (WITH_TOUCHPAD)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XORG_LIBINPUT REQUIRED xorg-libinput)
    pkg_check_modules(XORG_XI REQUIRED xi)
    pkg_check_modules(LIBUDEV REQUIRED libudev)
endif ()

set(lxqt-config-input_INCS
    ${X11_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../liblxqt-config-cursor"
)

if (WITH_TOUCHPAD)
    add_definitions(
        "-DWITH_TOUCHPAD=\"ON\""
    )

    set(lxqt-config-input_INCS
        ${lxqt-config-input_INCS}
        ${XORG_LIBINPUT_INCLUDE_DIRS}
    )
endif ()

include_directories(
    ${lxqt-config-input_INCS}
)

set(lxqt-config-input_HDRS
    keyboardconfig.h
    mouseconfig.h
    mouseconfig.h
    keyboardlayoutconfig.h
    selectkeyboardlayoutdialog.h
)

set(lxqt-config-input_SRCS
    lxqt-config-input.cpp
    keyboardconfig.cpp
    mouseconfig.cpp
    keyboardlayoutconfig.cpp
    selectkeyboardlayoutdialog.cpp
)

set(lxqt-config-input_UIS
    mouseconfig.ui
    keyboardconfig.ui
    keyboardlayoutconfig.ui
    selectkeyboardlayoutdialog.ui
)

if (WITH_TOUCHPAD)
    set(lxqt-config-input_HDRS
        ${lxqt-config-input_HDRS}
        touchpadconfig.h
    )

    set(lxqt-config-input_SRCS
        ${lxqt-config-input_SRCS}
        touchpadconfig.cpp
        touchpaddevice.cpp
    )

    set(lxqt-config-input_UIS
        ${lxqt-config-input_UIS}
        touchpadconfig.ui
    )
endif ()

# Translations **********************************
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-config-input_HDRS}
        ${lxqt-config-input_SRCS}
        ${lxqt-config-input_UIS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
lxqt_translate_desktop(DESKTOP_FILES SOURCES ${PROJECT_NAME}.desktop.in)

#************************************************

add_definitions(
  -DPACKAGE_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/lxqt-config-input"
)

add_executable(lxqt-config-input
    ${lxqt-config-input_SRCS}
    ${DESKTOP_FILES}
    ${QM_FILES}
    ${QM_LOADER}
)

set(lxqt-config-input_TLBS
    Qt5::Widgets
    Qt5::X11Extras
    ${X11_LIBRARIES}
    lxqt
    lxqt-config-cursor
)

if (WITH_TOUCHPAD)
    set(lxqt-config-input_TLBS
        ${lxqt-config-input_TLBS}
        ${X11_Xinput_LIB}
        udev
    )
endif ()

target_link_libraries(lxqt-config-input
    ${lxqt-config-input_TLBS}
)

set_target_properties(lxqt-config-input
    PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}/${LXQT_CONFIG_PROJECT}"
)

install(TARGETS
    lxqt-config-input
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
