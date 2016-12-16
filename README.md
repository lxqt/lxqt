# lxqt-build-tools

## Introduction

This repository is providing several tools needed to build LXQt itself as well as other components maintained by the LXQt project.   

These tools used to be spread over the repositories of various other components and were summarized to ease dependency management. So far many components, in particular [liblxqt](https://github.com/lxde/liblxqt), were representing a build dependency without being needed themselves but only because their repository was providing a subset of the tools which are now summarized here. So the use of this repository will reduce superfluous and bloated dependencies.   

## Installation

### Compiling sources

To build only CMake and Qt5Core are needed, optionally Git to pull VCS checkouts. Runtime dependencies do not exist.   

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has to be set to `/usr` on most operating systems.   

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual. (Strictly speaking `make` isn't even needed right now. On the other hand it doesn't hurt so packagers may just include it in case it'll be needed one day.)

## Packagers

Please keep in mind that lxqt-build-tools are arch dependend - the package picks up **32/64bit-ness** at build time of the package - so the package has to be built for the respective architecture, arch: all will **not** fly.
