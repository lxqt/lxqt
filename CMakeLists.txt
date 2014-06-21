cmake_minimum_required(VERSION 2.8.9)

project(liblxqt)

set(LXQT_MAJOR_VERSION 0)
set(LXQT_MINOR_VERSION 7)
set(LXQT_PATCH_VERSION 0)

# Set default installation paths
set(LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")
set(LXQT_PKG_CONFIG_DESCRIPTION "Shared library for LXQt applications")

set(PUB_HDRS
    aboutdialog/lxqtaboutdialog.h
    lxqtsettings.h
    lxqtxfitman.h
    lxqtplugininfo.h
    lxqtpowermanager.h
    addplugindialog/lxqtaddplugindialog.h
    lxqtscreensaver.h
    lxqtapplication.h
    lxqttranslator.h
    lxqtprogramfinder.h

    configdialog/lxqtconfigdialog.h
    configdialog/lxqtpageselectwidget.h

    lxqtpower/lxqtpower.h
    lxqtnotification.h
    lxqtautostartentry.h
    lxqtgridlayout.h
    lxqtrotatedwidget.h
    lxqtglobals.h
)

set(PUBLIC_CLASSES
    AboutDialog
    Settings
    XfitMan
    PluginInfo
    PowerManager
    AddPluginDialog
    ScreenSaver
    Application
    Translator
    ProgramFinder

    ConfigDialog
    PageSelectWidget

    Power
    Notification
    AutostartEntry
    GridLayout
    RotatedWidget
)

set(PRIV_HDRS
    aboutdialog/lxqtaboutdialog_p.h
    aboutdialog/technicalinfo.h
    translatorsinfo/translatorsinfo.h
)

set(SRCS
    aboutdialog/lxqtaboutdialog.cpp
    aboutdialog/technicalinfo.cpp
    lxqtxfitman.cpp
    lxqtplugininfo.cpp
    lxqtpowermanager.cpp
    addplugindialog/lxqtaddplugindialog.cpp
    lxqtsettings.cpp
    lxqtscreensaver.cpp
    lxqtapplication.cpp
    lxqttranslator.cpp
    lxqtprogramfinder.cpp

    configdialog/lxqtconfigdialog.cpp
    configdialog/lxqtpageselectwidget.cpp

    lxqtpower/lxqtpower.cpp
    lxqtpower/lxqtpowerproviders.cpp
    lxqtnotification.cpp
    lxqtautostartentry.cpp
    translatorsinfo/translatorsinfo.cpp
    lxqtgridlayout.cpp
    lxqtrotatedwidget.cpp
)

set(MOCS
    aboutdialog/lxqtaboutdialog.h
    aboutdialog/lxqtaboutdialog_p.h
    lxqtpowermanager.h
    addplugindialog/lxqtaddplugindialog.h
    lxqtsettings.h
    lxqtscreensaver.h
    lxqtapplication.h

    configdialog/lxqtconfigdialog.h
    configdialog/lxqtpageselectwidget.h

    lxqtnotification.h
    lxqtnotification_p.h
    lxqtpower/lxqtpower.h
    lxqtpower/lxqtpowerproviders.h
    lxqtgridlayout.h
    lxqtrotatedwidget.h
)

set(FORMS
    aboutdialog/lxqtaboutdialog.ui
    configdialog/lxqtconfigdialog.ui
    addplugindialog/lxqtaddplugindialog.ui
)

file(GLOB TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/liblxqt_*.ts
)

# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


option(USE_QT5 "Build with Qt5." $ENV{USE_QT5})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(USE_QT5)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt5Widgets REQUIRED QUIET)
    find_package(Qt5DBus REQUIRED QUIET)
    find_package(Qt5X11Extras REQUIRED QUIET)
    find_package(Qt5LinguistTools REQUIRED QUIET)
    find_package(Qt5Xdg REQUIRED)
    message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")

    QT5_WRAP_UI(UIS ${FORMS})
    QT5_ADD_TRANSLATION(QM_FILES ${TS_FILES})
    QT5_ADD_DBUS_INTERFACE(SRCS
        dbus/org.freedesktop.Notifications.xml
        notifications_interface
    )

    set(LXQT_QT_VERSION "5")
    set(LXQT_QT_VERSION_SUFFIX "-qt5")
    set(LXQT_PKG_CONFIG_REQUIRES "Qt5Widgets Qt5Xml Qt5DBus Qt5X11Extras")
