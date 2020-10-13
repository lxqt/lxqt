cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(screengrab)

include(GNUInstallDirs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# options
option(SG_DBUS_NOTIFY "Enable D-Bus notifications" ON)
option(SG_EXT_EDIT "Enable ability to edit screenshots in external editor" ON)
option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

# Minimum Versions
set(KF5_MINIMUM_VERSION "5.36.0")
set(QT_MINIMUM_VERSION "5.10.0")
set(QTXDG_MINIMUM_VERSION "3.5.0")

find_package(Qt5LinguistTools  ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Network ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt5X11Extras ${QT_MINIMUM_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)

# right now we declare it as required
find_package(X11 REQUIRED)
set(HAVE_X11 1)

find_package(
    XCB REQUIRED MODULE COMPONENTS
        XCB
        SHAPE
        XFIXES
)

find_package(
    X11_XCB REQUIRED MODULE
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SCREENGRAB_VERSION "2.0.1")

if (DEV_VERSION)
    set(VERSION "${SCREENGRAB_VERSION}-dev (${DEV_VERSION})")
else()
    set(VERSION ${SCREENGRAB_VERSION})
endif()

add_definitions(
    -DVERSION="${VERSION}"
    -DQT_NO_FOREACH
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "Install prefix: " ${CMAKE_INSTALL_PREFIX})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Woverloaded-virtual -Wall -Wextra")

set(SG_LIBDIR "${CMAKE_INSTALL_LIBDIR}/screengrab")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${SG_LIBDIR}")
message(STATUS "Library path: ${CMAKE_INSTALL_RPATH}")

# Although the names, LXQtTranslateTs and LXQtTranslateDesktop, they don't
#   bring any dependency on lxqt.
include(LXQtTranslateTs)
include(LXQtTranslateDesktop)
include(Qt5TranslationLoader)

if(SG_EXT_EDIT)
    add_definitions( -DSG_EXT_EDIT="1")
    find_package(Qt5Xdg ${QTXDG_MINIMUM_VERSION} REQUIRED)
endif()

if(SG_DBUS_NOTIFY)
    find_package(Qt5DBus ${QT_MINIMUM_VERSION} REQUIRED)
    add_definitions( -DSG_DBUS_NOTIFY="1")
endif()

message(STATUS "Editing screenshots in external editor support: " ${SG_EXT_EDIT})
message(STATUS "Enable D-Bus notifications: " ${SG_DBUS_NOTIFY})
message(STATUS "Use system Qxt Library: " ${SG_USE_SYSTEM_QXT})
message(STATUS "Update source translation translations/*.ts files: " ${UPDATE_TRANSLATIONS})

# docs
# CMAKE_INSTALL_FULL_DOCDIR = CMAKE_INSTALL_PREFIX/CMAKE_INSTALL_DATADIR/doc/PROJECT_NAME
message(STATUS "Documentation directory: " ${CMAKE_INSTALL_FULL_DOCDIR})
add_definitions(-DSG_DOCDIR="${CMAKE_INSTALL_FULL_DOCDIR}")

# app icon path
add_definitions(-DSG_ICONPATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/pixmaps/screengrab.png")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/common/qkeysequencewidget")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/common/qkeysequencewidget/src")

if (SG_EXT_EDIT)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/modules/extedit")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/modules/extedit")
endif()

set(SCREENGRAB_SRC
    src/core/main.cpp
    src/core/singleapp.cpp
    src/core/core.cpp
    src/core/config.cpp
    src/core/regionselect.cpp
    src/core/shortcutmanager.cpp
    src/core/modulemanager.cpp
    src/core/ui/configwidget.cpp
    src/core/ui/about.cpp
    src/core/ui/mainwindow.cpp
)

if(SG_DBUS_NOTIFY)
    set(SCREENGRAB_SRC ${SCREENGRAB_SRC} src/core/dbusnotifier.cpp)
endif()

if(X11_FOUND)
    set(SCREENGRAB_SRC ${SCREENGRAB_SRC} src/core/x11utils.cpp)
endif()

set(SCREENGRAB_HDR
    src/core/singleapp.h
)

set(SCREENGRAB_UI
    src/core/ui/configwidget.ui
    src/core/ui/aboutwidget.ui
    src/core/ui/mainwindow.ui
)

# Qt resource file
set(SCREENGRAB_QRC screengrab.qrc)
qt5_add_resources(QRC_SOURCES ${SCREENGRAB_QRC})

message(STATUS "Generating localize ...")

set(SCREENGRAB_CONFIG_FILES
    screengrab.conf
)

set(SCREENGRAB_DESKTOP_FILES_IN
    screengrab.desktop.in
)

lxqt_translate_ts(SCREENGRAB_QMS
    USE_QT5 TRUE
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${SCREENGRAB_SRC}
        ${SCREENGRAB_UI}
    INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

lxqt_translate_desktop(SCREENGRAB_DESKTOP_FILES
    SOURCES ${SCREENGRAB_DESKTOP_FILES_IN}
)

qt5_translation_loader(SCREENGRAB_QM_LOADER
    "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/translations"
    ${PROJECT_NAME}
)

# generating executable
add_executable(screengrab
    ${SCREENGRAB_SRC}
    ${SCREENGRAB_UI_H}
    ${QRC_SOURCES}
    ${SCREENGRAB_QMS}
    ${SCREENGRAB_DESKTOP_FILES}
    ${SCREENGRAB_QM_LOADER}
)

if(SG_EXT_EDIT)
    target_link_libraries(screengrab extedit)
endif()

if(SG_DBUS_NOTIFY)
    target_link_libraries(screengrab Qt5::DBus)
endif()

if (X11_XCB_FOUND)
    add_definitions( -DX11_XCB_FOUND="1")
    target_link_libraries(screengrab ${X11_XCB_LIBRARIES})
endif()

if (XCB_XCB_FOUND)
    add_definitions( -DXCB_XCB_FOUND="1")
    target_link_libraries(screengrab ${XCB_XCB_LIBRARY})
endif()

if (XCB_SHAPE_FOUND)
    add_definitions( -DXCB_SHAPE_FOUND="1")
    target_link_libraries(screengrab ${XCB_SHAPE_LIBRARY})
endif()

if (XCB_XFIXES_FOUND)
    add_definitions( -DXCB_XFOXES_FOUND="1")
    target_link_libraries(screengrab ${XCB_XFIXES_LIBRARY})
endif()

# Link with Network and X11Extras. See pull#86. TODO: Should be optional when upload module is needed.
target_link_libraries(screengrab qkeysequencewidget Qt5::Widgets KF5::WindowSystem Qt5::X11Extras Qt5::Network ${X11_LIBRARIES})

# installing
install(TARGETS screengrab RUNTIME DESTINATION bin)
# install html docs
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/html" DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}")
# install config files
install(FILES ${SCREENGRAB_CONFIG_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
# install desktop files
install(FILES ${SCREENGRAB_DESKTOP_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
# install pixmap
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/img/screengrab.png" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps")
