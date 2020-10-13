# QTermWidget

## Overview

A terminal emulator widget for Qt 5.

QTermWidget is an open-source project originally based on the KDE4 Konsole application, but it took its own direction later on.
The main goal of this project is to provide a unicode-enabled, embeddable Qt widget for using as a built-in console (or terminal emulation widget).

It is compatible with BSD, Linux and OS X.

This project is licensed under the terms of the [GPLv2](https://www.gnu.org/licenses/gpl-2.0.en.html) or any later version. See the LICENSE file for the full text of the license. Some files are published under compatible licenses:
```
Files: example/main.cpp
       lib/TerminalCharacterDecoder.cpp
       lib/TerminalCharacterDecoder.h
       lib/kprocess.cpp
       lib/kprocess.h
       lib/kpty.cpp
       lib/kpty.h
       lib/kpty_p.h
       lib/kptydevice.cpp
       lib/kptydevice.h
       lib/kptyprocess.cpp
       lib/kptyprocess.h
       lib/qtermwidget.cpp
       lib/qtermwidget.h
Copyright: Author Adriaan de Groot <groot@kde.org>
           2010, KDE e.V <kde-ev-board@kde.org>
           2002-2007, Oswald Buddenhagen <ossi@kde.org>
           2006-2008, Robert Knight <robertknight@gmail.com>
           2002, Waldo Bastian <bastian@kde.org>
           2008, e_k <e_k@users.sourceforge.net>
License: LGPL-2+

Files: pyqt/cmake/*
Copyright: 2012, Luca Beltrame <lbeltrame@kde.org>
           2012, Rolf Eike Beer <eike@sf-mail.de>
           2007-2014, Simon Edwards <simon@simonzone.com>
License: BSD-3-clause

Files: cmake/FindUtf8Proc.cmake
Copyright: 2009-2011, Kitware, Inc
           2009-2011, Philip Lowman <philip@yhbt.com>
License: BSD-3-clause

Files: pyqt/cmake/PythonCompile.py
License: public-domain
```

## Installation

### Compiling sources

The only runtime dependency is qtbase ≥ 5.7.1.
In order to build CMake ≥ 3.0.2 and [lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools/) >= 0.4.0 are needed as well as Git to pull translations and optionally latest VCS checkouts.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` will normally have to be set to `/usr`, depending on the way library paths are dealt with on 64bit systems variables like `CMAKE_INSTALL_LIBDIR` may have to be set as well.

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual.

### Binary packages

The library is provided by all major Linux distributions like Arch Linux, Debian, Fedora and openSUSE.
Just use the distributions' package managers to search for string `qtermwidget`.


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/qtermwidget/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/qtermwidget/multi-auto.svg" alt="Translation status" />
</a>
