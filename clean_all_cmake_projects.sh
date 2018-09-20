#!/bin/bash

source "cmake_repos.list"

for d in ${CMAKE_REPOS} ${OPTIONAL_CMAKE_REPOS}
do
	echo "removing $d/build"
	rm -rf $d/build
done
