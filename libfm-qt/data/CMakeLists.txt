install(FILES
    "archivers.list"
    "terminals.list"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/libfm-qt"
)

install(FILES
    "libfm-qt-mimetypes.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
)
