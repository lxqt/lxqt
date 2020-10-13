set(xdgiconengineplugin_CPP_FILES
    xdgiconengineplugin.cpp
)

add_library(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME} MODULE
    ${xdgiconengineplugin_CPP_FILES}
)

target_compile_definitions(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}
    PRIVATE
        "QT_NO_KEYWORDS"
)
target_link_libraries(${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}
    PUBLIC
        Qt5::Gui
        "${QTXDGX_ICONLOADER_LIBRARY_NAME}"
)

target_include_directories("${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}"
    PRIVATE
        "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

mark_as_advanced(QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH)

if (NOT DEFINED QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH)
    get_target_property(QT_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)
    if(NOT QT_QMAKE_EXECUTABLE)
        message(FATAL_ERROR "qmake is not found.")
    endif()

    # execute the command "qmake -query QT_INSTALL_PLUGINS" to get the path of plugins dir.
    execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE QT_PLUGINS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT QT_PLUGINS_DIR)
        message(FATAL_ERROR "Qt5 plugin directory cannot be detected.")
    endif()
    set(QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH "${QT_PLUGINS_DIR}/iconengines")
endif()

message(STATUS "XdgIconEnginePlugin will be installed into: ${QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH}")

install(TARGETS
    "${QTXDGX_ICONENGINEPLUGIN_LIBRARY_NAME}" DESTINATION "${QTXDGX_ICONENGINEPLUGIN_INSTALL_PATH}"
    COMPONENT Runtime
)

