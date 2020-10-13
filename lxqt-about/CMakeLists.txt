cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-about)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CPP_FILES
    aboutdialog/lxqtaboutdialog.cpp
    aboutdialog/technicalinfo.cpp
    translatorsinfo/translatorsinfo.cpp
    main.cpp
)

set(UI_FILES
    aboutdialog/lxqtaboutdialog.ui
)

# Minimum versions
set(LXQT_MINIMUM_VERSION "0.15.0")

find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

## Patch Version
set(LXQT_ABOUT_PATCH_VERSION 0)

set(LXQT_ABOUT_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_ABOUT_PATCH_VERSION})
add_definitions(
    "-DLXQT_ABOUT_VERSION=\"${LXQT_ABOUT_VERSION}\""
)

# Translations **********************************
include(LXQtTranslate)

lxqt_translate_ts(QM_FILES
    UPDATE_TRANSLATIONS ${UPDATE_TRANSLATIONS}
    SOURCES
        ${H_FILES}
        ${CPP_FILES}
        ${UI_FILES}
    INSTALL_DIR
        "${LXQT_TRANSLATIONS_DIR}/${PROJECT_NAME}"
)

lxqt_app_translation_loader(QM_LOADER ${PROJECT_NAME})

lxqt_translate_desktop(DESKTOP_FILES
    SOURCES
    resources/lxqt-about.desktop.in
)
#************************************************

include(translatorsinfo/CMakeLists.txt)
get_translatorsinfo_qrc(translatorsinfo_qrc)

add_executable(${PROJECT_NAME}
    ${CPP_FILES}
    ${QM_FILES}
    ${DESKTOP_FILES}
    ${translatorsinfo_qrc}
    ${QM_LOADER}

)

target_link_libraries(${PROJECT_NAME}
    lxqt
)

install(TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
install(FILES
    ${DESKTOP_FILES}
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    COMPONENT Runtime
)
