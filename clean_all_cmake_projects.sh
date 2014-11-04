#!/bin/sh
CMAKE_REPOS=" \
	libqtxdg \
	liblxqt \
	liblxqt-mount \
	libsysstat \
	lxqt-qtplugin \
	lxqt-globalkeys \
	lxqt-notificationd \
	lximage-qt \
	lxinput-qt \
	lxqt-about \
	lxqt-common \
	lxqt-config \
	lxqt-openssh-askpass \
	lxqt-panel \
	lxqt-policykit \
	lxqt-powermanagement \
	lxqt-runner \
	obconf-qt \
	pcmanfm-qt"

for d in $CMAKE_REPOS
do
	echo "removing $d/build"
	rm -rf $d/build
done
