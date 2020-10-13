cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(pcmanfm-qt)

# PcmanFm-Qt Version
set(PCMANFM_QT_VERSION_MAJOR 0)
set(PCMANFM_QT_VERSION_MINOR 15)
set(PCMANFM_QT_VERSION_PATCH 1)

set(PCMANFM_QT_VERSION ${PCMANFM_QT_VERSION_MAJOR}.${PCMANFM_QT_VERSION_MINOR}.${PCMANFM_QT_VERSION_PATCH})

# Minimum versions
set(LIBFMQT_MINIMUM_VERSION "0.15.1")
set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(QT_MINIMUM_VERSION "5.10.0")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Qt5DBus ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(fm-qt ${LIBFMQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)

message(STATUS "Building ${PROJECT_NAME} with Qt ${Qt5Core_VERSION}")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
include(GNUInstallDirs)
include(LXQtPreventInSourceBuilds)
include(LXQtConfigVars)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(pcmanfm)

# manpage for pcmanfm-qt
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/pcmanfm-qt.1.in"
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1"
    @ONLY
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)

# add Doxygen support to generate API docs
# References:
# http://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
# http://www.bluequartz.net/projects/EIM_Segmentation/SoftwareDocumentation/html/usewithcmakeproject.html
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs" DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif()

add_subdirectory(autostart)
add_subdirectory(config)
