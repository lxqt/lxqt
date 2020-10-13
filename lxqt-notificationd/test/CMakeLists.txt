set(NotifyTest_SRCS
    mainwindow.cpp
    main.cpp
)

add_executable(NotifyTest
    ${NotifyTest_SRCS}
)

target_link_libraries(NotifyTest
    lxqt
)
