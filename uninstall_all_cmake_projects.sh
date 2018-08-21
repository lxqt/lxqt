#!/bin/sh

INSTALL_MANIFEST="install_manifest.txt"

source "cmake_repos.list"

for d in ${CMAKE_REPOS} ${OPTIONAL_CMAKE_REPOS}
do
    INSTALL_MANIFEST_FULL_PATH="${d}/build/${INSTALL_MANIFEST}"
    if [ -f "${INSTALL_MANIFEST_FULL_PATH}" ]
    then
        echo "Uninstalling component ${d} ,,,"
        xargs rm -f < "${INSTALL_MANIFEST_FULL_PATH}" || exit $?
    else
        echo "${d}: ${INSTALL_MANIFEST} not found, component probably wasn't installed" >&2
    fi
done
