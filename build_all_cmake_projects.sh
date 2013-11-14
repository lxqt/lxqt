#!/bin/sh
CMAKE_REPOS=" \
	libqtxdg \
	liblxqt \
	liblxqt-mount \
	libsysstat \
	libxdsettings \
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
	echo ""; echo ""; echo "building: $d using externally specified options: $ALL_CMAKE_FLAGS"; echo ""
	mkdir -p $d/build
	pushd $d/build
	(cmake .. && make -j8) || exit 1
	popd
done
