#!/bin/sh
LAST_LXQT_VER="0.8.0"
CURRENT_LXQT_VER="HEAD"

LAST_QTXDG_VER="0.5.3"
CURRENT_QTXDG_VER="HEAD"

LAST_SYSSTAT_VER="0.1.0"
CURRENT_SYSSTAT_VER="HEAD"

LAST_LXIMAGE_QT_VER="0.2.0"
CURRENT_LXIMAGE_QT_VER="HEAD"

LAST_OBCONF_QT_VER="0.1.0"
CURRENT_OBCONF_QT_VER="HEAD"

LAST_COMPTON_CONF_VER="0.1.0"
CURRENT_COMPTON_QT_VER="HEAD"

# change log for main lxqt components
COMPONENTS="
	liblxqt \
	liblxqt-mount \
	lxqt-session \
	lxqt-qtplugin \
	lxqt-globalkeys \
	lxqt-notificationd \
	lxqt-about \
	lxqt-common \
	lxqt-config \
	lxqt-openssh-askpass \
	lxqt-panel \
	lxqt-policykit \
	lxqt-powermanagement \
	lxqt-runner"

for component in $COMPONENTS
do
	echo ""; echo "Changes of $component:";
	cd "$component" > /dev/null
	git log --oneline "$LAST_LXQT_VER".."$CURRENT_LXQT_VER"
	cd - > /dev/null
done


# change log for newly-added lxqt components
COMPONENTS="
	lxqt-admin"

for component in $COMPONENTS
do
	echo ""; echo "Changes of $component:";
	cd "$component" > /dev/null
	git log --oneline .."$CURRENT_LXQT_VER"
	cd - > /dev/null
done


# change log for qtxdg
echo
echo Changes of libqtxdg
cd libqtxdg > /dev/null
git log --oneline "${LAST_QTXDG_VER}".."${CURRENT_QTXDG_VER}"
cd - > /dev/null

# change log for sysstat
echo
echo Changes of libsysstat
cd libsysstat > /dev/null
git log --oneline "${LAST_SYSSTAT_VER}".."${CURRENT_SYSSTAT_VER}"
cd - > /dev/null

# change log for lximage-qt
echo
echo Changes of lximage-qt
cd lximage-qt > /dev/null
git log --oneline "${LAST_LXIMAGE_QT_VER}".."${CURRENT_LXIMAGE_QT_VER}"
cd - > /dev/null

# change log for obconf-qt
echo
echo Changes of obconf-qt
cd obconf-qt > /dev/null
git log --oneline "${LAST_OBCONF_QT_VER}".."${CURRENT_OBCONF_QT_VER}"
cd - > /dev/null

# change log for compton-cont
echo
echo Changes of compton-conf
cd compton-conf > /dev/null
git log --oneline "${LAST_COMPTON_CONF_VER}".."${CURRENT_COMPTON_CONF_VER}"
cd - > /dev/null
