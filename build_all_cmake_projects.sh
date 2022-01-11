#!/bin/sh

# various options for cmake based builds:
# CMAKE_BUILD_TYPE can specify a build (debug|release|...) build type
# LIB_SUFFIX can set the ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}
#     useful for 64 bit distros
# LXQT_PREFIX changes default /usr/local prefix
# LXQT_JOB_NUM Number of jobs to run in parallel while building. Defauts to
#   whatever nproc returns.
# DO_BUILD flag if components should be built (default 1)
# DO_INSTALL flag if components should be installed (default 1)
# DO_INSTALL_ROOT flag if rights should be elevated during install (default 1)
# UPDATE flag to use the latest of lxqt
#
# example:
# $ LIB_SUFFIX=64 ./build_all.sh
# or
# $ CMAKE_BUILD_TYPE=debug CMAKE_GENERATOR=Ninja CC=clang CXX=clang++ DO_INSTALL=0 ./build_all_cmake_projects.sh
# etc.

. ./cmake_repos.list

if [ -n "$UPDATE" ]; then
    echo "Using updated master branches for building"
    git pull
    git submodule update --init --recursive
    git submodule foreach git checkout master
fi

if [ -n "$LXQT_JOB_NUM" ]; then
    JOB_NUM="$LXQT_JOB_NUM"
elif which nproc > /dev/null; then
    # detect processor numbers (Linux only)
    JOB_NUM=`nproc`
else
    # assume default job number of 1 (non-Linux systems)
    JOB_NUM=1
fi
echo "Make job number: $JOB_NUM"

if [ -n "$CMAKE_BUILD_TYPE" ]; then
	CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE"
else
	CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=debug"
fi

if [ -n "$LXQT_PREFIX" ]; then
	CMAKE_INSTALL_PREFIX="-DCMAKE_INSTALL_PREFIX=$LXQT_PREFIX"
fi

if [ -n  "$CMAKE_GENERATOR" ]; then
	#echo "x$CMAKE_GENERATOR"
	if [ "$CMAKE_GENERATOR" = "Ninja" ]; then
		CMAKE_MAKE_PROGRAM="ninja"
		CMAKE_GENERATOR="-G $CMAKE_GENERATOR -DCMAKE_MAKE_PROGRAM=$CMAKE_MAKE_PROGRAM"
	fi
fi

[ -n "$CMAKE_MAKE_PROGRAM" ] || CMAKE_MAKE_PROGRAM="make"

if [ -n "$LIB_SUFFIX" ]; then
	CMAKE_LIB_SUFFIX="-DLIB_SUFFIX=$LIB_SUFFIX"
fi

[ -n "$DO_BUILD" ] || DO_BUILD=1

[ -n "$DO_INSTALL" ] || DO_INSTALL=1

[ -n "$DO_INSTALL_ROOT" ] || DO_INSTALL_ROOT=1


ALL_CMAKE_FLAGS="$CMAKE_BUILD_TYPE $CMAKE_INSTALL_PREFIX $CMAKE_LIB_SUFFIX $CMAKE_GENERATOR"

for d in $CMAKE_REPOS $OPTIONAL_CMAKE_REPOS
do
	echo "\n\nBuilding: $d using externally specified options: $ALL_CMAKE_FLAGS\n"
	mkdir -p "$d/build" \
		&& cd "$d/build" \
		|| exit 1
	if [ "$DO_BUILD" = 1 ]; then
		cmake $ALL_CMAKE_FLAGS .. || exit 1 && "$CMAKE_MAKE_PROGRAM" -j$JOB_NUM || exit 1
	fi
	if [ "$DO_INSTALL" = 1 ]; then
		if [ "$DO_INSTALL_ROOT" = 1 ]; then
			sudo "$CMAKE_MAKE_PROGRAM" install || exit 1
		else
			"$CMAKE_MAKE_PROGRAM" install || exit 1
		fi
	fi
	cd ../..
done