else()
    find_package(Qt4 REQUIRED QtCore QtGui QUIET)
    find_package(QTXDG REQUIRED)
    message(STATUS "Building with Qt ${QTVERSION}")

    include(${QT_USE_FILE})

    QT4_WRAP_CPP(MOCS ${MOCS})
    QT4_WRAP_UI(UIS ${FORMS})
    QT4_ADD_TRANSLATION(QM_FILES ${TS_FILES})
    QT4_ADD_DBUS_INTERFACE(SRCS
        dbus/org.freedesktop.Notifications.xml
        notifications_interface
    )

    set(LXQT_QT_VERSION "4")
    set(LXQT_QT_VERSION_SUFFIX "")
    set(LXQT_PKG_CONFIG_REQUIRES "QtCore, QtXml")
endif()

include(${QTXDG_USE_FILE})

find_package(X11 REQUIRED QUIET)

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${X11_INCLUDE_DIR}
    ${QTXDG_INCLUDE_DIRS}
)

if (NOT USE_QT5)
    include_directories (
        ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
    )
endif()

# Standard directories for installation
include(GNUInstallDirs)

set(LXQT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION})
set(LXQT_LIBRARY_NAME "lxqt${LXQT_QT_VERSION_SUFFIX}")
set(LXQT_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/${LXQT_LIBRARY_NAME})
set(LXQT_TRANSLATIONS_DIR ${LXQT_SHARE_DIR}/translations)

set(LXQT_INTREE_INCLUDE_DIR     ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include)
set(LXQT_INTREE_TARGETS_FILE    ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-targets.cmake)

set(LXQT_INSTALL_INCLUDE_DIR   ${CMAKE_INSTALL_FULL_INCLUDEDIR}/${LXQT_LIBRARY_NAME})
set(LXQT_INSTALL_CMAKE_DIR     ${CMAKE_INSTALL_PREFIX}/share/cmake)

include(cmake/FindInstallConfigPath.cmake)
include(create_portable_headers)

message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


# Copy public headers
foreach(h ${PUB_HDRS})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} ${LXQT_INTREE_INCLUDE_DIR}/LXQt/${bh} COPYONLY)
    if (NOT USE_QT5)
        configure_file(${h} ${LXQT_INTREE_INCLUDE_DIR}/lxqt/${bh} COPYONLY)
    endif()
endforeach()

# Create the portable headers
create_portable_headers(INTREE_PORTABLE_HEADERS ${LXQT_INTREE_INCLUDE_DIR}/LXQt ${PUBLIC_CLASSES})
check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${INTREE_PORTABLE_HEADERS})

#************************************************
# Create in-tree build infrastructure
#************************************************
set(CFG_LXQT_INCLUDE_DIR    ${LXQT_INTREE_INCLUDE_DIR})
set(CFG_LXQT_LIBRARY        ${LXQT_LIBRARY_NAME})
set(CFG_LXQT_USE_FILE       ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}_use.cmake)
set(CFG_LXQT_TARGETS_FILE   ${LXQT_INTREE_TARGETS_FILE})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqtX-config.cmake.in 
    ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-config.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_use.cmake.in
    ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}_use.cmake
    @ONLY
)

include(${CFG_LXQT_USE_FILE})


