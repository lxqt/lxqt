project("lxqt-leave")

set(CPP_FILES
    main.cpp
    leavedialog.cpp
    listwidget.cpp
)

set(H_FILES
    leavedialog.h
    listwidget.h
)

set(UI_FILES
    leavedialog.ui
)


# Translations **********************************
lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
    resources/lxqt-leave.desktop.in
    resources/lxqt-hibernate.desktop.in
    resources/lxqt-lockscreen.desktop.in
    resources/lxqt-logout.desktop.in
    resources/lxqt-reboot.desktop.in
    resources/lxqt-shutdown.desktop.in
    resources/lxqt-suspend.desktop.in
)

lxqt_translate_ts(lxqt-leave_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        ${CPP_FILES}
        ${UI_FILES}
        ${H_FILES}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(lxqt-leave_QM_LOADER ${PROJECT_NAME})
#************************************************


add_executable(lxqt-leave
    ${CPP_FILES}
    ${DESKTOP_FILES}
    ${lxqt-leave_QM_LOADER}
    ${lxqt-leave_QM_FILES}
)

target_link_libraries(lxqt-leave
    lxqt
)

install(TARGETS
    lxqt-leave
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES} DESTINATION
    "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
install(FILES
    resources/lxqt-leave.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    COMPONENT Runtime
)
