# libqtxdg

## Overview

`libqtxdg` is a Qt 5 implementation of freedesktop.org XDG specifications.   

It is maintained by the LXQt project and nearly all LXQt components are depending on it. Yet it can be used independently from this desktop environment, too.   

The library is able to use GTK+ icon theme caches for faster icon lookup. The cache file can be generated with utility `gtk-update-icon-cache` on a theme directory. If the cache is not present, corrupted, or outdated, the normal slow lookup is still run.   

## Installation

### Sources

At runtime qtbase is needed. gtk-update-icon-cache represents an optional runtime dependency for the reasons stated above.   
Additional build dependencies are CMake, qtsvg, qttools and optionally Git to pull latest VCS checkouts.

The code configuration is handled by CMake so all corresponding generic instructions apply. Specific CMake variables are
* BUILD_TESTS to build tests. Disabled by default (`OFF`).
* BUILD_DEV_UTILS which builds and installs development utils. Disabled by default as well.

To build and install run `make` and `make install`respectively.

### Binary packages

The library is provided by all major Linux distributions like Arch Linux, Debian, Fedora and openSUSE.   
Just use the distributions' package managers to search for string `libqtxdg`.

