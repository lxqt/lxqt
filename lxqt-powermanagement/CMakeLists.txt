cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-powermanagement)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

set(KF5_MINIMUM_VERSION "5.36.0")
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(Qt5DBus ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Svg ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(KF5IdleTime ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(KF5Solid ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys-ui ${LXQT_GLOBALKEYS_MINIMUM_VERSION} REQUIRED)

message(STATUS "Building with Qt${Qt5Core_VERSION}")

# Patch Version
set(LXQT_POWERMANAGEMENT_PATCH_VERSION 0)

set(LXQT_POWERMANAGEMENT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_POWERMANAGEMENT_PATCH_VERSION})
add_definitions(
    "-DLXQT_POWERMANAGEMENT_VERSION=\"${LXQT_POWERMANAGEMENT_VERSION}\""
)

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# Translations **********************************
include(LXQtTranslate)

add_subdirectory(config)
add_subdirectory(src)
add_subdirectory(autostart)
