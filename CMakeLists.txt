cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-build-tools)

option(WITH_XDG_DIRS_FALLBACK "Use our XDG_CONFIG_DIRS fallback" ON)

set(LXQT_BUILD_TOOLS_MAJOR_VERSION 0)
set(LXQT_BUILD_TOOLS_MINOR_VERSION 6)
set(LXQT_BUILD_TOOLS_PATCH_VERSION 0)
set(LXQT_BUILD_TOOLS_VERSION ${LXQT_BUILD_TOOLS_MAJOR_VERSION}.${LXQT_BUILD_TOOLS_MINOR_VERSION}.${LXQT_BUILD_TOOLS_PATCH_VERSION})


# Check for needed versions
# We need at least Qt 5.7.1 and glib-2.0 >= 2.50 to build all LXQt parts
find_package(PkgConfig REQUIRED)
find_package(Qt5Core "5.7.1"  REQUIRED)
pkg_check_modules(GLIB2 glib-2.0>=2.50 REQUIRED)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs) # Standard directories for installation
include(cmake/FindInstallConfigPath.cmake) # sets LXQT_ETC_XDG_DIR, if unset
include(cmake/modules/LXQtPreventInSourceBuilds.cmake)

#-----------------------------------------------------------------------------
# LXQT_DATA_DIR defaults to CMAKE_INSTALL_FULL_DATADIR. It's added to
#   XDG_DATA_DIRS by the startlxqt script
# Warning: Setting LXQT_DATA_DIR must be done after including GNUInstallDirs
#-----------------------------------------------------------------------------
if (NOT DEFINED LXQT_DATA_DIR)
    set(LXQT_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}" CACHE PATH
        "LXQt base directory relative to which data files should be searched"
    )
endif()

set(LXQT_LIBRARY_NAME "lxqt" CACHE STRING "lxqt")
set(LXQT_RELATIVE_SHARE_DIR "${LXQT_LIBRARY_NAME}")
set(LXQT_SHARE_DIR "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${LXQT_RELATIVE_SHARE_DIR}")
set(LXQT_RELATIVE_TRANSLATIONS_DIR "${LXQT_LIBRARY_NAME}/translations")
set(LXQT_TRANSLATIONS_DIR "${LXQT_SHARE_DIR}/translations")
set(LXQT_GRAPHICS_DIR "${LXQT_SHARE_DIR}/graphics")

file(GLOB toInstallFindModules
    "${PROJECT_SOURCE_DIR}/cmake/find-modules/*.cmake"
)

file(GLOB toInstallModules
    "${PROJECT_SOURCE_DIR}/cmake/modules/*.cmake"
    "${PROJECT_SOURCE_DIR}/cmake/modules/*.in"
)


#-----------------------------------------------------------------------------
# Superbuild mode
#-----------------------------------------------------------------------------
set(FIND_MODULES_INSTALL_DIR "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/lxqt-build-tools/cmake/find-modules/")
set(MODULES_INSTALL_DIR "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/lxqt-build-tools/cmake/modules/")

file(COPY
    ${toInstallFindModules}
    DESTINATION "${FIND_MODULES_INSTALL_DIR}"
)

file(COPY
    ${toInstallModules}
    DESTINATION "${MODULES_INSTALL_DIR}"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/lxqt-build-tools-config.cmake.in"
    "${CMAKE_BINARY_DIR}/lxqt-build-tools-config.cmake"
    INSTALL_DESTINATION "neverland"     # required, altough we don't install it
    PATH_VARS
        MODULES_INSTALL_DIR
        FIND_MODULES_INSTALL_DIR
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LXQtConfigVars.cmake.in"
    "${MODULES_INSTALL_DIR}/LXQtConfigVars.cmake"
    @ONLY
)


#-----------------------------------------------------------------------------
# Installable mode
#-----------------------------------------------------------------------------
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/lxqt-build-tools/")
set(FIND_MODULES_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/lxqt-build-tools/find-modules/")
set(MODULES_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/lxqt-build-tools/modules/")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/lxqt-build-tools-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install/lxqt-build-tools-config.cmake"
    INSTALL_DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    PATH_VARS
        MODULES_INSTALL_DIR
        FIND_MODULES_INSTALL_DIR
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/LXQtConfigVars.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/install/LXQtConfigVars.cmake"
    @ONLY
)

# The package version file is common to superbuild and installable mode
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/lxqt-build-tools-config-version.cmake.in"
    "${CMAKE_BINARY_DIR}/lxqt-build-tools-config-version.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/install/lxqt-build-tools-config.cmake"
    "${CMAKE_BINARY_DIR}/lxqt-build-tools-config-version.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

install(
    FILES ${toInstallFindModules}
    DESTINATION "${FIND_MODULES_INSTALL_DIR}"
)

install(
    FILES
        ${toInstallModules}
        "${CMAKE_CURRENT_BINARY_DIR}/install/LXQtConfigVars.cmake"
    DESTINATION
        "${MODULES_INSTALL_DIR}"
)

# Tools install
install(
    PROGRAMS
        "lxqt-transupdate"
    DESTINATION
        "${CMAKE_INSTALL_BINDIR}")
