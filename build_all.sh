#!/bin/sh

AUTOMAKE_REPOS=" \
	libfm \
	menu-cache \
	lxsession"

for d in $AUTOMAKE_REPOS;
do
	cd "$d"
	./autogen.sh && ./configure && make && sudo make install
	cd ..
done

CMAKE_REPOS=" \
	libqtxdg \
	liblxqt \
	liblxqt-mount \
	libsysstat \
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
	lxqt-power \
	lxqt-powermanagement \
	lxqt-runner \
	lxrandr-qt \
	obconf-qt \
	pcmanfm-qt"

for d in $CMAKE_REPOS;
do
	mkdir -p "$d/build"
	cd "$d/build"
	cmake .. && make && sudo make install
	cd ../..
done
