# lxqt-qtplugin

## Overview

This repository is providing a library `libqtlxqt` to integrate Qt with LXQt.

With this plugin, all Qt-based programs can adopt settings of LXQt, such as the icon theme.

## Installation

### Compiling source code

Runtime dependencies are libdbusmenu-qt5 and [liblxqt](https://github.com/lxqt/liblxqt).   
Additional build dependencies are CMake and qttools, optionally Git to pull latest VCS checkouts.   

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has to be set to `/usr` on most operating systems.   

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual.   

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your package manager to search for string `lxqt-qtplugin`.

## Configuration, Usage

To use the plugin in Qt 5, we have to export environment variable `QT_QPA_PLATFORMTHEME=lxqt`. Then every Qt5 program can load the theme plugin.   
If, for some unknown reasons, the plugin is not loaded, we can debug the plugin by exporting `QT_DEBUG_PLUGINS=1`. Then, Qt5 will print detailed information and error messages about all plugins in the console when running any Qt5 programs.
