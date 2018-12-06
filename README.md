# lxqt-build-tools

## Introduction

This repository is providing several tools needed to build LXQt itself as well
as other components maintained by the LXQt project.

These tools used to be spread over the repositories of various other components
and were summarized to ease dependency management. So far many components, in
particular [liblxqt](https://github.com/lxqt/liblxqt), were representing a build
dependency without being needed themselves but only because their repository was
providing a subset of the tools which are now summarized here. So the use of this
repository will reduce superfluous and bloated dependencies.

## Installation

### Compiling sources

To build only CMake and Qt5Core are needed, optionally Git to pull VCS checkouts.
Runtime dependencies do not exist.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual. (Strictly speaking `make` isn't even needed right now. On the other
hand it doesn't hurt so packagers may just include it in case it'll be needed
one day.)

## Packagers

This package is arch-independent now.  You can simply package it as
`BuildArch: noarch` (rpm) or `arch: all` (deb).

## Current Minimum Versions

| Package                      | Version|
|------------------------------|--------|
| KF5_MINIMUM_VERSION          | 5.36.0 |
| KF5SCREEN_MINIMUM_VERSION    | 5.2.0  |
| LIBFM_QT_MINIMUM_VERSION     | 0.14.0 |
| LIBFMQT_MINIMUM_VERSION      | 0.14.0 |
| LIBMENUCACHE_MINIMUM_VERSION | 1.1.0  |
| LXQTBT_MINIMUM_VERSION       | 0.6.0  |
| LXQT_MINIMUM_VERSION         | 0.14.0 |
| QTERMWIDGET_MINIMUM_VERSION  | 0.14.0 |
| QT_MINIMUM_VERSION           | 5.7.1  |
| QTXDG_MINIMUM_VERSION        | 3.3.0  |
