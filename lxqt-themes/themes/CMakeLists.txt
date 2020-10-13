install(DIRECTORY
    .
    DESTINATION "${LXQT_SHARE_DIR}/themes"
    COMPONENT Runtime
    PATTERN "CMake*"
    EXCLUDE PATTERN "cmake*"
    EXCLUDE PATTERN "Makefile" EXCLUDE
)
