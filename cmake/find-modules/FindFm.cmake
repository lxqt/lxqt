# Distributed under the OSI-approved BSD 3-Clause License. See accompanying
# BSD-3-Clause file for details.

#.rst:
# FindFm
# -----------
#
# Try to find the Fm library
#
# Once done this will define
#
# ::
#
#   FM_FOUND - System has the Fm library
#   FM_INCLUDE_DIR - The Fm library include directory
#   FM_INCLUDE_DIRS - Location of the headers needed to use the Fm library
#   FM_LIBRARIES - The libraries needed to use the Fm library
#   FM_DEFINITIONS - Compiler switches required for using the Fm library
#   FM_VERSION_STRING - the version of the Fm library found


# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
pkg_check_modules(PC_FM QUIET libfm)
set(FM_DEFINITIONS ${PC_FM_CFLAGS_OTHER})

find_path(FM_INCLUDE_DIR NAMES libfm/fm.h
   HINTS
   ${PC_FM_INCLUDEDIR}
   ${PC_FM_INCLUDE_DIRS}
   PATH_SUFFIXES libfm
)

find_library(FM_LIBRARIES NAMES fm libfm
   HINTS
   ${PC_FM_LIBDIR}
   ${PC_FM_LIBRARY_DIRS}
)


# iterate over all dependencies
unset(FD_LIBRARIES)
foreach(depend ${PC_FM_LIBRARIES})
    find_library(_DEPEND_LIBRARIES
        NAMES
            ${depend}
        HINTS
            ${PC_FM_LIBDIR}
            ${PC_FM_LIBRARY_DIRS}
    )

    if (_DEPEND_LIBRARIES)
        list(APPEND FD_LIBRARIES ${_DEPEND_LIBRARIES})
    endif()
    unset(_DEPEND_LIBRARIES CACHE)
endforeach()

set(FM_VERSION_STRING ${PC_FM_VERSION})
set(FM_INCLUDE_DIR ${PC_FM_INCLUDEDIR})

list(APPEND FM_INCLUDE_DIRS
    ${FM_INCLUDE_DIR}
    ${PC_FM_INCLUDE_DIRS}
)
list(REMOVE_DUPLICATES FM_INCLUDE_DIRS)

list(APPEND FM_LIBRARIES
    ${FD_LIBRARIES}
)

list(REMOVE_DUPLICATES FM_LIBRARIES)
# handle the QUIETLY and REQUIRED arguments and set FM_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fm
                                  REQUIRED_VARS FM_LIBRARIES FM_INCLUDE_DIR FM_INCLUDE_DIRS
                                  VERSION_VAR FM_VERSION_STRING)

mark_as_advanced(FM_INCLUDE_DIR FM_LIBRARIES)
