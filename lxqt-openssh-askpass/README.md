# lxqt-openssh-askpass

## Overview

`lxqt-openssh-askpass` is a GUI to query credentials on behalf of other programs.
As indicated by its name it's primarily targeted at `ssh-agent`, the SSH agent
of OpenSSH, but it works with other applications like e. g. EncFS as well.

It was considered to abandon the tool in favour of KDE's `ksshaskpass` and
lxqt-openssh-askpass had temporarily been declared deprecated for this reason.
But it turned out the close bond of `ksshaskpass` to KWallet conflicts with LXQt's
design goals so it's all but certain the replacement will happen.
See https://github.com/lxqt/lxqt/issues/362.

## Installation

### Compiling source code

The only runtime dependency is [liblxqt](https://github.com/lxqt/liblxqt).
Additional build dependencies are CMake and optionally Git to pull latest VCS
checkouts. The localization files were outsourced to repository
[lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies
are needed, too. Please refer to this repository's `README.md` for further
information.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch
Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your
package manager to search for string `lxqt-openssh-askpass`.

## Configuration, Usage

### ssh-agent

First of all it must be ensured `ssh-agent` is running in LXQt sessions. This is
basically beyond this document's scope but can e. g. be achieved by a user
systemd unit
```
[Unit]
Description=Some arbitrary description

[Service]
Type=forking
ExecStart=/usr/bin/ssh-agent -a /run/user/<ID>/ssh-agent

[Install]
WantedBy=default.target
```
where ID corresponds with the respective user's UID as displayed by `$ id <user>`.

Environment variable `SSH_AUTH_SOCK` must point to the socket of `ssh-agent` as
e. g. stated by option `-a` in the systemd unit depicted above. Environment
variable `SSH_ASKPASS` must be set to `lxqt-openssh-askpass` to indicate this
binary should be used.
Both variables can be set in section "Environment (Advanced)" of configuration
dialogue [LXQt Session Settings](https://github.com/lxqt/lxqt-session#lxqt-session-settings)
of [lxqt-session](https://github.com/lxqt/lxqt-session). Changes apply upon the
next login only.

Note binary `ssh-add` which is used to register keys with `ssh-agent` will use
GUI tools like `lxqt-openssh-askpass` only when it is *not* attached to a terminal.
So `lxqt-openssh-askpass` will not be used when `ssh-add` is launched from a
terminal emulator like QTerminal even when everything is configured as stated
above. `lxqt-openssh-askpass` will be used when the invocation of `ssh-add` is
handled by an autostart entry which can be configured in section "Autostart" of
"LXQt Session Settings" or when a desktop entry file is used to invoke the tool
from menus.

### EncFS

Simply hand `lxqt-openssh` to binary `encfs` by option `--extpass`, like in
`encfs --extpass=lxqt-openssh-askpass <rootdir> <mount point>`.
In contrast to `ssh-{agent,add}` this works when `encfs` is launched from a
terminal emulator, too.


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-openssh-askpass/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-openssh-askpass/multi-auto.svg" alt="Translation status" />
</a>
