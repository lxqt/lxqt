cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(liblxqt)


set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(KF5_MINIMUM_VERSION "5.36.0")
set(QT_MINIMUM_VERSION "5.10.0")
set(QTXDG_MINIMUM_VERSION "3.5.0")

# Major LXQt Version, belong to all components
set(LXQT_MAJOR_VERSION 0)
# Minor LXQt Version, belong to all components
set(LXQT_MINOR_VERSION 15)
#
# Patch Version, belong *only* to the component
# LXQt is 0.13 - liblxqt is at patch version 0
# The official LXQt version will follow liblxqt.
#
# In a perfect world all components would have the same major- and minor- and
# patch-version as liblxqt - in real life it will be fine if every component
# has it's own patch version within a major/minor life cyle.
#
set(LXQT_PATCH_VERSION 1)
set(LXQT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION})


option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
option(BUILD_BACKLIGHT_LINUX_BACKEND "Build the Linux backend for the backlight" ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Widgets DBus LinguistTools)
find_package(Qt5Xdg ${QTXDG_MINIMUM_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)
if (BUILD_BACKLIGHT_LINUX_BACKEND)
    find_package(PolkitQt5-1 REQUIRED)
endif (BUILD_BACKLIGHT_LINUX_BACKEND)
if (NOT APPLE)
    find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED X11Extras)
    find_package(X11 REQUIRED)
endif()
message(STATUS "Building ${PROJECT_NAME} with Qt ${Qt5Core_VERSION}")

include(CMakePackageConfigHelpers)
include(GNUInstallDirs) # Standard directories for installation

if (BUILD_BACKLIGHT_LINUX_BACKEND)
    add_subdirectory(lxqtbacklight/linux_backend/driver)
    add_definitions(-DUSE_BACKLIGHT_LINUX_BACKEND)
endif (BUILD_BACKLIGHT_LINUX_BACKEND)

set(LXQT_PKG_CONFIG_DESCRIPTION "Shared library for LXQt applications")

set(PUB_HDRS
    lxqthtmldelegate.h
    lxqtsettings.h
    lxqtplugininfo.h
    lxqtpowermanager.h
    lxqtapplication.h
    lxqtsingleapplication.h
    lxqttranslator.h
    lxqtprogramfinder.h

    configdialog/lxqtconfigdialog.h
    configdialog/lxqtconfigdialogcmdlineoptions.h
    configdialog/lxqtpageselectwidget.h

    lxqtpower/lxqtpower.h
    lxqtnotification.h
    lxqtautostartentry.h
    lxqtgridlayout.h
    lxqtrotatedwidget.h
    lxqtglobals.h
    lxqtbacklight.h
    #lxqtbacklight/virtual_backend.h
    #lxqtbacklight/linux_backend/linuxbackend.h
)

if (NOT APPLE)
    list (APPEND PUB_HDRS
        lxqtscreensaver.h
    )
endif (NOT APPLE)

set(PUBLIC_CLASSES
    HtmlDelegate
    Settings
    PluginInfo
    PowerManager
    Application
    SingleApplication
    Translator
    ProgramFinder

    ConfigDialog
    ConfigDialogCmdLineOptions
    PageSelectWidget

    Power
    Notification
    AutostartEntry
    GridLayout
    RotatedWidget
    Globals
    Backlight
)
if (NOT APPLE)
    list (APPEND PUBLIC_CLASSES
        ScreenSaver
    )
endif (NOT APPLE)

set(SRCS
    lxqthtmldelegate.cpp
    lxqtplugininfo.cpp
    lxqtpowermanager.cpp
    lxqtsettings.cpp
    lxqtapplication.cpp
    lxqtsingleapplication.cpp
    lxqttranslator.cpp
    lxqtprogramfinder.cpp

    configdialog/lxqtconfigdialog.cpp
    configdialog/lxqtconfigdialogcmdlineoptions.cpp
    configdialog/lxqtpageselectwidget.cpp

    lxqtpower/lxqtpower.cpp
    lxqtpower/lxqtpowerproviders.cpp
    lxqtautostartentry.cpp
    lxqtnotification.cpp
    lxqtgridlayout.cpp
    lxqtrotatedwidget.cpp
    lxqtbacklight.cpp
    lxqtbacklight/virtual_backend.cpp
)

if (NOT APPLE)
    list (APPEND SRCS
        lxqtscreensaver.cpp
    )
endif (NOT APPLE)

