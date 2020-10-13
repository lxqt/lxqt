# install default config files to /etc/xdg
install(FILES
    globalkeyshortcuts.conf
    DESTINATION "${CMAKE_INSTALL_DATADIR}/lxqt"
    COMPONENT Runtime
)
