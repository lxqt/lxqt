cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(libfm-qt)

set(LIBFM_QT_LIBRARY_NAME "fm-qt" CACHE STRING "fm-qt")

set(LIBFM_QT_API_VERSION_MAJOR 0)
set(LIBFM_QT_API_VERSION_MINOR 15)
set(LIBFM_QT_API_VERSION_PATCH 1)
set(LIBFM_QT_API_VERSION ${LIBFM_QT_API_VERSION_MAJOR}.${LIBFM_QT_API_VERSION_MINOR}.${LIBFM_QT_API_VERSION_PATCH})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# We use the libtool versioning scheme for the internal so name, "current:revision:age"
# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html#Updating-version-info
# https://www.sourceware.org/autobook/autobook/autobook_91.html
# http://pusling.com/blog/?p=352
# Actually, libtool uses different ways on different operating systems. So there is no
# universal way to translate a libtool version-info to a cmake version.
# We use "(current-age).age.revision" as the cmake version.
# current: 6, revision: 0, age: 0 => version: 6.0.0
set(LIBFM_QT_ABI_VERSION "7.0.0")
set(LIBFM_QT_SOVERSION "7")

set(GLIB_MINIMUM_VERSION "2.50.0")
set(LIBMENUCACHE_MINIMUM_VERSION "1.1.0")
set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(Qt5Widgets "${QT_MINIMUM_VERSION}" REQUIRED)
find_package(Qt5LinguistTools "${QT_MINIMUM_VERSION}" REQUIRED)
find_package(Qt5X11Extras "${QT_MINIMUM_VERSION}" REQUIRED)

find_package(lxqt-build-tools "${LXQTBT_MINIMUM_VERSION}" REQUIRED)
find_package(GLIB "${GLIB_MINIMUM_VERSION}" REQUIRED COMPONENTS gio gio-unix gobject gthread)
find_package(MenuCache "${LIBMENUCACHE_MINIMUM_VERSION}" REQUIRED)
find_package(Exif REQUIRED)
find_package(XCB REQUIRED)

message(STATUS "Building ${PROJECT_NAME} with Qt ${Qt5Core_VERSION}")

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
include(GNUInstallDirs)
include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(LXQtPreventInSourceBuilds)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}-config-version.cmake"
    VERSION ${LIBFM_QT_API_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_BINARY_DIR}/${LIBFM_QT_LIBRARY_NAME}-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/${LIBFM_QT_LIBRARY_NAME}"
    COMPONENT Devel
)

add_subdirectory(src)
add_subdirectory(data)

# add Doxygen support to generate API docs
# References:
# https://majewsky.wordpress.com/2010/08/14/tip-of-the-day-cmake-and-doxygen/
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)
if(BUILD_DOCUMENTATION)
    find_package(Doxygen REQUIRED)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(doc ALL
        ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
    install(DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/docs"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        COMPONENT Devel
    )
endif()
