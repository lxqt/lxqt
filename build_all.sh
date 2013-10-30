#!/bin/sh
#
# various options for cmake based builds:
# CMAKE_BUILD_TYPE can specify a build (debug|release|...) build type
# LIB_SUFFIX can set the ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}
#     useful fro 64 bit distros
# PREFIX changes default /usr/local prefix
#
# example:
# $ LIB_SUFFIX=64 ./build_all.sh
# or
# $ CMAKE_BUILD_TYPE=debug CMAKE_GENERATOR=Ninja CC=clang CXX=clang++ ./build_all.sh
# etc.

AUTOMAKE_REPOS=" \
	menu-cache \
	libfm \
	lxsession"

if env | grep -q ^LXQT_PREFIX= ; then
	PREF="--prefix=$LXQT_PREFIX"
else
	PREF=""
fi


for d in $AUTOMAKE_REPOS
do
	echo ""; echo ""; echo "building: $d into $PREF"; echo ""
	cd "$d"
	./autogen.sh && ./configure $PREF && make -j2 && sudo make install
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

if env | grep -q ^CMAKE_BUILD_TYPE= ; then
	CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
else
	CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=debug"
fi

if env | grep -q ^LXQT_PREFIX= ; then
	CMAKE_INSTALL_PREFIX="-DCMAKE_INSTALL_PREFIX=$LXQT_PREFIX"
else
	CMAKE_INSTALL_PREFIX=""
fi

if env | grep -q ^CMAKE_GENERATOR= ; then
	echo "x$CMAKE_GENERATOR"
	if [ "x$CMAKE_GENERATOR" = "xNinja" ] ; then
		CMAKE_MAKE_PROGRAM="ninja"
		CMAKE_GENERATOR="-G $CMAKE_GENERATOR -DCMAKE_MAKE_PROGRAM=$CMAKE_MAKE_PROGRAM"
	fi
fi

if [ "x$CMAKE_MAKE_PROGRAM" = "x" ] ; then
	CMAKE_MAKE_PROGRAM="make"
fi

if env | grep -q ^LIB_SUFFIX= ; then
	CMAKE_LIB_SUFFIX="-DLIB_SUFFIX=$LIB_SUFFIX"
else
	CMAKE_LIB_SUFFIX=""
fi


ALL_CMAKE_FLAGS="$CMAKE_BUILD_TYPE $CMAKE_INSTALL_PREFIX $CMAKE_LIB_SUFFIX $CMAKE_GENERATOR"

for d in $CMAKE_REPOS
do
	echo ""; echo ""; echo "building: $d using externally specified options: $ALL_CMAKE_FLAGS"; echo ""
	mkdir -p $d/build
	cd $d/build
	cmake $ALL_CMAKE_FLAGS .. && $CMAKE_MAKE_PROGRAM -j2 && sudo $CMAKE_MAKE_PROGRAM install
	cd ../..
done
