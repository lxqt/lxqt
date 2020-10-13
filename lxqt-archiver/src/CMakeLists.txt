add_subdirectory(core)

include_directories(
    core
    ${GLIB_INCLUDE_DIRS}
    ${GLIB_JSON_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(lxqt-archiver_SRCS
    main.cpp
    mainwindow.cpp
    archiver.cpp
    archivererror.cpp
    archiveritem.cpp
    archiverproxymodel.cpp
    progressdialog.cpp
    passworddialog.cpp
    createfiledialog.cpp
    extractfiledialog.cpp
    filetreeView.cpp
)

set(lxqt-archiver_UI
    mainwindow.ui
    progressdialog.ui
    passworddialog.ui
    create.ui
    extract.ui
    about.ui
)

# Translations
lxqt_translate_ts(lxqt-archiver_QM_FILES
    UPDATE_TRANSLATIONS
        ${UPDATE_TRANSLATIONS}
    SOURCES
        corestrs.h  # translatable strings of src/core/*.c genreated using extract_c_strs.py
        ${lxqt-archiver_SRCS}
        ${lxqt-archiver_UI}
    INSTALL_DIR
        "${CMAKE_INSTALL_DATADIR}/lxqt-archiver/translations"
)

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
    lxqt-archiver.desktop.in
)

add_executable(lxqt-archiver
    ${lxqt-archiver_SRCS}
    ${lxqt-archiver_QM_FILES}
    ${lxqt-archiver_UI}
    ${DESKTOP_FILES}
)

target_compile_definitions(lxqt-archiver
    PRIVATE
        LXQT_ARCHIVER_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/lxqt-archiver"
        LXQT_ARCHIVER_VERSION="${LXQT_ARCHIVER_VERSION}"
        QT_NO_FOREACH
        QT_NO_KEYWORDS
)

target_link_libraries(lxqt-archiver
    Qt5::Widgets
    fm-qt
    lxqt-archiver-core
    ${GLIB_LIBRARIES}
    ${GLIB_JSON_LDFLAGS}
)

install(TARGETS
    lxqt-archiver
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)

install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)

install(
    FILES icons/lxqt-archiver.svg
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps"
)
