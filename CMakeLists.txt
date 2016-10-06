cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(lxqt-build-tools)

set(LXQT_BUILD_TOOLS_MAJOR_VERSION 0)
set(LXQT_BUILD_TOOLS_MINOR_VERSION 1)
set(LXQT_BUILD_TOOLS_PATCH_VERSION 0)
set(LXQT_BUILD_TOOLS_VERSION ${LXQT_BUILD_TOOLS_MAJOR_VERSION}.${LXQT_BUILD_TOOLS_MINOR_VERSION}${LXQT_BUILD_TOOLS_PATCH_VERSION})

include(CMakePackageConfigHelpers)
include(GNUInstallDirs) # Standard directories for installation

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

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/lxqt-build-tools-config-version.cmake"
    VERSION ${LXQT_BUILD_TOOLS_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/install/lxqt-build-tools-config.cmake"
    "${CMAKE_BINARY_DIR}/lxqt-build-tools-config-version.cmake"
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
)

file(GLOB toInstallFindModules
    "${PROJECT_SOURCE_DIR}/cmake/find-modules/*.cmake"
)

install(
    FILES ${toInstallFindModules}
    DESTINATION "${FIND_MODULES_INSTALL_DIR}"
)

file(GLOB toInstallModules
    "${PROJECT_SOURCE_DIR}/cmake/modules/*.cmake"
    "${PROJECT_SOURCE_DIR}/cmake/modules/*.in"
)

install(
    FILES ${toInstallModules}
    DESTINATION "${MODULES_INSTALL_DIR}"
)
