# lxqt-powermanagement

## Overview

This repository is providing tools to monitor power management events and optionally trigger actions like e. g. shut down a system when laptop batteries are low on power.

The core components are two binaries `lxqt-powermanagement` and `lxqt-config-powermanagement`.   
`lxqt-powermanagement` is running in a daemon-like manner as so-called "LXQt Module" and doing the actual work. GUI "Power Management Settings (binary `lxqt-config-powermanagement`) is used to customize settings.

Warning messages are displayed on the desktop by [lxqt-notificationd](https://github.com/lxqt/lxqt-notificationd).

## Installation

### Compiling source code

Runtime dependencies are UPower, KIdleTime, qtsvg and [liblxqt](https://github.com/lxqt/liblxqt).   
Additional build dependencies are CMake and Solid, optionally Git to pull latest VCS checkouts. The localization files were outsourced to repository [lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies are needed, too. Please refer to this repository's `README.md` for further information.   

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has to be set to `/usr` on most operating systems.   

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual.   

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your package manager to search for string `lxqt-powermanagement`.

## Configuration, Usage

Daemon-like `lxqt-powermanagement` can be adjusted from section "Basic Settings" in configuration dialogue "LXQt Session Settings" (binary `lxqt-config-session`) of [lxqt-session](https://github.com/lxqt/lxqt-session).

To customize settings there's configuration dialogue "Power Management Settings" (binary `lxqt-config-powermanagement`). It can be opened from the panel's main menu - Preferences - LXQt Settings - Power Management and is provided by the "Configuration Center" (binary `lxqt-config`) of [lxqt-config](https://github.com/lxqt/lxqt-config) as well.

## Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-powermanagement">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-powermanagement/287x66-white.png" alt="Translation status" />
</a>    <a href="https://weblate.lxqt.org/projects/lxqt/lxqt-config-powermanagement">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-config-powermanagement/287x66-white.png" alt="Translation status" />
</a>
