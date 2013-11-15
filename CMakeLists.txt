cmake_minimum_required( VERSION 2.6 )

project(liblxqt)

set(LXQT_MAJOR_VERSION 0)
set(LXQT_MINOR_VERSION 5)
set(LXQT_PATCH_VERSION 0)
set(LXGQT_PKG_CONFIG_DESCRIPTION "Shared library for LXQt applications")

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


option(USE_QT5 "Force use the Qt5." $ENV{USE_QT5})
option(USE_QT4 "Force use the Qt4." $ENV{USE_QT4})

if((USE_QT4 AND USE_QT5) OR
   (NOT USE_QT4 AND NOT USE_QT5))

    # Autodetect Qt version
    find_package(Qt4 QUIET)
    if (QT4_FOUND)
        set(USE_QT4 ON)
        set(USE_QT5 OFF)
    else()
        set(USE_QT4 OFF)
        set(USE_QT5 ON)
    endif()
endif()


if(USE_QT5)
    cmake_minimum_required(VERSION 2.8.3)

    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)

    find_package(Qt5Widgets REQUIRED QUIET)
    message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")
    find_package(Qt5X11Extras REQUIRED QUIET)
    find_package(Qt5LinguistTools REQUIRED QUIET)

    set(LXQT_QT_VERSION "5")
    set(LXQT_PKG_CONFIG_REQUIRES "Qt5Widgets Qt5Xml")

    include_directories(${Qt5Widgets_INCLUDE_DIRS})
    add_definitions(${Qt5Widgets_DEFINITIONS})

    QT5_WRAP_UI(UIS ${FORMS})
    QT5_ADD_TRANSLATION(QM_FILES ${TS_FILES})
    #set(CMAKE_CXX_FLAGS
    #    "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
    #)

    set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES})

    #find_package(XDGQT5 REQUIRED QUIET)
else()
    find_package(Qt4 REQUIRED QUIET)
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
    set(LXQT_PKG_CONFIG_REQUIRES "QtCore, QtXml")
endif()

find_package(X11 REQUIRED QUIET)

find_package(xdgqt REQUIRED)
include(${XDGQT_USE_FILE})

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
    ${XDGQT_INCLUDE_DIRS}
)

# Standard directories for installation
include(GNUInstallDirs)

set(LXQT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION})
set(LXQT_LIBRARY_NAME "lxqt${LXQT_QT_VERSION}")
set(LXQT_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/lxqt)
set(LXQT_TRANSLATIONS_DIR ${LXQT_SHARE_DIR}/translations)

set(LXQT_INTREE_INCLUDE_DIR     ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/include)
set(LXQT_INTREE_TRAGETS_FILE    ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}-targets.cmake)

set(LXQT_INSTALL_INCLUDE_DIR   ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(LXQT_INSTALL_CMAKE_DIR     ${CMAKE_INSTALL_PREFIX}/share/cmake/)

include(cmake/FindInstallConfigPath.cmake)
include(create_portable_headers)



message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


# Copy public headers
foreach(h ${PUB_HDRS})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} ${LXQT_INTREE_INCLUDE_DIR}/LxQt/${bh} COPYONLY)
    configure_file(${h} ${LXQT_INTREE_INCLUDE_DIR}/lxqt/${bh} COPYONLY)
endforeach()

# Create the portable headers
create_portable_headers(INTREE_PORTABLE_HEADERS ${LXQT_INTREE_INCLUDE_DIR}/lxqt ${PUBLIC_CLASSES})
check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${INTREE_PORTABLE_HEADERS})

create_portable_headers(INTREE_PORTABLE_HEADERS ${LXQT_INTREE_INCLUDE_DIR}/LxQt ${PUBLIC_CLASSES})
check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${INTREE_PORTABLE_HEADERS})


#************************************************
# Create in-tree build infrastructure
#************************************************
set(CFG_LXQT_INCLUDE_DIR    ${LXQT_INTREE_INCLUDE_DIR})
set(CFG_LXQT_LIBRARY        ${LXQT_LIBRARY_NAME})
set(CFG_LXQT_USE_FILE       ${CMAKE_BINARY_DIR}/${LXQT_LIBRARY_NAME}_use.cmake)
set(CFG_LXQT_TRAGETS_FILE   ${LXQT_INTREE_TRAGETS_FILE})


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in 
    ${CMAKE_BINARY_DIR}/lxqt-config.cmake
    @ONLY
)

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
set(CFG_LXQT_USE_FILE       ${LXQT_INSTALL_CMAKE_DIR}${LXQT_LIBRARY_NAME}/${LXQT_LIBRARY_NAME}_use.cmake)
set(CFG_LXQT_TRAGETS_FILE   ${LXQT_INSTALL_CMAKE_DIR}${LXQT_LIBRARY_NAME}/${LXQT_LIBRARY_NAME}-targets.cmake)


configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in 
    ${CMAKE_CURRENT_BINARY_DIR}/install/lxqt-config.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqtX-config.cmake.in 
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}-config.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_use.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/install/${LXQT_LIBRARY_NAME}_use.cmake
    @ONLY
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/install/lxqt-config.cmake
    DESTINATION ${LXQT_INSTALL_CMAKE_DIR}/lxqt
    COMPONENT Devel
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
qt4_add_resources(QRC_CXX_SOURCES ${translatorsinfo_qrc})

add_library(${LXQT_LIBRARY_NAME}
    SHARED ${PUB_HDRS}
           ${PRIV_HDRS}
           ${SRCS}
           ${dbus_generated}
           ${UIS} ${MOCS}
           ${QM_FILES}
           ${QRC_CXX_SOURCES}
           
)


target_link_libraries(${LXQT_LIBRARY_NAME}
    ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${X11_X11_LIB}
    ${XDGQT_LIBRARIES}
)

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


export(TARGETS ${LXQT_LIBRARY_NAME} ${XDGQT_TARGET} FILE ${LXQT_INTREE_TRAGETS_FILE})



# Install in two places for inclusions like "include <lxde/lxdeclass.h>" and "include <LxQt/Class>".
install(FILES ${PUB_HDRS}         DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/lxqt)
install(FILES ${PORTABLE_HEADERS} DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/lxqt)

install(FILES ${PUB_HDRS}         DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/LxQt)
install(FILES ${PORTABLE_HEADERS} DESTINATION ${LXQT_INSTALL_INCLUDE_DIR}/LxQt)

install(FILES ${QM_FILES} DESTINATION ${LXQT_TRANSLATIONS_DIR})


#************************************************
# Create pkgconfig file
#************************************************
lxqt_create_pkgconfig_file(${LXQT_LIBRARY_NAME}
    ${LXGQT_PKG_CONFIG_DESCRIPTION}
    ${LXQT_PKG_CONFIG_REQUIRES}
    ${LXQT_LIBRARY_NAME}
    ${LXQT_VERSION}
)
#************************************************
