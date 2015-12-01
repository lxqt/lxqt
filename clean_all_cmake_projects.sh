#!/bin/sh
CMAKE_REPOS=" \
	libqtxdg \
	liblxqt \
	libsysstat \
	lxqt-session \
	lxqt-qtplugin \
	lxqt-globalkeys \
	lxqt-notificationd \
	lxqt-about \
	lxqt-common \
	lxqt-config \
	lxqt-admin \
	lxqt-openssh-askpass \
	lxqt-panel \
	lxqt-policykit \
	lxqt-powermanagement \
	lxqt-runner \
	libfm-qt \
	pcmanfm-qt \
	lximage-qt \
	lxqt-sudo \
	compton-conf \
	obconf-qt"

for d in $CMAKE_REPOS
do
	echo "removing $d/build"
	rm -rf $d/build
done