if (BUILD_BACKLIGHT_LINUX_BACKEND)
    list (APPEND SRCS
        lxqtbacklight/linux_backend/linuxbackend.cpp
    )
endif (BUILD_BACKLIGHT_LINUX_BACKEND)

set(MOCS
    lxqthtmldelegate.h
    lxqtpowermanager.h
    lxqtsettings.h
    lxqtapplication.h
    lxqtbacklight.h

    configdialog/lxqtconfigdialog.h
    configdialog/lxqtconfigdialog_p.h
    configdialog/lxqtpageselectwidget.h

    lxqtnotification.h
    lxqtnotification_p.h
    lxqtpower/lxqtpower.h
    lxqtpower/lxqtpowerproviders.h
    lxqtgridlayout.h
    lxqtrotatedwidget.h
    lxqtbacklight/virtual_backend.h
)

if (NOT APPLE)
    list (APPEND MOCS
        lxqtscreensaver.h
    )
endif (NOT APPLE)

if (BUILD_BACKLIGHT_LINUX_BACKEND)
    list (APPEND SRCS
        lxqtbacklight/linux_backend/linuxbackend.h
    )
endif (BUILD_BACKLIGHT_LINUX_BACKEND)

set(FORMS
    configdialog/lxqtconfigdialog.ui
)

set(POLKIT_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/org.lxqt.backlight.pkexec.policy"
)

# Build Polkit file
configure_file ( polkit/org.lxqt.backlight.pkexec.policy.in "${CMAKE_CURRENT_BINARY_DIR}/org.lxqt.backlight.pkexec.policy" )

