cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-config)

set(LXQT_CONFIG_PROJECT "${PROJECT_NAME}")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
option(WITH_APPEARANCE "Build the 'lxqt-config-appearance'" ON)
option(WITH_BRIGHTNESS "Build the 'lxqt-config-brightness'" ON)
option(WITH_FILE_ASSOCIATIONS "Build the 'lxqt-config-file-associations'" ON)
option(WITH_INPUT "Build the 'lxqt-config-input'" ON)
option(WITH_LOCALE "Build the 'lxqt-config-locale'" ON)
option(WITH_MONITOR "Build the 'lxqt-config-monitor'" ON)
option(WITH_TOUCHPAD "Build touchpad settings" ON)

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(KF5_MINIMUM_VERSION "5.36.0")
set(KF5SCREEN_MINIMUM_VERSION "5.2.0")
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(Qt5DBus ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Xml ${QT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# Patch Version
set(LXQT_CONFIG_PATCH_VERSION 0)

set(LXQT_CONFIG_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_CONFIG_PATCH_VERSION})
add_definitions(
    "-DLXQT_CONFIG_VERSION=\"${LXQT_CONFIG_VERSION}\""
)

include(LXQtTranslate)

add_subdirectory(src)
if (WITH_INPUT OR WITH_APPEARANCE)
    add_subdirectory(liblxqt-config-cursor)
endif()
if (WITH_INPUT)
    add_subdirectory(lxqt-config-input)
endif()
if (WITH_FILE_ASSOCIATIONS)
    add_subdirectory(lxqt-config-file-associations)
endif()
if (WITH_APPEARANCE)
    add_subdirectory(lxqt-config-appearance)
endif()
if (WITH_MONITOR)
    add_subdirectory(lxqt-config-monitor)
endif()
if (WITH_BRIGHTNESS)
    add_subdirectory(lxqt-config-brightness)
endif()
if (WITH_LOCALE)
    add_subdirectory(lxqt-config-locale)
endif()

install(FILES
    man/lxqt-config.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
