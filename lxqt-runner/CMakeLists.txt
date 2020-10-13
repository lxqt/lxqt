cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-runner)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CMakeDependentOption)
include(GNUInstallDirs)

option(RUNNER_MATH "Math operations support" ON)
option(RUNNER_VBOX "Virtual Box support" ON)
option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
option(USE_MENU_CACHE "Use libmenu-cache from LXDE to generate the app menu" OFF)
cmake_dependent_option(RUNNER_VBOX_HEADLESS
    "Use headless mode instead of default window" OFF
    "RUNNER_VBOX" OFF
)

# Minimum Versions
set(KF5_MINIMUM_VERSION "5.36.0")
set(LIBMENUCACHE_MINIMUM_VERSION "1.1.0")
set(LXQT_MINIMUM_VERSION "0.15.0")
set(LXQT_GLOBALKEYS_MINIMUM_VERSION "0.15.0")
set(QT_MINIMUM_VERSION "5.10")

find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Xml ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys-ui ${LXQT_GLOBALKEYS_MINIMUM_VERSION} REQUIRED)
message(STATUS "Building with Qt${Qt5Core_VERSION}")

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# Patch Version
set(LXQT_RUNNER_PATCH_VERSION 0)
set(LXQT_RUNNER_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_RUNNER_PATCH_VERSION})
add_definitions(
    "-DLXQT_RUNNER_VERSION=\"${LXQT_RUNNER_VERSION}\""
)

set(QTX_LIBRARIES Qt5::Widgets Qt5::Xml)

if (USE_MENU_CACHE OR RUNNER_MATH)
    find_package(PkgConfig REQUIRED)
endif ()

if (USE_MENU_CACHE)
    # optionally use libmenu-cache from lxde to generate the application menu
    find_package(MenuCache "$(LIBMENUCACHE_MINIMUM_VERSION}" REQUIRED)
endif()

if (RUNNER_MATH)
    pkg_check_modules(MUPARSER REQUIRED muparser)
endif ()

set(lxqt-runner_H_FILES
    dialog.h
    commanditemmodel.h
    widgets.h
    providers.h
    yamlparser.h
    configuredialog/configuredialog.h
)

set(lxqt-runner_CPP_FILES
    main.cpp
    dialog.cpp
    commanditemmodel.cpp
    widgets.cpp
    providers.cpp
    yamlparser.cpp
    configuredialog/configuredialog.cpp
)

set(lxqt-runner_UI_FILES
    dialog.ui
    configuredialog/configuredialog.ui
)

set(QRC_FILES

)

set(lxqt-runner_LIBRARIES
    lxqt
    lxqt-globalkeys
    lxqt-globalkeys-ui
    ${MENUCACHE_LIBRARIES}
    ${QTX_LIBRARIES}
    ${MUPARSER_LDFLAGS}
)

include(LXQtTranslate)

lxqt_translate_ts(lxqt-runner_QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lxqt-runner_H_FILES}
        ${lxqt-runner_CPP_FILES}
        ${lxqt-runner_UI_FILES}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(lxqt-runner_QM_LOADER ${PROJECT_NAME})

if (RUNNER_VBOX)
    list(APPEND QRC_FILES virtualbox-icons.qrc)
endif()

add_executable(${PROJECT_NAME}
    ${lxqt-runner_CPP_FILES}
    ${lxqt-runner_QRC_SOURCES}
    ${lxqt-runner_QM_FILES}
    ${lxqt-runner_QM_LOADER}
)

if (RUNNER_MATH)
    lxqt_enable_target_exceptions("${PROJECT_NAME}" PRIVATE)
endif ()

target_include_directories(${PROJECT_NAME}
    PRIVATE "$<$<BOOL:${MENUCACHE_FOUND}>:${MENUCACHE_INCLUDE_DIRS}>"
    PRIVATE "$<$<BOOL:${MUPARSER_FOUND}>:${MUPARSER_INCLUDE_DIRS}>"
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE "$<$<BOOL:${RUNNER_MATH}>:MATH_ENABLED>"
    PRIVATE "$<$<BOOL:${RUNNER_VBOX}>:VBOX_ENABLED>"
    PRIVATE "$<$<BOOL:${RUNNER_VBOX_HEADLESS}>:VBOX_HEADLESS_ENABLED>"
    PRIVATE "$<$<BOOL:${MENUCACHE_FOUND}>:HAVE_MENU_CACHE=1>"
)

target_link_libraries(${PROJECT_NAME}
    ${lxqt-runner_LIBRARIES}
    KF5::WindowSystem
)

install(TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)

install(FILES
    man/lxqt-runner.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)

add_subdirectory(autostart)
