set(pcmanfm_SRCS
    application.cpp
    pcmanfm.cpp
    mainwindow.cpp
    tabpage.cpp
    tabbar.cpp
    statusbar.cpp
    view.cpp
    launcher.cpp
    preferencesdialog.cpp
    xdgdir.cpp
    desktoppreferencesdialog.cpp
    desktopwindow.cpp
    autorundialog.cpp
    connectserverdialog.cpp
    settings.cpp
    bulkrename.cpp
    desktopentrydialog.cpp
)

qt5_add_dbus_adaptor(pcmanfm_DBUS_SRCS
    org.pcmanfm.Application.xml
    application.h
    PCManFM::Application
    applicationadaptor
    ApplicationAdaptor
)

# qt5_add_dbus_adaptor() already generated the moc files. It also marked the
# files with SKIP_AUTOMOC but we still need to mark them witk SKIP_AUTOGEN.
# TODO: Check if this behaviour is a CMake bug.
set_source_files_properties(${pcmanfm_DBUS_SRCS} PROPERTIES SKIP_AUTOGEN ON)

set(pcmanfm_UIS
    main-win.ui
    about.ui
    shortcuts.ui
    preferences.ui
    desktop-preferences.ui
    desktop-folder.ui
    autorun.ui
    connect.ui
    bulk-rename.ui
    desktopentrydialog.ui
)

# add translation for pcmanfm-qt
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES ${pcmanfm_SRCS} ${pcmanfm_UIS}
    INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

# translate desktop entry files for pcmanfm-qt and desktop preferences
lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
        pcmanfm-qt.desktop.in
        pcmanfm-qt-desktop-pref.desktop.in
)

add_executable(pcmanfm-qt
    ${pcmanfm_SRCS}
    ${pcmanfm_DBUS_SRCS}
    ${pcmanfm_UIS}
    ${QM_FILES}
    ${DESKTOP_FILES}
)

target_compile_definitions(pcmanfm-qt
    PRIVATE
        PCMANFM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/pcmanfm-qt"
        PCMANFM_QT_VERSION="${PCMANFM_QT_VERSION}"
        LIBFM_DATA_DIR="${PKG_FM_PREFIX}/share/libfm"
)

target_include_directories(pcmanfm-qt
    PRIVATE
        "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

target_link_libraries(pcmanfm-qt
    Qt5::X11Extras
    Qt5::Widgets
    Qt5::DBus
    fm-qt
)

install(TARGETS pcmanfm-qt RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# install a desktop entry file for pcmanfm-qt and desktop preferences
install(FILES ${DESKTOP_FILES} DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)
