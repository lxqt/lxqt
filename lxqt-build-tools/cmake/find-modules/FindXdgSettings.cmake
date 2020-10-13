#.rst:
# FindXdgSettings
# ------------
#
# Try to find the xdg-settings executable
#
# This will define the following variables:
#
# ``XdgSettings_FOUND``
#     True if (the requested version of) XdgSettings is available
#
# ``XdgSettings_VERSION``
#     The version of XdgSettings
#
# ``XdgSettings_EXECUTABLE``
#     The xdg-settings executable
#
# If ``XdgSettings_FOUND`` is TRUE, it will also define the following imported
# target:
#
# ``XdgSettings::XdgSettings``
#     The xdg-settings executable
#
#
#=============================================================================
# Copyright (c) 2019, Luís Pereira <luis.artur.pereira@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

include(FindPackageHandleStandardArgs)

set(_xdgsettings_REQUIRED_VARS)
set(_xdgsettings_deps XdgMime)

set(_xdgsettings_QUIET)
if (XdgSettings_FIND_QUIETLY)
    set(_xdgsettings_QUIET QUIET)
endif()

set(_xdgsettings_REQUIRED)
if (XdgSettings_FIND_REQUIRED)
        set(_xdgsettings_REQUIRED REQUIRED)
endif()

find_program(XdgSettings_EXECUTABLE NAMES xdg-settings)

if (XdgSettings_EXECUTABLE)
    execute_process(COMMAND ${XdgSettings_EXECUTABLE} --version
        OUTPUT_VARIABLE _xdgsettings_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(REGEX REPLACE "xdg-settings ([0-9]\\.[0-9]\\.[0-9]+).*"
    "\\1" XdgSettings_VERSION_STRING "${_xdgsettings_version}")

if (XdgSettings_FIND_REQUIRED)
    list(APPEND _xdgsettings_REQUIRED_VARS ${XdgSettings_EXECUTABLE})
endif()

foreach(module ${_xdgsettings_deps})
    find_package(${module} ${XdgSettings_FIND_VERSION} ${_xdgsettings_QUIET} ${_xdgsettings_REQUIRED})
    if (XdgSettings_FIND_REQUIRED)
        list(APPEND _xdgsettings_REQUIRED_VARS ${${module}_EXECUTABLE})
    endif()
endforeach()

find_package_handle_standard_args(XdgSettings
    REQUIRED_VARS _xdgsettings_REQUIRED_VARS
    VERSION_VAR XdgSettings_VERSION_STRING
)

if (XdgSettings_FOUND AND NOT TARGET XdgSettings::XdgSettings)
    add_executable(XdgSettings::XdgSettings IMPORTED)
    set_target_properties(XdgSettings::XdgSettings PROPERTIES
        IMPORTED_LOCATION "${XdgSettings_EXECUTABLE}"
    )
endif()

mark_as_advanced(XdgSettings_EXECUTABLE)
