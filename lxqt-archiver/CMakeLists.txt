cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-archiver)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)


set(GLIB_MINIMUM_VERSION "2.50.0")
set(LIBFMQT_MINIMUM_VERSION "0.15.0")
set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(QT_MINIMUM_VERSION "5.10.0")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)


find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)  # seems to be required by fm-qt?
find_package(fm-qt ${LIBFMQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)

find_package(GLIB ${GLIB_MINIMUM_VERSION} REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(
    GLIB_JSON
    json-glib-1.0
    REQUIRED
)

set(LXQT_ARCHIVER_MAJOR_VERSION 0)
set(LXQT_ARCHIVER_MINOR_VERSION 2)
set(LXQT_ARCHIVER_PATCH_VERSION 0)
set(LXQT_ARCHIVER_VERSION ${LXQT_ARCHIVER_MAJOR_VERSION}.${LXQT_ARCHIVER_MINOR_VERSION}.${LXQT_ARCHIVER_PATCH_VERSION})
add_definitions("-DLXQT_ARCHIVER_VERSION=\"${LXQT_ARCHIVER_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)
include(LXQtTranslate)

add_subdirectory(src)
