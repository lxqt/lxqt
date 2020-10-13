#.rst:
# FindXdgOpen
# ------------
#
# Try to find the xdg-open executable
#
# This will define the following variables:
#
# ``XdgOpen_FOUND``
#     True if (the requested version of) XdgOpen is available
#
# ``XdgOpen_VERSION``
#     The version of XdgOpen
#
# ``XdgOpen_EXECUTABLE``
#     The xdg-open executable
#
# If ``XdgOpen_FOUND`` is TRUE, it will also define the following imported
# target:
#
# ``XdgOpen::XdgOpen``
#     The xdg-open executable
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

set(_xdgopen_REQUIRED_VARS)
set(_xdgopen_deps XdgMime)

set(_xdgopen_QUIET)
if (XdgOpen_FIND_QUIETLY)
    set(_xdgopen_QUIET QUIET)
endif()

set(_xdgopen_REQUIRED)
if (XdgOpen_FIND_REQUIRED)
        set(_xdgopen_REQUIRED REQUIRED)
endif()

find_program(XdgOpen_EXECUTABLE NAMES xdg-open)

if (XdgOpen_EXECUTABLE)
    execute_process(COMMAND ${XdgOpen_EXECUTABLE} --version
        OUTPUT_VARIABLE _xdgopen_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(REGEX REPLACE "xdg-open ([0-9]\\.[0-9]\\.[0-9]+).*"
    "\\1" XdgOpen_VERSION_STRING "${_xdgopen_version}")

if (XdgOpen_FIND_REQUIRED)
    list(APPEND _xdgopen_REQUIRED_VARS ${XdgOpen_EXECUTABLE})
endif()

foreach(module ${_xdgopen_deps})
    find_package(${module} ${XdgOpen_FIND_VERSION} ${_xdgopen_QUIET} ${_xdgopen_REQUIRED})
    if (XdgOpen_FIND_REQUIRED)
        list(APPEND _xdgopen_REQUIRED_VARS ${${module}_EXECUTABLE})
    endif()
endforeach()

find_package_handle_standard_args(XdgOpen
    REQUIRED_VARS _xdgopen_REQUIRED_VARS
    VERSION_VAR XdgOpen_VERSION_STRING
)

if (XdgOpen_FOUND AND NOT TARGET XdgOpen::XdgOpen)
    add_executable(XdgOpen::XdgOpen IMPORTED)
    set_target_properties(XdgOpen::XdgOpen PROPERTIES
        IMPORTED_LOCATION "${XdgOpen_EXECUTABLE}"
    )
endif()

mark_as_advanced(XdgOpen_EXECUTABLE)
