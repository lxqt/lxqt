include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GLIB_JSON_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
)

add_definitions(
    -DSHDIR=\"${CMAKE_INSTALL_LIBEXECDIR}/lxqt-archiver\"
    -DPRIVEXECDIR=\"${CMAKE_INSTALL_LIBEXECDIR}/lxqt-archiver\"
)

add_library(lxqt-archiver-core STATIC
    tr-wrapper.c  # our own wrapper for QTranslater
    file-data.c
    file-utils.c
    fr-archive.c
    fr-command-7z.c
    fr-command-ace.c
    fr-command-alz.c
    fr-command-ar.c
    fr-command-arj.c
    fr-command.c
    fr-command-cfile.c
    fr-command-cpio.c
    fr-command-dpkg.c
    fr-command-iso.c
    fr-command-jar.c
    fr-command-lha.c
    fr-command-lrzip.c
    fr-command-rar.c
    fr-command-rpm.c
    fr-command-tar.c
    fr-command-unarchiver.c
    fr-command-unstuff.c
    fr-command-zip.c
    fr-command-zoo.c
    fr-enum-types.c
    fr-error.c
    fr-init.c
    fr-marshal.c
    fr-proc-error.c
    fr-process.c
    gio-utils.c
    glib-utils.c
    java-utils.c
    rar-utils.c
)

target_link_libraries(lxqt-archiver-core
    ${GLIB_LIBRARIES}
    ${GLIB_JSON_LDFLAGS}
)

add_executable(rpm2cpio
    commands/rpm2cpio.c
)
target_link_libraries(rpm2cpio
    ${GLIB_LIBRARIES}
    ${GLIB_JSON_LDFLAGS}
)
install(TARGETS
    rpm2cpio
    DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/lxqt-archiver"
    COMPONENT Runtime
)


set(SCRIPT_FILES
    sh/isoinfo.sh
)

install(FILES
    ${SCRIPT_FILES}
    DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/lxqt-archiver"
    COMPONENT Runtime
)
