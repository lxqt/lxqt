# Distributed under the OSI-approved BSD 3-Clause License. See accompanying
# BSD-3-Clause file for details.

#.rst:
# FindExif
# -----------
#
# Try to find the Exif library
#
# Once done this will define
#
# ::
#
#   EXIF_FOUND - System has the Exif library
#   EXIF_INCLUDE_DIR - The Exif library include directory
#   EXIF_INCLUDE_DIRS - Location of the headers needed to use the Exif library
#   EXIF_LIBRARIES - The libraries needed to use the Exif library
#   EXIF_DEFINITIONS - Compiler switches required for using the Exif library
#   EXIF_VERSION_STRING - the version of the Exif library found


# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig)
pkg_check_modules(PC_EXIF libexif)
set(EXIF_DEFINITIONS ${PC_EXIF_CFLAGS_OTHER})

find_path(EXIF_INCLUDE_DIR NAMES libexif/exif-data.h
   HINTS
   ${PC_EXIF_INCLUDEDIR}
   ${PC_EXIF_INCLUDE_DIRS}
   PATH_SUFFIXES libexif
)

find_library(EXIF_LIBRARIES NAMES exif libexif
   HINTS
   ${PC_EXIF_LIBDIR}
   ${PC_EXIF_LIBRARY_DIRS}
)


# iterate over all dependencies
unset(FD_LIBRARIES)
foreach(depend ${PC_EXIF_LIBRARIES})
    find_library(_DEPEND_LIBRARIES
        NAMES
            ${depend}
        HINTS
            ${PC_EXIF_LIBDIR}
            ${PC_EXIF_LIBRARY_DIRS}
    )

    if (_DEPEND_LIBRARIES)
        list(APPEND FD_LIBRARIES ${_DEPEND_LIBRARIES})
    endif()
    unset(_DEPEND_LIBRARIES CACHE)
endforeach()

set(EXIF_VERSION_STRING ${PC_EXIF_VERSION})
set(EXIF_INCLUDE_DIR ${PC_EXIF_INCLUDEDIR})

list(APPEND EXIF_INCLUDE_DIRS
    ${EXIF_INCLUDE_DIR}
    ${PC_EXIF_INCLUDE_DIRS}
)
list(REMOVE_DUPLICATES EXIF_INCLUDE_DIRS)

list(APPEND EXIF_LIBRARIES
    ${FD_LIBRARIES}
)

list(REMOVE_DUPLICATES EXIF_LIBRARIES)
# handle the QUIETLY and REQUIRED arguments and set EXIF_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Exif
                                  REQUIRED_VARS EXIF_LIBRARIES EXIF_INCLUDE_DIR EXIF_INCLUDE_DIRS
                                  VERSION_VAR EXIF_VERSION_STRING)

mark_as_advanced(EXIF_INCLUDE_DIR EXIF_LIBRARIES)
