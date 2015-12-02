#!/bin/sh

INSTALL_MANIFEST="install_manifest.txt"

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

for d in ${CMAKE_REPOS}
do
    INSTALL_MANIFEST_FULL_PATH="${d}/build/${INSTALL_MANIFEST}"
    if [ -f "${INSTALL_MANIFEST_FULL_PATH}" ]
    then
        xargs rm < "${INSTALL_MANIFEST_FULL_PATH}"
    else
        echo "${d}: ${INSTALL_MANIFEST} not found"
    fi
done
