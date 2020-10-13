cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(libqtxdg)

option(BUILD_TESTS "Builds tests" OFF)
option(BUILD_DEV_UTILS "Builds and install development utils" OFF)

# additional cmake files
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(QTXDG_MAJOR_VERSION 3)
set(QTXDG_MINOR_VERSION 5)
set(QTXDG_PATCH_VERSION 0)
set(QTXDG_VERSION_STRING ${QTXDG_MAJOR_VERSION}.${QTXDG_MINOR_VERSION}.${QTXDG_PATCH_VERSION})

set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(QT_MINIMUM_VERSION "5.10.0")
set(GLIB_MINIMUM_VERSION "2.41.0") # Mime Apps new implementation

find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Widgets Svg Xml DBus)
find_package(GLIB ${GLIB_MINIMUM_VERSION} REQUIRED COMPONENTS gobject gio gio-unix)

include(GNUInstallDirs)             # Standard directories for installation
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)

include(LXQtPreventInSourceBuilds)
include(LXQtCreatePkgConfigFile)
include(LXQtCreatePortableHeaders)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# Add link optimizations. It make the .so around 33% smaller without any known
# drawback.
include(compiler_settings NO_POLICY_SCOPE)

set(CMAKE_AUTOMOC ON)

if (BUILD_TESTS)
    find_package(Qt5 ${QT_MINIMUM_VERSION} CONFIG REQUIRED Test)
endif()


set(QTXDGX_LIBRARY_NAME "Qt5Xdg")
set(QTXDGX_FILE_NAME "qt5xdg")

set(QTXDGX_ICONLOADER_LIBRARY_NAME "Qt5XdgIconLoader")
set(QTXDGX_ICONLOADER_FILE_NAME "qt5xdgiconloader")
set(QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME "Qt5XdgIconPlugin")

set(QTXDGX_PKG_CONFIG_DESCRIPTION "Qt5Xdg, a Qt5 implementation of XDG standards")
set(QTXDGX_PKG_CONFIG_REQUIRES "Qt5Core >= ${QT_MINIMUM_VERSION}, Qt5Xml >= ${QT_MINIMUM_VERSION}, Qt5Widgets >= ${QT_MINIMUM_VERSION}, Qt5DBus >= ${QT_MINIMUM_VERSION}, Qt5XdgIconLoader = ${QTXDG_VERSION_STRING}")

set(QTXDGX_ICONLOADER_PKG_CONFIG_DESCRIPTION "Qt5XdgIconLader, a Qt5 XDG Icon Loader")
set(QTXDGX_ICONLOADER_PKG_CONFIG_REQUIRES "Qt5Gui >= ${QT_MINIMUM_VERSION}, Qt5Svg >= ${QT_MINIMUM_VERSION}")

set(QTXDGX_INTREE_INCLUDEDIR "${CMAKE_CURRENT_BINARY_DIR}/InTreeBuild/include")

message(STATUS "Building ${PROJECT_NAME} with Qt ${Qt5Core_VERSION}")

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    target_compile_definitions(${QTXDGX_LIBRARY_NAME}
        PRIVATE "QTXDG_TESTS=\"1\""
    )
    add_subdirectory(test)
else()
    message(STATUS "")
    message(STATUS "For building tests use -DBUILD_TESTS=Yes option.")
    message(STATUS "")
endif()

if (BUILD_DEV_UTILS)
    add_subdirectory(util)
endif()

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${QTXDGX_FILE_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    VERSION ${QTXDG_VERSION_STRING}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake.in"
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config-version.cmake"
    VERSION ${QTXDG_VERSION_STRING}
    COMPATIBILITY AnyNewerVersion
)

lxqt_create_pkgconfig_file(
    PACKAGE_NAME ${QTXDGX_LIBRARY_NAME}
    DESCRIPTIVE_NAME ${QTXDGX_LIBRARY_NAME}
    DESCRIPTION ${QTXDGX_PKG_CONFIG_DESCRIPTION}
    INCLUDEDIRS ${QTXDGX_FILE_NAME}
    LIBS ${QTXDGX_LIBRARY_NAME}
    REQUIRES ${QTXDGX_PKG_CONFIG_REQUIRES}
    REQUIRES_PRIVATE ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    VERSION ${QTXDG_VERSION_STRING}
    INSTALL
)

lxqt_create_pkgconfig_file(
    PACKAGE_NAME ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    DESCRIPTIVE_NAME ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    DESCRIPTION ${QTXDGX_ICONLOADER_PKG_CONFIG_DESCRIPTION}
    INCLUDEDIRS ${QTXDGX_ICONLOADER_FILE_NAME}
    LIBS ${QTXDGX_ICONLOADER_LIBRARY_NAME}
    REQUIRES ${QTXDGX_ICONLOADER_PKG_CONFIG_REQUIRES}
    VERSION ${QTXDG_VERSION_STRING}
    INSTALL
)

install(FILES
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${QTXDGX_FILE_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config.cmake"
    "${CMAKE_BINARY_DIR}/${QTXDGX_ICONLOADER_FILE_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
    COMPONENT Devel
)

install(EXPORT
    "${QTXDGX_FILE_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_FILE_NAME}"
    FILE "${QTXDGX_FILE_NAME}-targets.cmake"
    COMPONENT Devel
)

install(EXPORT
    "${QTXDGX_ICONLOADER_FILE_NAME}-targets"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${QTXDGX_ICONLOADER_FILE_NAME}"
    FILE "${QTXDGX_ICONLOADER_FILE_NAME}-targets.cmake"
    COMPONENT Devel
)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

#add_custom_target(uninstall
#    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
