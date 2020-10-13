# PCManFM-Qt

## Overview

PCManFM-Qt is the Qt port of PCManFM, the file manager of [LXDE](https://lxde.org).

In LXQt sessions it is in addition used to handle the desktop. Nevertheless it 
can be used independently of LXQt as well.

PCManFM-Qt is licensed under the terms of the 
[GPLv2](https://www.gnu.org/licenses/gpl-2.0.en.html) or any later version. See 
file LICENSE for its full text.   

## Installation

### Compiling source code

Runtime dependencies are qtx11extras, lxmenu-data,
[liblxqt](https://github.com/lxqt/liblxqt) and
[libfm-qt](https://github.com/lxqt/libfm-qt).
Additional build dependencies is CMake.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` 
has to be set to `/usr` on most operating systems, depending on the way library 
paths are dealt with on 64bit systems variables like `CMAKE_INSTALL_LIBDIR` may 
have to be set as well.   

To build run `make`, to install `make install` which accepts variable `DESTDIR` 
as usual.

### Binary packages

Official binary packages are available in Arch Linux, Debian (as of Debian stretch), 
Fedora (version 0.10.0 only so far) and openSUSE (Leap and Tumbleweed).   

## Usage

The file manager functionality should be self-explanatory, handling of the 
desktop deserves some notes:

To handle the desktop binary `pcmanfm-qt` has to be launched with switch 
`--desktop` set. Optionally switch `--profile` can be used to safe settings 
specific to certain session types like the different desktop environments.   
In LXQt sessions, PCManFM-Qt is launched with theses switches set as 
[LXQt Module](https://github.com/lxqt/lxqt-session#lxqt-modules).   

To configure the desktop there's a dialogue "Desktop Preferences". Technically 
it corresponds with launching `pcmanfm-qt` with switch `--desktop-pref` set. It 
is available in the desktop's context menu and included as topic "Desktop" in 
sub-menu Preferences - LXQt settings of the panel's main menu as well as the 
[Configuration Center](https://github.com/lxqt/lxqt-config#configuration-center) 
of lxqt-config.   

All switches (command line options) mentioned above are explained in detail in
`man 1 pcmanfm-qt`.   

## Development

Issues should go to the tracker of PCManFM-Qt at https://github.com/lxqt/pcmanfm-qt/issues.


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/pcmanfm-qt/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/pcmanfm-qt/multi-auto.svg" alt="Translation status" />
</a>
