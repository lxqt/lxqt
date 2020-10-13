# libfm-qt

## Overview

libfm-qt is the Qt port of libfm, a library providing components to build
desktop file managers which belongs to [LXDE](https://lxde.org).

libfm-qt is licensed under the terms of the
[LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
or any later version. See file LICENSE for its full text.   

fm-qt-config.cmake.in is licensed under the terms of the
[BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)

## Installation

### Compiling source code

Runtime dependencies are Qt X11 Extras and libfm ≥ 1.2
(not all features are provided by libfm-qt yet).   
Additional build dependencies are CMake,
[lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools) and optionally Git
to pull latest VCS checkouts. The localization files were outsourced to
repository [lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding
dependencies are needed, too. Please refer to this repository's `README.md` for
further information.   

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` 
has to be set to `/usr` on most operating systems, depending on the way library
paths are dealt with on 64bit systems variables like `CMAKE_INSTALL_LIBDIR` may
have to be set as well.   

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.   

### Binary packages

Official binary packages are available in Arch Linux, Debian (as of Debian
stretch) and openSUSE (Leap 42.1 and Tumbleweed).   
The library is still missing in Fedora which is providing version 0.10.0 of
PCManFM-Qt only so far. This version was still including the code outsourced
into libfm-qt later so libfm-qt will have to be provided by Fedora, too,
as soon as the distribution upgrades to PCManFM-Qt ≥ 0.10.1.   

## Development

Issues should go to the tracker of PCManFM-Qt at
https://github.com/lxqt/pcmanfm-qt/issues.


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/libfm-qt/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/libfm-qt/multi-auto.svg" alt="Translation status" />
</a>
