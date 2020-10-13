# Distributed under the OSI-approved BSD 3-Clause License. See accompanying
# BSD-3-Clause file for details.

#.rst:
# FindMenuCache
# -----------
#
# Try to find the MenuCache library
#
# Once done this will define
#
# ::
#
#   MENUCACHE_FOUND - System has the MenuCache library
#   MENUCACHE_INCLUDE_DIR - The MenuCache library include directory
#   MENUCACHE_INCLUDE_DIRS - Location of the headers needed to use the MenuCache library
#   MENUCACHE_LIBRARIES - The libraries needed to the MenuCache library
#   MENUCACHE_DEFINITIONS - Compiler switches required for using the MenuCache library
#   MENUCACHE_VERSION_STRING - the version of MenuCache library found


# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig)
pkg_check_modules(PC_MENUCACHE libmenu-cache)
set(MENUCACHE_DEFINITIONS ${PC_MENUCACHE_CFLAGS_OTHER})

find_path(MENUCACHE_INCLUDE_DIRS
    NAMES
        menu-cache.h
        menu-cache/menu-cache.h
   HINTS
       ${PC_MENUCACHE_INCLUDEDIR}
       ${PC_MENUCACHE_INCLUDE_DIRS}
   PATH_SUFFIXES
        libmenu-cache
)

find_library(MENUCACHE_LIBRARIES
    NAMES
        menu-cache
        libmenu-cache
   HINTS
       ${PC_MENUCACHE_LIBDIR}
       ${PC_MENUCACHE_LIBRARY_DIRS}
)


# iterate over all dependencies
unset(FD_LIBRARIES)
foreach(depend ${PC_MENUCACHE_LIBRARIES})
    find_library(_DEPEND_LIBRARIES
        NAMES
            ${depend}
        HINTS
            ${PC_MENUCACHE_LIBDIR}
            ${PC_MENUCACHE_LIBRARY_DIRS}
    )

    if (_DEPEND_LIBRARIES)
        list(APPEND FD_LIBRARIES ${_DEPEND_LIBRARIES})
    endif()
    unset(_DEPEND_LIBRARIES CACHE)
endforeach()

set(MENUCACHE_VERSION_STRING ${PC_MENUCACHE_VERSION})
set(MENUCACHE_INCLUDE_DIR ${PC_MENUCACHE_INCLUDEDIR})

list(APPEND MENUCACHE_INCLUDE_DIRS
    ${MENUCACHE_INCLUDE_DIR}
    ${PC_MENUCACHE_INCLUDE_DIRS}
)
list(REMOVE_DUPLICATES MENUCACHE_INCLUDE_DIRS)

list(APPEND MENUCACHE_LIBRARIES
    ${FD_LIBRARIES}
)

list(REMOVE_DUPLICATES MENUCACHE_LIBRARIES)

# handle the QUIETLY and REQUIRED arguments and set MENUCACHE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MenuCache
                                  REQUIRED_VARS MENUCACHE_LIBRARIES MENUCACHE_INCLUDE_DIR MENUCACHE_INCLUDE_DIRS
                                  VERSION_VAR MENUCACHE_VERSION_STRING)

mark_as_advanced(MENUCACHE_INCLUDE_DIR MENUCACHE_LIBRARIES)
