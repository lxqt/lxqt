project(lxqt-admin-user)

# build static helper class first
include_directories (
    ${CMAKE_CURRENT_BINARY_DIR}
)

set ( lxqt-admin-user_SRCS
    main.cpp
    mainwindow.cpp
    userdialog.cpp
    groupdialog.cpp
    usermanager.cpp
)

set ( lxqt-admin-user_MOCS
    mainwindow.h
    userdialog.h
    groupdialog.h
    usermanager.h
)

set( lxqt-admin-user_UIS
    mainwindow.ui
    userdialog.ui
    groupdialog.ui
)

qt5_wrap_cpp(MOCS ${lxqt-admin-user_MOCS})
qt5_wrap_ui(UIS ${lxqt-admin-user_UIS})

# Translations **********************************
lxqt_translate_ts(lxqt-admin-user_QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
    ${lxqt-admin-user_MOCS}
    ${lxqt-admin-user_SRCS}
    ${lxqt-admin-user_UIS}
    INSTALL_DIR
    "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
    lxqt-admin-user.desktop.in
)
lxqt_app_translation_loader(lxqt-admin-user_QM_LOADER ${PROJECT_NAME})
#************************************************

add_executable(lxqt-admin-user
    ${lxqt-admin-user_SRCS}
    ${UIS}
    ${MOCS}
    ${DESKTOP_FILES}
    ${lxqt-admin-user_QM_FILES}
    ${lxqt-admin-user_QM_LOADER}
)

target_link_libraries(lxqt-admin-user
    KF5::WindowSystem
    Qt5::Widgets
    lxqt
)

install(TARGETS lxqt-admin-user RUNTIME DESTINATION bin)
install(FILES ${DESKTOP_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)

# for policykit
# manpage for pcmanfm-qt
configure_file(
    "org.lxqt.lxqt-admin-user.policy.in"
    "${CMAKE_CURRENT_BINARY_DIR}/org.lxqt.lxqt-admin-user.policy"
    @ONLY
)

if(${CMAKE_SYSTEM_NAME} MATCHES  "FreeBSD")
    install(PROGRAMS "lxqt-admin-user-helper.freebsd" DESTINATION bin RENAME lxqt-admin-user-helper)
else()
    install(PROGRAMS "lxqt-admin-user-helper.default" DESTINATION bin RENAME lxqt-admin-user-helper)
endif()
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.lxqt.lxqt-admin-user.policy" DESTINATION "${POLKITQT-1_POLICY_FILES_INSTALL_DIR}")
