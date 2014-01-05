#!/bin/sh
CMAKE_REPOS=" \
	libqtxdg \
	liblxqt \
	liblxqt-mount \
	libsysstat \
	libxdsettings \
	lxqt-qtplugin \
	lxqt-globalkeys \
	lxqt-notificationd \
	lximage-qt \
	lxinput-qt \
	lxqt-about \
	lxqt-appswitcher \
	lxqt-common \
	lxqt-config \
	lxqt-openssh-askpass \
	lxqt-panel \
	lxqt-policykit \
	lxqt-power \
	lxqt-powermanagement \
	lxqt-runner \
	lxrandr-qt \
	obconf-qt \
	pcmanfm-qt"

for d in $CMAKE_REPOS
do
	echo "removing $d/build"
	rm -rf $d/build	
done
