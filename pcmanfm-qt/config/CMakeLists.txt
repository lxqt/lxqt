# install default config files to /etc/xdg

configure_file(pcmanfm-qt/lxqt/settings.conf.in pcmanfm-qt/lxqt/settings.conf @ONLY)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt/lxqt/settings.conf"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/pcmanfm-qt/lxqt"
    COMPONENT Runtime
)