#************************************************
# Create instalable build infrastructure
#************************************************
set(CFG_LXQT_INCLUDE_DIR    ${LXQT_INSTALL_INCLUDE_DIR})
set(CFG_LXQT_LIBRARY        ${LXQT_LIBRARY_NAME})
set(CFG_LXQT_USE_FILE       ${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}/${LXQT_LIBRARY_NAME}_use.cmake)
set(CFG_LXQT_TARGETS_FILE   ${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}/${LXQT_LIBRARY_NAME}-targets.cmake)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqtX-config.cmake.in 
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}-config.cmake
    @ONLY
)

# use gcc visibility feature to decrease unnecessary exported symbols
if (CMAKE_COMPILER_IS_GNUCXX)
    # set visibility to hidden to hide symbols, unlesss they're exporeted
    # manually in the code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions -Wl,-no-undefined -Wall")
endif()
add_definitions(-DCOMPILE_LIBLXQT)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_use.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}_use.cmake
    @ONLY
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}_use.cmake
    DESTINATION ${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}
    COMPONENT Devel
)

install(EXPORT 
    ${LXQT_LIBRARY_NAME}-targets
    DESTINATION ${LXQT_INSTALL_CMAKE_DIR}/${LXQT_LIBRARY_NAME}
    COMPONENT Devel
)


include(translatorsinfo/CMakeLists.txt)
get_translatorsinfo_qrc(translatorsinfo_qrc)
if(USE_QT5)
    qt5_add_resources(QRC_CXX_SOURCES ${translatorsinfo_qrc})
else()
    qt4_add_resources(QRC_CXX_SOURCES ${translatorsinfo_qrc})
endif()

add_library(${LXQT_LIBRARY_NAME}
    SHARED ${PUB_HDRS}
           ${PRIV_HDRS}
           ${SRCS}
           ${dbus_generated}
           ${UIS} ${MOCS}
           ${QM_FILES}
           ${QRC_CXX_SOURCES}
           
)

if (USE_QT5)
	qt5_use_modules(${LXQT_LIBRARY_NAME} Widgets DBus X11Extras)
    target_link_libraries(${LXQT_LIBRARY_NAME}
        ${X11_X11_LIB}
        ${QTXDG_LIBRARIES}
    )
else()
    target_link_libraries(${LXQT_LIBRARY_NAME}
        ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${X11_X11_LIB}
        ${QTXDG_LIBRARIES}
    )
endif()

set_target_properties(${LXQT_LIBRARY_NAME} PROPERTIES
    VERSION   ${LXQT_VERSION}
    SOVERSION ${LXQT_MAJOR_VERSION}
)

install(TARGETS 
    ${LXQT_LIBRARY_NAME} 
    DESTINATION ${CMAKE_INSTALL_LIBDIR} 
    EXPORT ${LXQT_LIBRARY_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)


export(TARGETS ${LXQT_LIBRARY_NAME} ${QTXDG_TARGET} FILE ${LXQT_INTREE_TARGETS_FILE})

install(FILES ${PUB_HDRS}                DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/LXQt)

if (NOT USE_QT5)
    install(FILES ${PUB_HDRS}   DESTINATION ${LXQT_INSTALL_INCLUDE_DIR})
endif()

install(FILES ${INTREE_PORTABLE_HEADERS} DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/LXQt)

install(FILES ${QM_FILES} DESTINATION ${LXQT_TRANSLATIONS_DIR})

#************************************************
# Create pkgconfig file
#************************************************
lxqt_create_pkgconfig_file(${LXQT_LIBRARY_NAME}
    ${LXQT_PKG_CONFIG_DESCRIPTION}
    ${LXQT_PKG_CONFIG_REQUIRES}
    ${LXQT_LIBRARY_NAME}
    ${LXQT_VERSION}
)
#************************************************


# building tarball with CPack -------------------------------------------------
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VERSION_MAJOR ${LXQT_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${LXQT_MINOR_VERSION})
set (CPACK_PACKAGE_VERSION_PATCH ${LXQT_PATCH_VERSION})
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include (CPack)
