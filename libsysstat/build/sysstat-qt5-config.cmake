# - Find the sysstat include and library dirs and define a some macros
#
# The module defines the following variables
#
#  SYSSTAT_FOUND         - Set to TRUE if all of the above has been found



####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was sysstat-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set(SYSSTAT_MAJOR_VERSION 0)
set(SYSSTAT_MINOR_VERSION 4)
set(SYSSTAT_PATCH_VERSION 3)
set(SYSSTAT_VERSION       0.4.3)

include(CMakeFindDependencyMacro)

find_dependency(Qt5Core 5.10.0)
if (NOT TARGET sysstat-qt5)
    if (POLICY CMP0024)
        cmake_policy(SET CMP0024 NEW)
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/sysstat-qt5-targets.cmake")
endif()
