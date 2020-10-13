cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-globalkeys)

# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

set(KF5_MINIMUM_VERSION "5.36.0")
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5DBus ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)

# Version
set(LXQT_GLOBALKEYS_MAJOR_VERSION 0)
set(LXQT_GLOBALKEYS_MINOR_VERSION 15)
set(LXQT_GLOBALKEYS_PATCH_VERSION 0)
set(LXQT_GLOBALKEYS_VERSION ${LXQT_GLOBALKEYS_MAJOR_VERSION}.${LXQT_GLOBALKEYS_MINOR_VERSION}.${LXQT_GLOBALKEYS_PATCH_VERSION})
add_definitions("-DLXQT_GLOBALKEYS_VERSION=\"${LXQT_GLOBALKEYS_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtTranslate)
include(LXQtCreatePkgConfigFile)
include(LXQtCreatePortableHeaders)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

include(GNUInstallDirs) # Standard directories for installation
include(CMakePackageConfigHelpers)

set(LXQT_GLOBALKEYS_LIBRARY_NAME lxqt-globalkeys)
set(LXQT_GLOBALKEYS_UI_LIBRARY_NAME lxqt-globalkeys-ui)
set(LXQT_INSTALL_CMAKE_DIR     "${CMAKE_INSTALL_DATAROOTDIR}/cmake")

#************************************************
# Build config.cmake files
#************************************************
set(LXQT_GLOBALKEYS_CMAKE_NAME ${LXQT_GLOBALKEYS_LIBRARY_NAME})
set(LXQT_GLOBALKEYS_UI_CMAKE_NAME ${LXQT_GLOBALKEYS_UI_LIBRARY_NAME})

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_globalkeys-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}-config.cmake"
    INSTALL_DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_globalkeys_ui-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_UI_CMAKE_NAME}-config.cmake"
    INSTALL_DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}"
)
#************************************************
# End of build config.cmake
#************************************************

#************************************************
# Generate config-version.cmake files
#************************************************
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}-config-version.cmake"
    VERSION ${LXQT_GLOBALKEYS_VERSION}
    COMPATIBILITY AnyNewerVersion
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_UI_CMAKE_NAME}-config-version.cmake"
    VERSION ${LXQT_GLOBALKEYS_VERSION}
    COMPATIBILITY AnyNewerVersion
)
#************************************************
# End of generate config-version.cmake files
#************************************************

set(LXQT_GLOBALKEYS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(daemon)
add_subdirectory(config)
add_subdirectory(client)
add_subdirectory(ui)

install(FILES
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}-config-version.cmake"
    DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_GLOBALKEYS_CMAKE_NAME}"
)

install(FILES
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_UI_CMAKE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${LXQT_GLOBALKEYS_UI_CMAKE_NAME}-config-version.cmake"
    DESTINATION "${LXQT_INSTALL_CMAKE_DIR}/${LXQT_GLOBALKEYS_UI_CMAKE_NAME}"
)

# merged from lxqt-common
add_subdirectory(autostart)
add_subdirectory(xdg)

