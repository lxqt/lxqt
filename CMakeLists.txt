project(librazorqt)
find_package(X11 REQUIRED)

include_directories (
	${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
	${X11_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/libraries
)

set(razorqt_PUB_HDRS
    aboutdialog/razoraboutdlg.h
    razorsettings.h
    xfitman.h
    razorplugininfo.h
    powermanager.h
    addplugindialog/addplugindialog.h
    screensaver.h
    razorapplication.h
    razorshortcutbutton.h
    # upower. Maybe standalone lib?
    #librazor-upower/battery.h
    #librazor-upower/ucpufreq.h
    #librazor-upower/upower.h
    libtranslate.h
    pageselectwidget.h
    razorconfigdialog.h
    razorpower/razorpower.h
    razornotification.h
    translatorsinfo/translatorsinfo.h
)

set(razorqt_PRIV_HDRS
    aboutdialog/razoraboutdlg_p.h
    aboutdialog/technicalinfo.h
)

set(razorqt_SRCS
    aboutdialog/razoraboutdlg.cpp
    aboutdialog/technicalinfo.cpp
    xfitman.cpp
    razorplugininfo.cpp
    powermanager.cpp
    addplugindialog/addplugindialog.cpp
    razorsettings.cpp
    screensaver.cpp
    razorapplication.cpp
    razorshortcutbutton.cpp
    # upower. Maybe standalone lib?
    #librazor-upower/battery.cpp
    #librazor-upower/ucpufreq.cpp
    #librazor-upower/upower.cpp
    pageselectwidget.cpp
    razorconfigdialog.cpp
    razorpower/razorpower.cpp
    razorpower/razorpowerproviders.cpp
    razornotification.cpp
    translatorsinfo/translatorsinfo.cpp

)

set(razorqt_MOCS
    aboutdialog/razoraboutdlg.h
    aboutdialog/razoraboutdlg_p.h
    powermanager.h
    addplugindialog/addplugindialog.h
    razorsettings.h
    screensaver.h
    razorapplication.h
    razorshortcutbutton.h
    razorshortcutbutton_p.h
    # upower. Maybe standalone lib?
    #librazor-upower/battery.h
    #librazor-upower/ucpufreq.h
    #librazor-upower/upower.h
    pageselectwidget.h
    razorconfigdialog.h
    razornotification.h
    razornotification_p.h
    razorpower/razorpower.h
    razorpower/razorpowerproviders.h
)

QT4_ADD_DBUS_INTERFACE(razorqt_SRCS
    ../../razorqt-resources/dbus/org.freedesktop.Notifications.xml
    notifications_interface
)

set( razorqt_UIS
    aboutdialog/razoraboutdlg.ui
    razorconfigdialog.ui
    addplugindialog/addplugindialog.ui
)

#message(STATUS "+++++++++++++++++++"  ${dbus_generated})

set(APP_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/librazorqt)
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")

# Translations **********************************
include(RazorTranslate)
razor_translate_ts(RAZORQT_QM_FILES
    SOURCES
        ${razorqt_PUB_HDRS}
        ${razorqt_PRIV_HDRS}
        ${razorqt_SRCS}
        ${razorqt_MOCS}
        ${razorqt_UIS}
    INSTALLATION_DIR
        ${APP_SHARE_DIR}
)

#************************************************


QT4_WRAP_CPP(MOCS ${razorqt_MOCS})
QT4_WRAP_UI(UIS ${razorqt_UIS})


include(RazorLibSuffix)

include(translatorsinfo/CMakeLists.txt)
get_translatorsinfo_qrc(translatorsinfo_qrc)
qt4_add_resources(QRC_CXX_SOURCES ${translatorsinfo_qrc})

add_library ( razorqt SHARED ${razorqt_PUB_HDRS} ${razorqt_PRIV_HDRS} ${razorqt_SRCS} ${dbus_generated} ${UIS} ${MOCS} ${RAZORQT_QM_FILES} ${QRC_CXX_SOURCES})

target_link_libraries ( razorqt  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY}
                                 ${QT_QTDBUS_LIBRARY}
                                 ${X11_X11_LIB}
                                 qtxdg
                      )

set_target_properties(razorqt PROPERTIES
  VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}
  SOVERSION ${MAJOR_VERSION}
)

install(TARGETS razorqt DESTINATION lib${LIB_SUFFIX})
install(FILES ${razorqt_PUB_HDRS} DESTINATION include/razorqt)

include(create_pkgconfig_file)
create_pkgconfig_file(razorqt "Shared library for Razor-qt applications")
