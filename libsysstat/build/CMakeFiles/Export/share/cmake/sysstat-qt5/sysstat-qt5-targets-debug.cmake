#----------------------------------------------------------------
# Generated CMake target import file for configuration "debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sysstat-qt5" for configuration "debug"
set_property(TARGET sysstat-qt5 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(sysstat-qt5 PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libsysstat-qt5.so.0.4.3"
  IMPORTED_SONAME_DEBUG "libsysstat-qt5.so.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS sysstat-qt5 )
list(APPEND _IMPORT_CHECK_FILES_FOR_sysstat-qt5 "${_IMPORT_PREFIX}/lib/libsysstat-qt5.so.0.4.3" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
