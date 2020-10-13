cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lxqt-panel)

option(UPDATE_TRANSLATIONS "Update source translation translations/*.ts files" OFF)
option(WITH_SCREENSAVER_FALLBACK "Include support for converting the deprecated 'screensaver' plugin to 'quicklaunch'. This requires the lxqt-leave (lxqt-session) to be installed in runtime." ON)
# plugin-mainmenu
option(USE_MENU_CACHE "Use menu-cached (no noticable penalty even on a 2004 single core pentium if not used)" OFF)


# additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

macro(setByDefault VAR_NAME VAR_VALUE)
  if(NOT DEFINED ${VAR_NAME})
    set (${VAR_NAME} ${VAR_VALUE})
  endif(NOT DEFINED ${VAR_NAME})
endmacro()

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(REQUIRED_QT_VERSION "5.10")
set(KF5_MINIMUM_VERSION "5.36.0")
set(LXQT_GLOBALKEYS_MINIMUM_VERSION "0.15.0")
set(LXQT_MINIMUM_VERSION "0.15.0")

find_package(Qt5DBus ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5LinguistTools ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Widgets ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5X11Extras ${REQUIRED_QT_VERSION} REQUIRED)
find_package(Qt5Xml ${REQUIRED_QT_VERSION} REQUIRED)
find_package(KF5WindowSystem ${KF5_MINIMUM_VERSION} REQUIRED)
find_package(lxqt ${LXQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt-globalkeys-ui ${LXQT_GLOBALKEYS_MINIMUM_VERSION} REQUIRED)

# Patch Version
set(LXQT_PANEL_PATCH_VERSION 1)
set(LXQT_PANEL_VERSION ${LXQT_MAJOR_VERSION}.${LXQT_MINOR_VERSION}.${LXQT_PANEL_PATCH_VERSION})
add_definitions("-DLXQT_PANEL_VERSION=\"${LXQT_PANEL_VERSION}\"")

include(LXQtPreventInSourceBuilds)
include(LXQtTranslate)

# All LXQtCompilerSettings except CMAKE_MODULE_LINKER_FLAGS work just fine
# So we reset only these Flags after loading LXQtCompilerSettings
# lxqt-build-tools:
#  set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined ${SYMBOLIC_FLAGS} ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "==OLD== CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}")
set( OLD_CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}")
include(LXQtCompilerSettings NO_POLICY_SCOPE)
set(CMAKE_MODULE_LINKER_FLAGS "${OLD_CMAKE_MODULE_LINKER_FLAGS} ${SYMBOLIC_FLAGS}")

# Warning: This must be before add_subdirectory(panel). Move with caution.
set(PLUGIN_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/lxqt-panel")
add_definitions(
    -DPLUGIN_DIR=\"${PLUGIN_DIR}\"
)

message(STATUS "CMAKE Module linker flags: ${CMAKE_MODULE_LINKER_FLAGS}")
message(STATUS "Panel plugins location: ${PLUGIN_DIR}")

#########################################################################
# Plugin system
# You can enable/disable building of the plugin using cmake options.
#    cmake -DWORLDCLOCK_PLUGIN=Yes .. # Enable worldclock plugin
#    cmake -DWORLDCLOCK_PLUGIN=No ..  # Disable worldclock plugin

include("cmake/BuildPlugin.cmake")

set(ENABLED_PLUGINS) # list of enabled plugins
set(STATIC_PLUGINS) # list of statically linked plugins

setByDefault(COLORPICKER_PLUGIN Yes)
if(COLORPICKER_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Color Picker")
    add_subdirectory(plugin-colorpicker)
endif()

setByDefault(CPULOAD_PLUGIN Yes)
if(CPULOAD_PLUGIN)
    find_library(STATGRAB_LIB statgrab)

    if(NOT STATGRAB_LIB)
        message(FATAL_ERROR "CPU Load plugin requires libstatgrab")
    endif()
    list(APPEND ENABLED_PLUGINS "Cpu Load")
    add_subdirectory(plugin-cpuload)
endif()

setByDefault(DIRECTORYMENU_PLUGIN Yes)
if(DIRECTORYMENU_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Directory menu")
    add_subdirectory(plugin-directorymenu)
endif()

setByDefault(DOM_PLUGIN No)
if(DOM_PLUGIN)
    list(APPEND ENABLED_PLUGINS "DOM")
    add_subdirectory(plugin-dom)
endif(DOM_PLUGIN)

setByDefault(DESKTOPSWITCH_PLUGIN Yes)
if(DESKTOPSWITCH_PLUGIN)
    list(APPEND STATIC_PLUGINS "desktopswitch")
    add_definitions(-DWITH_DESKTOPSWITCH_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Desktop Switcher")
    add_subdirectory(plugin-desktopswitch)
endif()

setByDefault(KBINDICATOR_PLUGIN Yes)
if(KBINDICATOR_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Keyboard Indicator")
    add_subdirectory(plugin-kbindicator)
endif(KBINDICATOR_PLUGIN)

setByDefault(MAINMENU_PLUGIN Yes)
if(MAINMENU_PLUGIN)
    list(APPEND STATIC_PLUGINS "mainmenu")
    add_definitions(-DWITH_MAINMENU_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Application menu")
    add_subdirectory(plugin-mainmenu)
endif()

setByDefault(MOUNT_PLUGIN Yes)
if(MOUNT_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Mount")
    add_subdirectory(plugin-mount)
endif(MOUNT_PLUGIN)

setByDefault(QUICKLAUNCH_PLUGIN Yes)
if(QUICKLAUNCH_PLUGIN)
    list(APPEND STATIC_PLUGINS "quicklaunch")
    add_definitions(-DWITH_QUICKLAUNCH_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Quicklaunch")
    add_subdirectory(plugin-quicklaunch)
endif()

setByDefault(SENSORS_PLUGIN Yes)
if(SENSORS_PLUGIN)
    find_library(SENSORS_LIB sensors)

    if(NOT SENSORS_LIB)
        message(FATAL_ERROR "Sensors plugin requires lm_sensors")
    endif()
    list(APPEND ENABLED_PLUGINS "Sensors")
    add_subdirectory(plugin-sensors)
endif()

setByDefault(SHOWDESKTOP_PLUGIN Yes)
if(SHOWDESKTOP_PLUGIN)
    list(APPEND STATIC_PLUGINS "showdesktop")
    add_definitions(-DWITH_SHOWDESKTOP_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Show Desktop")
    add_subdirectory(plugin-showdesktop)
endif()

setByDefault(NETWORKMONITOR_PLUGIN Yes)
if(NETWORKMONITOR_PLUGIN)
    find_library(STATGRAB_LIB statgrab)

    if(NOT STATGRAB_LIB)
        message(FATAL_ERROR "Network Monitor plugin requires libstatgrab")
    endif()
    list(APPEND ENABLED_PLUGINS "Network Monitor")
    add_subdirectory(plugin-networkmonitor)
endif()

setByDefault(SYSSTAT_PLUGIN Yes)
if(SYSSTAT_PLUGIN)
    list(APPEND ENABLED_PLUGINS "System Stats")
    add_subdirectory(plugin-sysstat)
endif(SYSSTAT_PLUGIN)

setByDefault(TASKBAR_PLUGIN Yes)
if(TASKBAR_PLUGIN)
  list(APPEND STATIC_PLUGINS "taskbar")
  add_definitions(-DWITH_TASKBAR_PLUGIN)
  list(APPEND ENABLED_PLUGINS "Taskbar")
  add_subdirectory(plugin-taskbar)
endif()

setByDefault(STATUSNOTIFIER_PLUGIN Yes)
if(STATUSNOTIFIER_PLUGIN)
    list(APPEND STATIC_PLUGINS "statusnotifier")
    add_definitions(-DWITH_STATUSNOTIFIER_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Status Notifier")
    add_subdirectory(plugin-statusnotifier)
endif()

setByDefault(TRAY_PLUGIN Yes)
if(TRAY_PLUGIN)
    list(APPEND STATIC_PLUGINS "tray")
    add_definitions(-DWITH_TRAY_PLUGIN)
    list(APPEND ENABLED_PLUGINS "System Tray")
    add_subdirectory(plugin-tray)
endif()

setByDefault(VOLUME_PLUGIN Yes)
setByDefault(VOLUME_USE_PULSEAUDIO Yes)
setByDefault(VOLUME_USE_ALSA Yes)
if(VOLUME_PLUGIN)
    if (VOLUME_USE_PULSEAUDIO)
        find_package(PulseAudio)
        if (NOT PULSEAUDIO_FOUND)
            message(FATAL_ERROR "PulseAudio not found, but required (VOLUME_USE_PULSEAUDIO) for Volume plugin!")
        endif ()
    endif(VOLUME_USE_PULSEAUDIO)

    if(VOLUME_USE_ALSA)
        find_package(ALSA)
        if (NOT ALSA_FOUND)
            message(FATAL_ERROR "ALSA not found, but required (VOLUME_USE_ALSA) for Volume plugin!")
        endif ()
    endif()

    list(APPEND ENABLED_PLUGINS   "Volume")
    message(STATUS "")
    message(STATUS "Volume plugin will be built")
    message(STATUS "    ALSA: ${ALSA_FOUND}")
    message(STATUS "    PulseAudio: ${PULSEAUDIO_FOUND}")
    message(STATUS "")
    add_subdirectory(plugin-volume)
endif()

setByDefault(WORLDCLOCK_PLUGIN Yes)
if(WORLDCLOCK_PLUGIN)
    list(APPEND STATIC_PLUGINS "worldclock")
    add_definitions(-DWITH_WORLDCLOCK_PLUGIN)
    list(APPEND ENABLED_PLUGINS "World Clock")
    add_subdirectory(plugin-worldclock)
endif(WORLDCLOCK_PLUGIN)

setByDefault(SPACER_PLUGIN Yes)
if(SPACER_PLUGIN)
    list(APPEND STATIC_PLUGINS "spacer")
    add_definitions(-DWITH_SPACER_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Spacer")
    add_subdirectory(plugin-spacer)
endif()

setByDefault(BACKLIGHT_PLUGIN Yes)
if(BACKLIGHT_PLUGIN)
    list(APPEND ENABLED_PLUGINS "Backlight")
    add_subdirectory(plugin-backlight)
endif()

#########################################################################

message(STATUS  "**************** The following plugins will be built ****************")
foreach (PLUGIN_STR ${ENABLED_PLUGINS})
    message(STATUS "  ${PLUGIN_STR}")
endforeach()
message(STATUS "*********************************************************************")

add_subdirectory(panel)

# merged from lxqt-common
add_subdirectory(autostart)
add_subdirectory(menu)
