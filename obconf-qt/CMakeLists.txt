cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(obconf-qt)

find_program(SED_PROGRAM sed)

include(GNUInstallDirs)


# Minimum Versions
set(GLIB_MINIMUM_VERSION "2.50.0")
set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(QT_MINIMUM_VERSION "5.10.0")

set(OBCONFQT_MAJOR_VERSION 0)
set(OBCONFQT_MINOR_VERSION 15)
set(OBCONFQT_PATCH_VERSION 0)
set(OBCONFQT_VERSION ${OBCONFQT_MAJOR_VERSION}.${OBCONFQT_MINOR_VERSION}.${OBCONFQT_PATCH_VERSION})
add_definitions("-DOBCONFQT_VERSION=\"${OBCONFQT_VERSION}\"")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
message(STATUS "Building with Qt${Qt5Core_VERSION}")

find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)

find_package(GLIB ${GLIB_MINIMUM_VERSION} REQUIRED)

find_package(PkgConfig)
pkg_check_modules(OPENBOX REQUIRED
  obrender-3.5
  obt-3.5
)

include(LXQtPreventInSourceBuilds)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# add src subdirectory
add_subdirectory(src)

# install an icon for the application
install(
  FILES data/obconf-qt.png
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps"
)
