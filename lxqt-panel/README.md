# lxqt-panel

## Overview

`lxqt-panel` represents the taskbar of LXQt.

The elements available in lxqt-panel are called "plugin" technically. This applies e. g. to the source code where they reside in directories `./plugin-<plugin>` like `plugin-mainmenu`. In contrast to this they are called "widgets" by the configuration GUI so far. Also, a more descriptive term is used to refer to distinct plugins within the GUI. E. g. the aforementioned `plugin-mainmenu` is called "Application menu" that way.
Configuration dialogue "Add Plugins", see [below](https://github.com/lxqt/lxqt-panel#customizing), is listing all available plugins plus a short description and hence provides an overview of the available ones.
Notes on some of the plugins, sorted by terms used within the GUI in alphabetical order, technical term in parenthesis:

#### Date & time (plugin-clock) / World clock (plugin-worldclock)

Both provide clock and calendar functionality. plugin-worldclock can display various time zones in addition but lacks a tooltip displaying current date and time upon hovering.
These plugins will probably be merged into one, see https://github.com/lxqt/lxqt/issues/312.

#### Quick launch (plugin-quicklaunch)

A plugin to launch applications from the panel. By default it is empty and displays a message "Drop application icons here". Applications need to be available in panel's main menu and can be included into plugin-quicklaunch by drag & drop.

#### Status Notifier Plugin (plugin-statusnotifier) / System Tray (plugin-tray)

Both provide a notification area within the panel, that is an area where arbitrary applications can place informational icons. This is frequently used e. g. by chat or mail clients to inform about incoming messages or tools configuring the network to inform about connections. (So it's some kind of counterpart to the desktop notifications displayed by [lxqt-notificationd](https://github.com/lxqt/lxqt-notificationd)).
The difference between the two plugins is a technical one. **plugin-tray** is implementing the so-called [System Tray Protocol](https://www.freedesktop.org/wiki/Specifications/systemtray-spec). It's a specification that has been around for years but has some serious technical limitations and in particular won't work under Wayland. **plugin-statusnotifier** on the other hand is implementing the so-called [StatusNotifierItem (SNI)](https://www.freedesktop.org/wiki/Specifications/StatusNotifierItem) specification which can be considered a successor of the System Tray Protocol.
Both plugins are maintained in parallel as not all relevant applications are compatible with SNI so far. In particular both Qt 4 and all GTK applications need some kind of wrapper to deal with it. Both plugins can be used in parallel without any issue, applications supporting both specifications will normally chose to display their icons in plugin-statusnotifier.

#### Volume control (plugin-volume)

As indicated by the name, a volume control. Technically Alsa, OSS and PulseAudio can be used as backend. The plugin itself is providing a control to adjust the main volume only but it allows for launching specific UIs of the backend in use like e. g. [pavucontrol-qt](https://github.com/lxqt/pavucontrol-qt) to adjust PulseAudio.

## Installation

### Compiling source code

The runtime dependencies are libxcomposite, libdbusmenu-qt5, KGuiAddons, KWindowSystem, Solid, menu-cache, lxmenu-data, [liblxqt](https://github.com/lxqt/liblxqt) and [lxqt-globalkeys](https://github.com/lxqt/lxqt-globalkeys).
Several plugins or features thereof are optional and need additional runtime dependencies. Namely these are (plugin / feature in parenthesis) Alsa library (Alsa support in plugin-volume), PulseAudio client library (PulseAudio support in plugin-volume), lm-sensors (plugin-sensors), libstatgrab (plugin-cpuload, plugin-networkmonitor), [libsysstat](https://github.com/lxqt/libsysstat) (plugin-sysstat). All of them are enabled by default and have to be disabled by CMake variables as required, see below.
In addition CMake is a mandatory build dependency. Git is optionally needed to pull latest VCS checkouts. The localization files were outsourced to repository [lxqt-l10n](https://github.com/lxqt/lxqt-l10n) so the corresponding dependencies are needed, too. Please refer to this repository's `README.md` for further information.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX` has to be set to `/usr` on most operating systems, depending on the way library paths are dealt with on 64bit systems variables like CMAKE_INSTALL_LIBDIR may have to be set as well.
By default all available plugins and features thereof are built and CMake fails when dependencies aren't met. Building particular plugins can be disabled by boolean CMake variables `<plugin>_PLUGIN` where the plugin is referred by its technical term like e. g. in `SYSSTAT_PLUGIN`. Alsa and PulseAudio support in plugin-volume can be disabled by boolean CMake variables `VOLUME_USE_ALSA` and `VOLUME_USE_PULSEAUDIO`.

To build run `make`, to install `make install` which accepts variable `DESTDIR` as usual.

### Binary packages

Official binary packages are provided by all major Linux distributions like Arch Linux, Debian (as of Debian stretch only), Fedora and openSUSE. Just use your package manager to search for string `lxqt-panel`.

## Configuration, Usage

### Launching

The panel is run as a daemon-like [LXQt Module](https://github.com/lxqt/lxqt-session#lxqt-modules) the launch of which can be adjusted in section "Basic Settings" of configuration dialogue [LXQt Session Settings](https://github.com/lx/lxqt-session#lxqt-session-settings) of [lxqt-session](https://github.com/lxqt/lxqt-session).

### Customizing

To customize the panel itself there's a context menu, that is a menu opened by right-clicking the pointer device. It is comprising sections "\<plugin\>" and "Panel" which allow for configuring the plugin the pointer is currently over and the panel as a whole respectively.

In section "Panel" topics "Configure Panel" and "Manage Widgets" open different panes of a dialogue "Configure Panel" which allow for configuring the panel as a whole and the various plugins respectively.
Pane "Widgets" allows for configuring and removing all plugins currently included in lxqt-panel. The plus sign opens another dialogue "Add plugins" which is used to add plugins. It comes with a list of all plugins plus some short descriptions and can hence serve as overview what plugins are available.

Sometimes right-clicking over particular plugins may bring up a context menu dealing with the respective plugin's functionality *only* which means the plugin in question cannot be configured the usual way. This affects e. g. plugin-quicklaunch as soon as items were added (the context menu is limited to topics dealing with the items included in plugin-quicklaunch).
Currently there are two ways to deal with this. Some themes like e. g. `Frost` come with handles at the plugins' left end providing the regular context menu. Also, it can be assumed at least one plugin is included in the panel that's always featuring the regular context menu like e. g. plugin-mainmenu. Either way pane "Widgets" of "Configure Panel" can be accessed and used to configure the particular plugin.

## Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-colorpicker/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-colorpicker/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-cpuload/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-cpuload/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-desktopswitch/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-desktopswitch/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-directorymenu/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-directorymenu/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-dom/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-dom/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-kbindicator/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-kbindicator/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-mainmenu/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-mainmenu/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-mount/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-mount/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-networkmonitor/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-networkmonitor/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-quicklaunch/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-quicklaunch/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-sensors/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-sensors/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-showdesktop/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-showdesktop/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-spacer/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-spacer/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-statusnotifier/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-statusnotifier/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-sysstat/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-sysstat/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-taskbar/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-taskbar/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-tray/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-tray/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-volume/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-volume/287x66-white.png" alt="Translation status" />
</a>
<a href="https://weblate.lxqt.org/projects/lxqt/lxqt-panel-plugin-worldclock/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/lxqt-panel-plugin-worldclock/287x66-white.png" alt="Translation status" />
</a>