file(GLOB LXQT_CONFIG_FILES resources/*.conf)

QT5_ADD_DBUS_INTERFACE(DBUS_INTERFACE_SRCS
    dbus/org.freedesktop.Notifications.xml
    notifications_interface
)

QT5_ADD_DBUS_ADAPTOR(DBUS_ADAPTOR_SRCS
    dbus/org.lxqt.SingleApplication.xml
    lxqtsingleapplication.h LXQt::SingleApplication
)

set_property(SOURCE ${DBUS_INTERFACE_SRCS} ${DBUS_ADAPTOR_SRCS} PROPERTY SKIP_AUTOGEN ON)
list(APPEND SRCS "${DBUS_INTERFACE_SRCS}" "${DBUS_ADAPTOR_SRCS}")

# KF5WindowSystem is missing here. KF5WindowSystem doesn't provide an .pc file.
set(LXQT_PKG_CONFIG_REQUIRES "Qt5Xdg >= ${QTXDG_MINIMUM_VERSION}, Qt5Widgets >= ${QT_MINIMUM_VERSION}, Qt5Xml >= ${QT_MINIMUM_VERSION}, Qt5DBus >= ${QT_MINIMUM_VERSION}")

if (NOT APPLE)
    set(LXQT_PKG_CONFIG_REQUIRES "${LXQT_PKG_CONFIG_REQUIRES}, Qt5X11Extras >= ${QT_MINIMUM_VERSION}")
endif (NOT APPLE)


# Standard directories for installation
include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)
include(LXQtCreatePkgConfigFile)
include(LXQtCreatePortableHeaders)
include(LXQtConfigVars)

set(LXQT_INTREE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include")

set(LXQT_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/${LXQT_LIBRARY_NAME}")
set(LXQT_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/cmake")


## Translations
include(LXQtTranslateTs)

lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${SRCS}
        ${FORMS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


# Copy public headers
foreach(h ${PUB_HDRS})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} "${LXQT_INTREE_INCLUDE_DIR}/LXQt/${bh}" COPYONLY)
endforeach()

# Create the portable headers
lxqt_create_portable_headers(INTREE_PORTABLE_HEADERS
    NAME_PREFIX "lxqt"
    OUTPUT_DIR "${LXQT_INTREE_INCLUDE_DIR}/LXQt"
    HEADER_NAMES ${PUBLIC_CLASSES}
)

#check_portable_headers(H_FILES ${PUB_HDRS} LINKS "${INTREE_PORTABLE_HEADERS}")

#************************************************
# Create in-tree build infrastructure
#************************************************
set(CFG_LXQT_TARGETS_FILE   "${LXQT_INTREE_TARGETS_FILE}")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-config.cmake"
    INSTALL_DESTINATION "neverland"     # required, altough we don't install it
)

#************************************************
# Create installable build infrastructure
#************************************************
set(CFG_LXQT_TARGETS_FILE "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}/${LXQT_LIBRARY_NAME}-targets.cmake")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}-config.cmake"
    INSTALL_DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-config-version.cmake"
    VERSION ${LXQT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-config-version.cmake"
    DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}"
    COMPONENT Devel
)

install(EXPORT
    ${LXQT_LIBRARY_NAME}-targets
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LXQT_LIBRARY_NAME}"
    COMPONENT Devel
)

add_library(${LXQT_LIBRARY_NAME}
    SHARED ${PUB_HDRS}
           ${SRCS}
           ${dbus_generated}
           ${UIS} ${MOCS}
           ${QM_FILES}
           ${QRC_CXX_SOURCES}

)

target_link_libraries(${LXQT_LIBRARY_NAME}
    PUBLIC
        KF5::WindowSystem
        Qt5::Widgets
        Qt5::DBus
        Qt5Xdg
)

if (NOT APPLE)
    target_link_libraries(${LXQT_LIBRARY_NAME}
        PUBLIC
            Qt5::X11Extras
        PRIVATE
            ${X11_Xscreensaver_LIB}
            ${X11_X11_LIB}
    )
endif (NOT APPLE)

set_target_properties(${LXQT_LIBRARY_NAME} PROPERTIES
    VERSION   ${LXQT_VERSION}
    SOVERSION ${LXQT_MAJOR_VERSION}
)

target_compile_definitions(${LXQT_LIBRARY_NAME}
    PRIVATE
        "LXQT_RELATIVE_SHARE_DIR=\"${LXQT_RELATIVE_SHARE_DIR}\""
        "LXQT_SHARE_DIR=\"${LXQT_SHARE_DIR}\""
        "LXQT_RELATIVE_SHARE_TRANSLATIONS_DIR=\"${LXQT_RELATIVE_TRANSLATIONS_DIR}\""
        "LXQT_SHARE_TRANSLATIONS_DIR=\"${LXQT_TRANSLATIONS_DIR}\""
        "LXQT_GRAPHICS_DIR=\"${LXQT_GRAPHICS_DIR}\""
        "LXQT_ETC_XDG_DIR=\"${LXQT_ETC_XDG_DIR}\""
        "LXQT_DATA_DIR=\"${LXQT_DATA_DIR}\""
        "LXQT_INSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\""
        "LXQT_VERSION=\"${LXQT_VERSION}\""
        "COMPILE_LIBLXQT"
        "$<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>"
        "$<$<CONFIG:Release>:QT_NO_WARNING_OUTPUT>"
        "QT_NO_KEYWORDS"
)

target_include_directories(${LXQT_LIBRARY_NAME}
    INTERFACE
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${LXQT_LIBRARY_NAME}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${LXQT_LIBRARY_NAME}/LXQt>"
        "$<BUILD_INTERFACE:${LXQT_INTREE_INCLUDE_DIR}>"
        "$<BUILD_INTERFACE:${LXQT_INTREE_INCLUDE_DIR}/LXQt>"
)

install(TARGETS
    ${LXQT_LIBRARY_NAME}
    DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    EXPORT ${LXQT_LIBRARY_NAME}-targets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT Runtime
)

install(FILES
    ${PUB_HDRS}
    DESTINATION "${LXQT_INSTALL_INCLUDE_DIR}/LXQt"
    COMPONENT Devel
)

install(FILES
    ${INTREE_PORTABLE_HEADERS}
    DESTINATION "${LXQT_INSTALL_INCLUDE_DIR}/LXQt"
    COMPONENT Devel
)

install(FILES ${LXQT_CONFIG_FILES}
    DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/lxqt"
    COMPONENT Runtime
)

if (BUILD_BACKLIGHT_LINUX_BACKEND)
    install(FILES ${POLKIT_FILES} DESTINATION "${POLKITQT-1_POLICY_FILES_INSTALL_DIR}")
endif (BUILD_BACKLIGHT_LINUX_BACKEND)

#************************************************
# Create and install pkgconfig file
#************************************************
lxqt_create_pkgconfig_file(
    PACKAGE_NAME ${LXQT_LIBRARY_NAME}
    DESCRIPTIVE_NAME ${LXQT_LIBRARY_NAME}
    DESCRIPTION ${LXQT_PKG_CONFIG_DESCRIPTION}
    INCLUDEDIRS ${LXQT_LIBRARY_NAME}
    LIBS ${LXQT_LIBRARY_NAME}
    REQUIRES ${LXQT_PKG_CONFIG_REQUIRES}
    VERSION ${LXQT_VERSION}
    INSTALL
)
#************************************************
