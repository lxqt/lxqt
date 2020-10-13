cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-openssh-askpass)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

# Minimum Versions
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
message(STATUS "Building with Qt${Qt5Core_VERSION}")

# Patch Version
set(LXQT_ASKPASS_PATCH_VERSION 0)

set(LXQT_ASKPASS_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_ASKPASS_PATCH_VERSION})
add_definitions("-DLXQT_ASKPASS_VERSION=\"${LXQT_ASKPASS_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

set(lxqt-openssh-askpass_HDRS
    src/mainwindow.h
)

set(lxqt-openssh-askpass_SRCS
    src/main.cpp
    src/mainwindow.cpp
)

set(lxqt-openssh-askpass_UIS
    src/mainwindow.ui
)

# Translations **********************************
include(LXQtTranslate)

lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-openssh-askpass_HDRS}
        ${lxqt-openssh-askpass_SRCS}
        ${lxqt-openssh-askpass_UIS}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(lxqt-openssh_QM_LOADER ${PROJECT_NAME})
#************************************************

add_executable(lxqt-openssh-askpass
    ${lxqt-openssh-askpass_SRCS}
    ${TRS}
    ${QM_FILES}
    ${lxqt-openssh_QM_LOADER}
)

target_link_libraries(lxqt-openssh-askpass
    Qt5::Widgets
    lxqt
)

INSTALL(TARGETS
    lxqt-openssh-askpass
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)

install(FILES
    man/lxqt-openssh-askpass.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
