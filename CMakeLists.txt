
find_package(X11 REQUIRED)

include_directories (
	${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
    ${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
	${X11_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/libraries
)

set(razorqt_HDRS
    razoraboutdlg.h
    razorsettings.h
    xdgautostart.h
    xfitman.h
    razorplugininfo.h
    powermanager.h
    addplugindialog/addplugindialog.h
    screensaver.h
    razorshortcutbutton.h
    razorshortcutbutton_p.h
    # upower. Maybe standalone lib?
    #librazor-upower/battery.h
    #librazor-upower/ucpufreq.h
    #librazor-upower/upower.h
    libtranslate.h
    pageselectwidget.h
    razorpower/razorpower.h
)

set(razorqt_SRCS
    razoraboutdlg.cpp
    xdgautostart.cpp
    xfitman.cpp
    razorplugininfo.cpp
    powermanager.cpp
    addplugindialog/addplugindialog.cpp
    razorsettings.cpp
    screensaver.cpp
    razorshortcutbutton.cpp
    # upower. Maybe standalone lib?
    #librazor-upower/battery.cpp
    #librazor-upower/ucpufreq.cpp
    #librazor-upower/upower.cpp
    pageselectwidget.cpp
    razorpower/razorpower.cpp
    razorpower/razorpowerproviders.cpp
)

set(razorqt_MOCS
    razoraboutdlg.h
    powermanager.h
    addplugindialog/addplugindialog.h
    razorsettings.h
    screensaver.h
    razorshortcutbutton.h
    razorshortcutbutton_p.h
    # upower. Maybe standalone lib?
    #librazor-upower/battery.h
    #librazor-upower/ucpufreq.h
    #librazor-upower/upower.h
    pageselectwidget.h
    razorpower/razorpower.h
    razorpower/razorpowerproviders.h
)

# Translations **********************************
file(GLOB RAZORQT_TS_FILES
    translations/*.ts
)
#************************************************


QT4_WRAP_CPP(MOCS ${razorqt_MOCS})
qt4_add_translation(RAZORQT_QM_FILES ${RAZORQT_TS_FILES})


set(APP_SHARE_DIR ${CMAKE_INSTALL_PREFIX}/share/librazorqt)
add_definitions(-DTRANSLATIONS_DIR=\"${APP_SHARE_DIR}\")

set ( razorqt_UIS
    razoraboutdlg.ui
    addplugindialog/addplugindialog.ui
)

QT4_WRAP_UI(UIS ${razorqt_UIS})


include(RazorLibSuffix)

add_library ( razorqt SHARED ${razorqt_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} ${RAZORQT_QM_FILES})

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
install(FILES ${razorqt_HDRS} DESTINATION include/razorqt)
install(FILES ${RAZORQT_QM_FILES}     DESTINATION ${APP_SHARE_DIR})

include(create_pkgconfig_file)
create_pkgconfig_file(razorqt "Shared library for Razor-qt applications")
