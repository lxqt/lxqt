# lxqt-globalkeys

## Overview

This repository is providing tools to set global keyboard shortcuts in LXQt
sessions, that is shortcuts which apply to the LXQt session as a whole and are
not limited to distinct applications.

The core components are two binaries `lxqt-globalkeysd` and
`lxqt-config-globalkeyshortcuts`. `lxqt-globalkeysd` is running in a daemon-like
manner as so-called [LXQt Module](https://github.com/lxqt/lxqt-session#lxqt-modules)
and doing the actual work. GUI `lxqt-config-globalkeyshortcuts` is used to
customize the shortcut settings.

## Installation

### Compiling source code

The only runtime dependencies is [liblxqt](https://github.com/lxqt/liblxqt).
Additional build dependencies are CMake and optionally Git to pull latest VCS
checkouts. The localization files were outsourced to repository
[lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies
are needed, too. Please refer to this repository's `README.md` for further
information.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems, depending on the way library
paths are dealt with on 64bit systems variables like `CMAKE_INSTALL_LIBDIR` may
have to be set as well.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch
Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your
package manager to search for string `lxqt-globalkeys`.

## Configuration, Usage

### Caveat lxqt-globalkeys vs. window managers

Window managers can assign shortcuts as well and their scope may overlap with
the one of lxqt-globalkeys in LXQt sessions. This was e. g. affecting elderly
configuration files `rc.xml` of Openbox which were assigning `Ctrl+Alt+D` to
toggling the display of the desktop which lxqt-globalkeys tries to do as well.
This resulted in a warning message "Global shortcut C+A+d+ cannot be registered",
see https://github.com/lxqt/lxqt/issues/1032. As depicted in this issue
[lxqt-notificationd](https://github.com/lxqt/lxqt-notificationd) will show a
warning when conflicts like this come up and users can thus decide whether they
want to have the shortcut in question handled by the respective window manager
or lxqt-globalkeys.

### lxqt-globalkeys

Daemon-like `lxqt-globalkeysd` can be adjusted from section "Basic Settings" in
configuration dialogue [LXQt Session Settings](https://github.com/lxqt/lxqt-session#overview)
of [lxqt-session](https://github.com/lxqt/lxqt-session).

Configuration dialogue "Global Actions Manager" (binary `lxqt-config-globalkeyshortcuts`)
which is used to customize shortcuts can be opened from the panel's main menu -
Preferences - LXQt Settings - Shortcut Keys and is provided by the
[Configuration Center](https://github.com/lxqt/lxqt-config#configuration-center)
of [lxqt-config](https://github.com/lxqt/lxqt-config) as well.

### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-globalkeys-config/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-globalkeys-config/multi-auto.svg" alt="Translation status" />
</a>
