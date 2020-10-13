include_directories(
    ${X11_INCLUDE_DIR}
    ${XFIXES_INCLUDE_DIRS}
)

# For some unknown reasons these are required for build on FreeBSD
link_directories(
    ${QT_LIBRARY_DIR}
    ${GLIB_LIBRARY_DIRS}
)

set(lximage-qt_SRCS
    lximage-qt.cpp
    mainwindow.cpp
    preferencesdialog.cpp
    application.cpp
    imageview.cpp
    modelfilter.cpp
    loadimagejob.cpp
    saveimagejob.cpp
    resizeimagedialog.cpp
    screenshotdialog.cpp
    screenshotselectarea.cpp
    screenshotselectareagraphicsview.cpp
    settings.cpp
    graphicsscene.cpp
    mrumenu.cpp
    statusbar.cpp

    upload/imageshackprovider.cpp
    upload/imageshackupload.cpp
    upload/imgbbprovider.cpp
    upload/imgbbupload.cpp
    upload/imgurprovider.cpp
    upload/imgurupload.cpp
    upload/provider.cpp
    upload/upload.cpp
    upload/uploaddialog.cpp

    resource.qrc
)

qt5_add_dbus_adaptor(lximage-qt_DBUS_SRCS
    org.lxde.LxImage.Application.xml
    application.h
    LxImage::Application
    applicationadaptor
    ApplicationAdaptor
)
set_property(SOURCE ${lximage-qt_DBUS_SRCS} PROPERTY SKIP_AUTOGEN ON)
list(APPEND lximage-qt_SRCS "${lximage-qt_DBUS_SRCS}")

set(lximage-qt_UIS
    mainwindow.ui
    preferencesdialog.ui
    resizeimagedialog.ui
    screenshotdialog.ui

    upload/uploaddialog.ui
)

# add translation for lximage-qt
include(LXQtTranslateTs)
lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${lximage-qt_SRCS}
        ${lximage-qt_UIS}
    INSTALL_DIR
        "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

# process desktop entry files
include(LXQtTranslateDesktop)

file(GLOB desktop_files_in ../data/*.desktop.in)
lxqt_translate_desktop(desktop_files SOURCES ${desktop_files_in})
install(FILES ${desktop_files} DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")

add_executable(lximage-qt
    ${lximage-qt_SRCS}
    ${lximage-qt_UIS}
    ${desktop_files}
    ${QM_FILES}
)

add_definitions(
    -DLXIMAGE_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/lximage-qt"
    -DLXIMAGE_VERSION="${LXIMAGE_VERSION}"
)

set(QT_LIBRARIES Qt5::Widgets Qt5::Network Qt5::Core Qt5::DBus Qt5::PrintSupport Qt5::X11Extras Qt5::Svg)

target_link_libraries(lximage-qt
    fm-qt
    ${QT_LIBRARIES}
    ${EXIF_LIBRARIES}
    ${X11_LIBRARIES}
    ${XFIXES_LDFLAGS}
)

install(TARGETS lximage-qt RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
