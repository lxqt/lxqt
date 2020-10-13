cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
Project(qps)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

set(MAJOR_VERSION 2)
set(MINOR_VERSION 1)
set(PATCH_VERSION 0)
set(QPS_VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
add_definitions(
    -DQPS_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"
)

# Minimum Versions
set(LXQTBT_MINIMUM_VERSION "0.7.0")
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Widgets X11Extras DBus LinguistTools)
find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)

include(GNUInstallDirs)
include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(Qt5TranslationLoader)

# LXQtCompilerSettings adds them but qps still uses implicit string casts
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_BYTEARRAY
)

# Must be defined after including GNUInstallDirs. Move with care.
set(QPS_TRANSLATIONS_DIR
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME}/translations"
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

# install the man page
install(FILES qps.1 DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/man/man1")

# install the app icon
install(FILES
    icon/qps.png
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps"
)
