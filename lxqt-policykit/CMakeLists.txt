cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-policykit-agent)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

find_package(PkgConfig)
pkg_check_modules(POLKIT_AGENT REQUIRED polkit-agent-1)
message(STATUS "polkit agent: ${POLKIT_AGENT_INCLUDE_DIRS} ${POLKIT_AGENT_LIBRARIES}")

set(POLKIT_AGENT_BINARY_DIR "${CMAKE_INSTALL_BINDIR}" CACHE FILEPATH "Directory for install polkit agent")

# Minumum Versions
set(LXQT_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10.0")

find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(PolkitQt5-1 REQUIRED)
message(STATUS "Building with Qt${Qt5Core_VERSION}")

# Patch Version
set(LXQT_POLKITAGENT_PATCH_VERSION 0)

set(LXQT_POLKITAGENT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_POLKITAGENT_PATCH_VERSION})
add_definitions(
    "-DLXQT_POLKITAGENT_VERSION=\"${LXQT_POLKITAGENT_VERSION}\""
)

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# Deal with a bug in polkit-qt5
if (NOT IS_ABSOLUTE "${POLKITQT-1_INCLUDE_DIR}")
    set(POLKITQT-1_INCLUDE_DIR "${POLKITQT-1_INSTALL_DIR}/${POLKITQT-1_INCLUDE_DIR}")
    set(POLKITQT-1_AGENT_LIBRARY "${POLKITQT-1_INSTALL_DIR}/${POLKITQT-1_AGENT_LIBRARY}")
    set(POLKITQT-1_CORE_LIBRARY "${POLKITQT-1_INSTALL_DIR}/${POLKITQT-1_CORE_LIBRARY}")
endif()
message(STATUS "PolkitQt-1: ${POLKITQT-1_VERSION_STRING}")
message(STATUS "   install: ${POLKITQT-1_INSTALL_DIR}")
message(STATUS "      libs: ${POLKITQT-1_CORE_LIBRARY} ${POLKITQT-1_AGENT_LIBRARY}")
message(STATUS "   include: ${POLKITQT-1_INCLUDE_DIR}")

# TODO/FIXME: pkg-config for hardcoded paths
# pkg-config --cflags glib-2.0
# pkg-config --cflags polkit-agent-1
include_directories(
    ${POLKIT_AGENT_INCLUDE_DIRS}
    "${POLKITQT-1_INCLUDE_DIR}"
)

set(lxqt-policykit-agent_SRCS
    src/main.cpp
    src/policykitagent.cpp
    src/policykitagentgui.cpp
)

set(lxqt-policykit-agent_H
    src/policykitagent.h
    src/policykitagentgui.h
)

set(lxqt-policykit-agent_UI
    src/policykitagentgui.ui
)

# Translations **********************************
include(LXQtTranslate)

lxqt_translate_ts(lxqt-policykit-agent_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-policykit-agent_H}
        ${lxqt-policykit-agent_SRCS}
        ${lxqt-policykit-agent_UI}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})
#************************************************

add_executable(lxqt-policykit-agent
    ${lxqt-policykit-agent_SRCS}
    ${lxqt-policykit-agent_MOCS}
    ${lxqt-policykit-agent_QM_FILES}
    ${QM_LOADER}
)

target_link_libraries(lxqt-policykit-agent
    Qt5::Widgets
    lxqt
    ${POLKITQT-1_CORE_LIBRARY}
    ${POLKITQT-1_AGENT_LIBRARY}
    ${POLKIT_AGENT_LDFLAGS}
)

# install
install(TARGETS
    lxqt-policykit-agent
    DESTINATION "${POLKIT_AGENT_BINARY_DIR}"
    COMPONENT Runtime
)
install(FILES
    man/lxqt-policykit-agent.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)

add_subdirectory(autostart)
