cmake_minimum_required( VERSION 2.6 )

project(liblxqt)

set(LXQT_MAJOR_VERSION 0)
set(LXQT_MINOR_VERSION 5)
set(LXQT_PATCH_VERSION 0)

# Set default installation paths
set(LIB_INSTALL_DIR "lib${LIB_SUFFIX}" CACHE PATH "Installation path for libraries")

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
    LxQtAboutDialog
    LxQtSettings
    LxQtXfitMan
    LxQtPluginInfo
    LxQtPowerManager
    LxQtAddPluginDialog
    LxQtScreenSaver
    LxQtApplication
    LxQtTranslator
    LxQtProgramFinder

    LxQtConfigDialog
    LxQtPageSelectWidget

    LxQtPower
    LxQtNotification
    LxQtAutostartEntry
    LxQtGridLayout
    LxQtRotatedWidget
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

set(LXQT_VERSION
    ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION}
)

message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


#************************************************
# Build config.cmake files
#************************************************
set(LXQT_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/lxqt)
set(LXQT_TRANSLATIONS_DIR ${LXQT_SHARE_DIR}/translations)
include(cmake/FindInstallConfigPath.cmake)

set(LXQT_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/lxqt)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/lxqt-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/lxqt-config.cmake
    @ONLY
)
#************************************************
# End of build config.cmake
#************************************************
include_directories(${LXQT_INCLUDE_DIR})

set(APP_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/liblxqt)


# use gcc visibility feature to decrease unnecessary exported symbols
if (CMAKE_COMPILER_IS_GNUCXX)
    # set visibility to hidden to hide symbols, unlesss they're exporeted
    # manually in the code
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fno-exceptions -Wl,-no-undefined -Wall")
endif()
add_definitions(-DCOMPILE_LIBLXQT)

# create the portable headers *******************
include(create_portable_headers)
create_portable_headers(PORTABLE_HEADERS ${PUBLIC_CLASSES})
check_portable_headers("${PUB_HDRS}")
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

install(TARGETS lxqt DESTINATION ${LIB_INSTALL_DIR})
install(FILES ${PUB_HDRS}  DESTINATION ${LXQT_INCLUDE_DIR})
install(FILES ${PORTABLE_HEADERS} DESTINATION ${LXQT_INCLUDE_DIR})
install(FILES ${QM_FILES} DESTINATION ${LXQT_TRANSLATIONS_DIR})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/lxqt-config.cmake
    DESTINATION share/cmake/lxqt
)
install(FILES cmake/lxqt_use.cmake DESTINATION share/cmake/lxqt)

include(cmake/create_pkgconfig_file.cmake)
create_pkgconfig_file(lxqt "Shared library for LXQt applications")

# building tarball with CPack -------------------------------------------------
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VERSION_MAJOR ${LXQT_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${LXQT_MINOR_VERSION})
set (CPACK_PACKAGE_VERSION_PATCH ${LXQT_PATCH_VERSION})
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include (CPack)
