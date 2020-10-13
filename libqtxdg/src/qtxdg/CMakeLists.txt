set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QTX_LIBRARIES Qt5::Widgets Qt5::Xml Qt5::DBus)

set(libqtxdg_PUBLIC_H_FILES
    xdgaction.h
    xdgdesktopfile.h
    xdgdirs.h
    xdgicon.h
    xdgmenu.h
    xdgmenuwidget.h
    xmlhelper.h
    xdgautostart.h
    xdgmacros.h
    xdgmimetype.h
    xdgmimeapps.h
    xdgdefaultapps.h
)

set(libqtxdg_PUBLIC_CLASSES
    XdgAction
    XdgDesktopFile
    XdgDirs
    XdgIcon
    XdgMenu
    XdgMenuWidget
    XmlHelper
    XdgAutoStart
    XdgMimeType
    XdgMimeApps
    XdgDefaultApps
)

set(libqtxdg_PRIVATE_H_FILES
    qtxdglogging.h
    xdgmenuapplinkprocessor.h
    xdgmenulayoutprocessor.h
    xdgmenu_p.h
    xdgmenureader.h
    xdgmenurules.h
    xdgdesktopfile_p.h
    xdgmimeapps_p.h
)

set(libqtxdg_CPP_FILES
    qtxdglogging.cpp
    xdgaction.cpp
    xdgdesktopfile.cpp
    xdgdirs.cpp
    xdgicon.cpp
    xdgmenuapplinkprocessor.cpp
    xdgmenu.cpp
    xdgmenulayoutprocessor.cpp
    xdgmenureader.cpp
    xdgmenurules.cpp
    xdgmenuwidget.cpp
    xmlhelper.cpp
    xdgautostart.cpp
    xdgmimetype.cpp
    xdgmimeapps.cpp
    xdgmimeappsbackendinterface.cpp
    xdgmimeappsglibbackend.cpp
    xdgdefaultapps.cpp
)

QT5_ADD_DBUS_INTERFACE(libqtxdg_DBUS_INTERFACE_SRCS
    dbus/org.freedesktop.Application.xml
    application_interface
)

set_property(SOURCE ${libqtxdg_DBUS_INTERFACE_SRCS} PROPERTY SKIP_AUTOGEN ON)

add_library(${QTXDGX_LIBRARY_NAME} SHARED
    ${libqtxdg_PUBLIC_H_FILES}
    ${libqtxdg_PRIVATE_H_FILES}
    ${libqtxdg_CPP_FILES}
    ${libqtxdg_DBUS_INTERFACE_SRCS}
)

target_link_libraries(${QTXDGX_LIBRARY_NAME}
    PUBLIC
        ${QTX_LIBRARIES}
        ${QTXDGX_ICONLOADER_LIBRARY_NAME}
        ${GLIB_LIBRARIES}
        ${GLIB_GOBJECT_LIBRARIES}
        ${GLIB_GIO_LIBRARIES}
)

set_target_properties(${QTXDGX_LIBRARY_NAME} PROPERTIES
  VERSION ${QTXDG_VERSION_STRING}
  SOVERSION ${QTXDG_MAJOR_VERSION}
)

target_compile_definitions(${QTXDGX_LIBRARY_NAME}
    PRIVATE
        "QTXDG_COMPILATION=\"1\""
        "QTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
        "QT_NO_KEYWORDS"
)

target_include_directories(${QTXDGX_LIBRARY_NAME}
    INTERFACE
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}>"
        "$<BUILD_INTERFACE:${QTXDGX_INTREE_INCLUDEDIR}>"
    PRIVATE
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
        ${GLIB_GIO_UNIX_INCLUDE_DIR}
)

# create the portble headers
lxqt_create_portable_headers(libqtxdg_PORTABLE_HEADERS
    HEADER_NAMES ${libqtxdg_PUBLIC_CLASSES}
    OUTPUT_DIR "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
)

# Copy public headers (in tree building)
foreach(h ${libqtxdg_PUBLIC_H_FILES})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${bh}" COPYONLY)
endforeach()

# Copy private headers (in tree building)
foreach(h ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES})
    get_filename_component(bh ${h} NAME)
    configure_file(${h} "${QTXDGX_INTREE_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}/private/qtxdg/${bh}" COPYONLY)
endforeach()

install(TARGETS
    ${QTXDGX_LIBRARY_NAME} DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    EXPORT "${QTXDGX_FILE_NAME}-targets"
    COMPONENT Runtime
)

install(FILES
    ${libqtxdg_PUBLIC_H_FILES}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)

install(FILES
    ${libqtxdg_PRIVATE_INSTALLABLE_H_FILES}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}/${QTXDG_VERSION_STRING}/private/qtxdg"
    COMPONENT Devel
)

install(FILES
    ${libqtxdg_PORTABLE_HEADERS}
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${QTXDGX_FILE_NAME}"
    COMPONENT Devel
)
