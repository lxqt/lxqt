project(lxqt-backlight_backend)

set(C_FILES
    lxqtbacklight_backend.c
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
    ${C_FILES}
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
