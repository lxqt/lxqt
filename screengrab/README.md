# ScreenGrab

##### version 2.0-dev

ScreenGrab - A program for fast creating screenshots, and easily publishing them on internet image hosting services. It works on Linux and Windows operating systems. ScreenGrab uses the Qt framework and thus, it is independent from any desktop environment.

### Build requirements

 * Qt5 >= 5.2 (Qt 4.x support only 1.x branch)
 * CMake >= 3.1.0 (only for building ScreenGrab from sources)
 * GCC > 4.5
 * KF5WindowSystem
 * [optional] Qxt Library > 0.6 (if you want to build ScreenGrab using your system Qxt version - see the "Build options" section in this file)

### Build

To build ScreenGrab from sources, use these commands:

	mkdir build
	cd build
	cmake [BUILD OPTIONS] ../
	make
	make install

### Build options

You can use some or all of these parameters to customise your build.

 * **-DCMAKE_INSTALL_PREFIX** - Install prefix for Linux distro. Default setting: "/usr".
 * **-DSG_XDG_CONFIG_SUPPORT** - Place config files into XDGC_CONFIG_HOME directory (usually - ~/.config/${app_name) ). Default setting: ON. In previous versions the config files were stored in ~/.screengrab (Set this parameter to "OFF" to store the config files here).
 * **-DSG_EXT_UPLOADS** - Enable uploading screenshots to image hosting services. Default setting: ON.
 * **-DSG_DBUS_NOTIFY** - Enable D-Bus notifications.
 * **-DSG_GLOBALSHORTCUTS** - Enable global shortcuts for main actions to create screenshots. Default setting: ON.
 * **-DSG_USE_SYSTEM_QXT** - Use system version Qxt Library for global shortcuts. Default setting: OFF.
 * **-DSG_DOCDIR** - Name for the directory of user's documentation. Default setting:  "screengrab".
 * **-DQKSW_SHARED** - Enable shared linking with qkeysequencewidget library (in src/common/qksysekwesewidget). Default setting: OFF.
 * **-DDEV_VERSION** - Set a dev-version here, maybe git describe $foo. Default not set.

#### Build notes

For Debian based Linux distributions please use the distribution build tools. One can get the source-code with
```
apt source screengrab # Sources for the released package
- or -
git clone https://salsa.debian.org/lxqt-team/screengrab
```
Build with the tools provided by `devscripts` or better with `pbuilder` or `sbuild`

### LICENSE

Screengrab is licensed under the GPL v2. See file LICENSE in docs directory for more information

### Contacts

Web homepage: https://github.com/lxqt/screengrab  
Bug Tracker: https://github.com/lxqt/screengrab/issues  


### Copyright

(c) 2014-2018, LXQt Team  
(c) 2009-2013, Artem 'DOOMer' Galichkin


### Translation (Weblate)

<a href="https://weblate.lxqt.org/projects/lxqt/screengrab/">
<img src="https://weblate.lxqt.org/widgets/lxqt/-/screengrab/multi-auto.svg" alt="Translation status" />
</a>
