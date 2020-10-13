set (extedit_SRC
    moduleextedit.cpp
    extedit.cpp
)

set (extedit_HDR
    extedit.h
)

lxqt_translate_ts(extedit_QMS
    USE_QT5 TRUE
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    TEMPLATE "extedit"
    TRANSLATION_DIR "${CMAKE_SOURCE_DIR}/translations"
    SOURCES
        ${extedit_SRC}
        ${extedit_HDR}
    INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/translations"
)

qt5_translation_loader(extedit_QM_LOADER
    "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}/translations"
    "extedit"
)

add_library(extedit
    SHARED
        ${extedit_SRC}
        ${extedit_QMS}
        ${extedit_QM_LOADER}
)
set_property (TARGET extedit PROPERTY SOVERSION 1.0.0)
install(TARGETS extedit DESTINATION ${SG_LIBDIR})

target_link_libraries(extedit Qt5::Widgets Qt5::X11Extras Qt5Xdg)
