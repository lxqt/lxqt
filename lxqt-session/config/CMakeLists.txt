# install default config files to /etc/xdg
install(FILES
    lxqt.conf
    session.conf
    windowmanagers.conf
    DESTINATION "${CMAKE_INSTALL_DATADIR}/lxqt"
    COMPONENT Runtime
)
install(DIRECTORY openbox
    DESTINATION "${LXQT_ETC_XDG_DIR}"
    COMPONENT Runtime
)
