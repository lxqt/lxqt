# - Try to find the UDev library
# Once done this will define
#
#  UDEV_FOUND - system has UDev
#  UDEV_INCLUDE_DIR - the libudev include directory
#  UDEV_LIBS - The libudev libraries

# Copyright (c) 2010, Rafael Fernández López, <ereslibre@kde.org>
# Copyright (c) 2016, Luís Pereira, <luis.artur.pereira@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

find_package(PkgConfig)
pkg_check_modules(PC_UDEV libudev)

find_path(UDEV_INCLUDE_DIR libudev.h
    HINTS ${PC_UDEV_INCLUDEDIR} ${PC_UDEV_INCLUDE_DIRS})

find_library(UDEV_LIBS udev HINTS ${PC_UDEV_LIBDIR} ${PC_UDEV_LIBRARY_DIRS})

if(UDEV_INCLUDE_DIR AND UDEV_LIBS)
   include(CheckFunctionExists)
   include(CMakePushCheckState)
   cmake_push_check_state()
   set(CMAKE_REQUIRED_LIBRARIES ${UDEV_LIBS} )

   cmake_pop_check_state()

endif()

set(UDEV_VERSION_STRING ${PC_UDEV_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDev
    REQUIRED_VARS UDEV_INCLUDE_DIR UDEV_LIBS
    VERSION_VAR ${UDEV_VERSION_STRING})

mark_as_advanced(UDEV_INCLUDE_DIR UDEV_LIBS)

include(FeatureSummary)
set_package_properties(UDev PROPERTIES
   URL "https://www.kernel.org/pub/linux/utils/kernel/hotplug/udev/udev.html"
   DESCRIPTION "Linux dynamic device management")
