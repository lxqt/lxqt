# lxqt-notificationd

## Overview

`lxqt-notificationd` is LXQt's implementation of a daemon according to the
[Desktop Notifications Specification](https://developer.gnome.org/notification-spec/).

This specification defines a way to display notifications in pop-up windows on
desktops. Notifications like this are e. g. frequently used by chat or mail
clients to inform about incoming messages, by media players to indicate the
beginning of another track but also by the desktop environment itself to indicate
volume changes or similar.

lxqt-notificationd is comprising binaries `lxqt-notificationd` and
`lxqt-config-notificationd`.
`lxqt-notificationd` is running in a daemon-like manner as so-called
[LXQt Module](https://github.com/lxqt/lxqt-session#lxqt-modules) and doing the
actual work. GUI "Desktop Notifications", binary `lxqt-config-notificationd`, is
used to customize the notifications.

lxqt-notificationd is implementing version 1.2 of the said specification.

## Installation

### Compiling source code

Runtime dependencies are KWindowSystem, [liblxqt](https://github.com/lxqt/liblxqt)
and [lxqt-common](https://github.com/lxqt/lxqt-common).
Additional build dependencies are CMake and optionally Git to pull latest VCS
checkouts. The localization files were outsourced to repository
[lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies
are needed, too. Please refer to this repository's `README.md` for further information.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has
to be set to `/usr` on most operating systems.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch
Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your
package manager to search for string `lxqt-notificationd`.

## Configuration, Usage

As `lxqt-notificationd` is run as LXQt Module it can be adjusted from section
"Basic Settings" in configuration dialogue
[LXQt Session Settings](https://github.com/lxqt/lxqt-session#lxqt-session-settings)
of [lxqt-session](https://github.com/lxqt/lxqt-session).

Configuration dialogue "Desktop Notifications" (binary `lxqt-config-notificationd`)
which is used to customize notifications can be opened from the panel's main
menu - Preferences - LXQt Settings - Desktop Notifications and is provided by the
[Configuration Center](https://github.com/lxqt/lxqt-config#configuration-center)
of [lxqt-config](https://github.com/lxqt/lxqt-config) as well.

## Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-notificationd/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-notificationd/287x66-white.png" alt="Translation status" />
</a>

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-config-notificationd/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-config-notificationd/287x66-white.png" alt="Translation status" />
</a>
