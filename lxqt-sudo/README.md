# lxqt-sudo

## Overview

lxqt-sudo is a graphical front-end of commands `sudo` and `su` respectively. As such it enables regular users to launch applications with permissions of other users including root.   

## Installation

### Compiling source code

Runtime dependencies are qtbase, sudo (su should be installed by default on all *ix operating systems) and [liblxqt](https://github.com/lxqt/liblxqt).   
Installing at least one icon theme according to the [XDG Icon Theme Specification](https://www.freedesktop.org/wiki/Specifications/icon-theme-spec/) like e. g. "Oxygen Icons" is recommended to have the GUI display icons.   
Additional build dependencies are CMake and optionally Git to pull latest VCS checkouts. The localization files were outsourced to repository [lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies are needed, too. Please refer to this repository's `README.md` for further information.   

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has to be set to `/usr` on most operating systems.   

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual.   

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch Linux, Debian (as of Debian stretch), Fedora and openSUSE. It's also available on FreeBSD. Just use your package manager to search for string `lxqt-sudo`.

## Configuration

lxqt-sudo itself does not require any configuration.   

In order to use it as front-end of `sudo` the corresponding permissions have to be set, though. Most of the time this is handled by binary `visudo` or editing configuration file `/etc/sudoers` manually which both is beyond this document's scope.   

## Usage

lxqt-sudo comes with a man page explaining the syntax very well so running `man 1 lxqt-sudo` should get you started.   

By default `sudo` is used as backend, the choice can be enforced by command line options `--su[do]` or by using symbolic links `lxsu` and `lxsudo` which belong to regular installations of lxqt-sudo.   


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-sudo/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-sudo/multi-auto.svg" alt="Translation status" />
</a>
