cmake_minimum_required( VERSION 2.6 )

project(liblxqt)

set(LXQT_MAJOR_VERSION 0)
set(LXQT_MINOR_VERSION 5)
set(LXQT_PATCH_VERSION 0)


set(PUB_HDRS
    aboutdialog/lxqtaboutdialog.h
    razorsettings.h
    xfitman.h
    razorplugininfo.h
    powermanager.h
    addplugindialog/addplugindialog.h
    screensaver.h
    lxqtapplication.h
    libtranslate.h
    programfinder.h

    configdialog/configdialog.h
    configdialog/pageselectwidget.h

    lxqtpower/lxqtpower.h
    lxqtnotification.h
    lxqtautostartentry.h
    translatorsinfo/translatorsinfo.h
    lxqtgridlayout.h
    rotatedwidget.h
)

set(PUB_LINKS

    aboutdialog/LxQtAboutDialog
)

set(PRIV_HDRS
    aboutdialog/lxqtaboutdialog_p.h
    aboutdialog/technicalinfo.h
)

set(SRCS
    aboutdialog/lxqtaboutdialog.cpp
    aboutdialog/technicalinfo.cpp
    xfitman.cpp
    razorplugininfo.cpp
    powermanager.cpp
    addplugindialog/addplugindialog.cpp
    razorsettings.cpp
    screensaver.cpp
    lxqtapplication.cpp
    programfinder.cpp

    configdialog/configdialog.cpp
    configdialog/pageselectwidget.cpp

    lxqtpower/lxqtpower.cpp
    lxqtpower/lxqtpowerproviders.cpp
    lxqtnotification.cpp
    lxqtautostartentry.cpp
    translatorsinfo/translatorsinfo.cpp
    lxqtgridlayout.cpp
    rotatedwidget.cpp
)

set(MOCS
    aboutdialog/lxqtaboutdialog.h
    aboutdialog/lxqtaboutdialog_p.h
    powermanager.h
    addplugindialog/addplugindialog.h
    razorsettings.h
    screensaver.h
    lxqtapplication.h

    configdialog/configdialog.h
    configdialog/pageselectwidget.h

    lxqtnotification.h
    lxqtnotification_p.h
    lxqtpower/lxqtpower.h
    lxqtpower/lxqtpowerproviders.h
    lxqtgridlayout.h
    rotatedwidget.h
)

set(FORMS
    aboutdialog/lxqtaboutdialog.ui

    configdialog/configdialog.ui

    addplugindialog/addplugindialog.ui
)


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
    ${CMAKE_SOURCE_DIR}/libraries
)

set(LXQT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PATCH_VERSION})

message(STATUS "")
message(STATUS "liblxqt version: ${LXQT_VERSION}")
message(STATUS "")


#************************************************
# Build config.cmake files
#************************************************
set(LXQT_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/lxqt/)
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

set(APP_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/liblxqrt)
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")

# Translations **********************************
include(cmake/lxqt_use.cmake)
razor_translate_ts(QM_FILES
    SOURCES
        ${PUB_HDRS}
        ${PRIV_HDRS}
        ${SRCS}
        ${MOCS}
        ${FORMS}
    INSTALLATION_DIR
        ${APP_SHARE_DIR}
)

#************************************************


QT4_WRAP_CPP(MOCS ${MOCS})
QT4_WRAP_UI(UIS ${FORMS})

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
    qtxdg
)

set_target_properties(lxqt PROPERTIES
    VERSION   ${LXQT_VERSION}
    SOVERSION ${LXQT_MAJOR_VERSION}
)

install(TARGETS lxqt DESTINATION lib${LIB_SUFFIX})
install(FILES ${PUB_HDRS}  DESTINATION ${LXQT_INCLUDE_DIR})
install(FILES ${PUB_LINKS} DESTINATION ${LXQT_INCLUDE_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lxqt-config.cmake DESTINATION share/cmake/lxqt)
install(FILES cmake/lxqt_use.cmake DESTINATION share/cmake/lxqt)

include(cmake/create_pkgconfig_file.cmake)
create_pkgconfig_file(lxqt "Shared library for LXQt applications")
