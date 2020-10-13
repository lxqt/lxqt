add_executable(qtxdg-mat
    matcommandmanager.cpp
    matcommandinterface.cpp
    defappmatcommand.cpp
    openmatcommand.cpp
    mimetypematcommand.cpp
    defwebbrowsermatcommand.cpp
    defemailclientmatcommand.cpp
    deffilemanagermatcommand.cpp

    qtxdg-mat.cpp
)

target_compile_definitions(qtxdg-mat
    PRIVATE
        "-DQTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
        "QT_NO_KEYWORDS"
)

target_link_libraries(qtxdg-mat
    ${QTXDGX_LIBRARY_NAME}
)

install(TARGETS
    qtxdg-mat
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
