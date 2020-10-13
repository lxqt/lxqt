set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(QTXDG_DESKTOP_FILE_START_SRCS
    qtxdg-desktop-file-start.cpp
)

set(QTXDG_ICONFINDER_SRCS
    qtxdg-iconfinder.cpp
)

add_executable(qtxdg-desktop-file-start
    ${QTXDG_DESKTOP_FILE_START_SRCS}
)

add_executable(qtxdg-iconfinder
    ${QTXDG_ICONFINDER_SRCS}
)

target_include_directories(qtxdg-desktop-file-start
    PRIVATE "${PROJECT_SOURCE_DIR}/qtxdg"
)

target_include_directories(qtxdg-iconfinder
    PRIVATE "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

target_compile_definitions(qtxdg-desktop-file-start
    PRIVATE
        "-DQTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
        "QT_NO_KEYWORDS"
)

target_compile_definitions(qtxdg-iconfinder
    PRIVATE
        "-DQTXDG_VERSION=\"${QTXDG_VERSION_STRING}\""
        "QT_NO_KEYWORDS"
)

target_link_libraries(qtxdg-desktop-file-start
    ${QTXDGX_LIBRARY_NAME}
)

target_link_libraries(qtxdg-iconfinder
    ${QTXDGX_ICONLOADER_LIBRARY_NAME}
)

install(TARGETS
    qtxdg-desktop-file-start
    qtxdg-iconfinder
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
