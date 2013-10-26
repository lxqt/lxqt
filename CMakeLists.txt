cmake_minimum_required( VERSION 2.6 )

project(liblxqt)

set(LXQT_MAJOR_VERSION 0)
set(LXQT_MINOR_VERSION 5)
set(LXQT_PATCH_VERSION 0)


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

find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

QT4_ADD_DBUS_INTERFACE(SRCS
    dbus/org.freedesktop.Notifications.xml
    notifications_interface
)

find_package(X11 REQUIRED QUIET)

find_package(QTXDG REQUIRED QUIET)
include(${QTXDG_USE_FILE})


include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
    ${QTXDG_INCLUDE_DIRS}
)

set(LXQT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION})

message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


set(LXQT_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/lxqt)
set(LXQT_TRANSLATIONS_DIR ${LXQT_SHARE_DIR}/translations)
include(cmake/FindInstallConfigPath.cmake)
include(create_portable_headers)

#************************************************
# Create in-tree build infrastructure
#************************************************
if(INTREE_ROOT_DIR)
    # Copy public headers
    foreach(h ${PUB_HDRS})
        get_filename_component(bh ${h} NAME)
        configure_file(${h} ${INTREE_ROOT_DIR}/include/LxQt/${bh} COPYONLY)
        configure_file(${h} ${INTREE_ROOT_DIR}/include/lxqt/${bh} COPYONLY)
    endforeach()

    # Create the portable headers
    create_portable_headers(INTREE_PORTABLE_HEADERS ${INTREE_ROOT_DIR}/include/lxqt ${PUBLIC_CLASSES})
    check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${INTREE_PORTABLE_HEADERS})

    create_portable_headers(INTREE_PORTABLE_HEADERS ${INTREE_ROOT_DIR}/include/LxQt ${PUBLIC_CLASSES})
    check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${INTREE_PORTABLE_HEADERS})


    # Create config.cmake & use.cmake files
    set(LXQT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR};${INTREE_ROOT_DIR}/include")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in
        ${INTREE_ROOT_DIR}/cmake/lxqt-config.cmake
        @ONLY
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_use.cmake
        ${INTREE_ROOT_DIR}/cmake/lxqt_use.cmake
        @ONLY
    )
endif()


#************************************************
# Create instalable build infrastructure
#************************************************

# Create the portable headers
create_portable_headers(PORTABLE_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/include ${PUBLIC_CLASSES})
check_portable_headers(H_FILES ${PUB_HDRS} LINKS ${PORTABLE_HEADERS})


# Create config.cmake & use.cmake files
set(LXQT_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/lxqt-config.cmake
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt_use.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/lxqt_use.cmake
    @ONLY
)

#************************************************


qt4_wrap_cpp(MOCS ${MOCS})
qt4_wrap_ui(UIS ${FORMS})
qt4_add_translation(QM_FILES ${TS_FILES})
include(cmake/lxqt_use.cmake)


include(translatorsinfo/CMakeLists.txt)
get_translatorsinfo_qrc(translatorsinfo_qrc)
qt4_add_resources(QRC_CXX_SOURCES ${translatorsinfo_qrc})

add_library(lxqt
    SHARED ${PUB_HDRS}
           ${PRIV_HDRS}
           ${SRCS}
           ${dbus_generated}
           ${UIS} ${MOCS}
           ${QM_FILES}
           ${QRC_CXX_SOURCES})

target_link_libraries(lxqt
    ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${X11_X11_LIB}
    ${QTXDG_LIBRARIES}
)

set_target_properties(lxqt PROPERTIES
    VERSION   ${LXQT_VERSION}
    SOVERSION ${LXQT_MAJOR_VERSION}
)

install(TARGETS lxqt DESTINATION lib${LIB_SUFFIX})

# Install in two places for inclusions like "include <lxde/lxdeclass.h>" and "include <LxQt/Class>".
install(FILES ${PUB_HDRS}         DESTINATION ${LXQT_INCLUDE_DIR}/lxqt)
install(FILES ${PORTABLE_HEADERS} DESTINATION ${LXQT_INCLUDE_DIR}/lxqt)

install(FILES ${PUB_HDRS}         DESTINATION ${LXQT_INCLUDE_DIR}/LxQt)
install(FILES ${PORTABLE_HEADERS} DESTINATION ${LXQT_INCLUDE_DIR}/LxQt)

install(FILES ${QM_FILES} DESTINATION ${LXQT_TRANSLATIONS_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/lxqt-config.cmake DESTINATION share/cmake/lxqt)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake/lxqt_use.cmake    DESTINATION share/cmake/lxqt)

include(cmake/create_pkgconfig_file.cmake)
create_pkgconfig_file(lxqt "Shared library for LXQt applications")
