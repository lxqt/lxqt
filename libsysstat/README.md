# libsysstat

## Overview

libsysstat is a library to query system information like CPU and memory usage or
network traffic. Conceptually it's roughly similar to
[libstatgrab](https://www.i-scream.org/libstatgrab), a third-party library used
by LXQt as well.

It is maintained by the LXQt project but can be used independently from this
desktop environment, too. As for LXQt libsysstat is used by plugin-sysstat of
lxqt-panel and hence an optional dependency of the latter.

Note replacing both libsysstat and libstatgrab in LXQt by KDE Frameworks components
is being considered but a final decision hasn't been made yet, see
https://github.com/lxqt/lxqt/issues/704.

## Installation

### Sources

The only runtime dependency is qtbase. To build CMake and
[lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools) are needed in
addition, as well as optionally Git to pull latest VCS checkouts.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
will normally have to be set to `/usr`, depending on the way library paths are
dealt with on 64bit systems variables like `CMAKE_INSTALL_LIBDIR` may have to be
set as well.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.

### Binary packages

The library is provided by all major Linux distributions like Arch Linux,
Debian, Fedora and openSUSE. Just use the distributions' package managers to
search for string `libsysstat`.
