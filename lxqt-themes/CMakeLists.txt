cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-themes)

# Minimum Versions
set(LXQTBT_MINIMUM_VERSION "0.7.0")

# lxqt-themes has no binaries - but we can set an informal version
# Version 0.15.0

find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)

include(GNUInstallDirs)
include(LXQtPreventInSourceBuilds)
include(LXQtTranslateDesktop)
include(LXQtConfigVars)

install(DIRECTORY "graphics/" DESTINATION "${LXQT_GRAPHICS_DIR}")
add_subdirectory(themes)
add_subdirectory(icons)
