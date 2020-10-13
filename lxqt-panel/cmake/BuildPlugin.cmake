MACRO (BUILD_LXQT_PLUGIN NAME)
    set(PROGRAM "lxqt-panel")
    project(${PROGRAM}_${NAME})

    set(PROG_SHARE_DIR ${CMAKE_INSTALL_FULL_DATAROOTDIR}/lxqt/${PROGRAM})
    set(PLUGIN_SHARE_DIR ${PROG_SHARE_DIR}/${NAME})

    # Translations **********************************
    lxqt_translate_ts(${PROJECT_NAME}_QM_FILES
        UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
        SOURCES
            ${HEADERS}
            ${SOURCES}
            ${MOCS}
            ${UIS}
        TEMPLATE
            ${NAME}
        INSTALL_DIR
            ${LXQT_TRANSLATIONS_DIR}/${PROGRAM}/${NAME}
    )

    #lxqt_translate_to(QM_FILES ${CMAKE_INSTALL_FULL_DATAROOTDIR}/lxqt/${PROGRAM}/${PROJECT_NAME})
    file (GLOB ${PROJECT_NAME}_DESKTOP_FILES_IN resources/*.desktop.in)
    lxqt_translate_desktop(DESKTOP_FILES
        SOURCES
            ${${PROJECT_NAME}_DESKTOP_FILES_IN}
    )

    lxqt_plugin_translation_loader(QM_LOADER ${NAME} "lxqt-panel")
    #************************************************

    file (GLOB CONFIG_FILES resources/*.conf)

    if (NOT DEFINED PLUGIN_DIR)
        set (PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/${PROGRAM})
    endif (NOT DEFINED PLUGIN_DIR)

    set(QTX_LIBRARIES Qt5::Widgets)
    if(QT_USE_QTXML)
        set(QTX_LIBRARIES ${QTX_LIBRARIES} Qt5::Xml)
    endif()
    if(QT_USE_QTDBUS)
        set(QTX_LIBRARIES ${QTX_LIBRARIES} Qt5::DBus)
    endif()

    list(FIND STATIC_PLUGINS ${NAME} IS_STATIC)
    set(SRC ${HEADERS} ${SOURCES} ${QM_LOADER} ${MOC_SOURCES} ${${PROJECT_NAME}_QM_FILES} ${RESOURCES} ${UIS} ${DESKTOP_FILES})
    if (${IS_STATIC} EQUAL -1) # not static
        add_library(${NAME} MODULE ${SRC}) # build dynamically loadable modules
        install(TARGETS ${NAME} DESTINATION ${PLUGIN_DIR}) # install the *.so file
    else() # static
        add_library(${NAME} STATIC ${SRC}) # build statically linked lib
    endif()
    target_link_libraries(${NAME} ${QTX_LIBRARIES} lxqt ${LIBRARIES} KF5::WindowSystem)

    install(FILES ${CONFIG_FILES}  DESTINATION ${PLUGIN_SHARE_DIR})
    install(FILES ${DESKTOP_FILES} DESTINATION ${PROG_SHARE_DIR})

ENDMACRO(BUILD_LXQT_PLUGIN)
