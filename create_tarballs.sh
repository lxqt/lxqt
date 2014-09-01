#!/bin/sh
mkdir -p release

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

# cmake projects
for cmake_file in `find . -mindepth 2 -maxdepth 2 -name 'CMakeLists.txt'`;
do
  package_dir=`dirname $cmake_file`
  package_name=`basename $package_dir`
  build_dir="$package_dir/build"
  echo "Build tarball for $package_name"
  mkdir -p "$build_dir" && cd "$build_dir" # enter the package build dir
  cmake .. && make package_source # create tarball
  cd -
  # move tarballs to release dir
  mv "$build_dir/$package_name"*.tar.* ./release
  if [ "$?" != "0" ]; then
    echo "Error creating the tarball"
    exit
  fi
done

# automake projects
for automake_file in `find . -mindepth 2 -maxdepth 2 -name 'Makefile.am'`;
do
  package_dir=`dirname $automake_file`
  package_name=`basename $package_dir`
  echo "Build tarball for $package_name"
  cd "$package_dir"
  ./autogen.sh && make dist
  cd -
  mv "$package_dir/$package_name"*.tar.* ./release
  if [ "$?" != "0" ]; then
    echo "Error creating the tarball"
    exit
  fi
done
