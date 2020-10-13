#.rst:
# FindXdgEmail
# ------------
#
# Try to find the xdg-email executable
#
# This will define the following variables:
#
# ``XdgEmail_FOUND``
#     True if (the requested version of) XdgEmail is available
#
# ``XdgEmail_VERSION``
#     The version of XdgEmail
#
# ``XdgEmail_EXECUTABLE``
#     The xdg-email executable
#
# If ``XdgEmail_FOUND`` is TRUE, it will also define the following imported
# target:
#
# ``XdgEmail::XdgEmail``
#     The xdg-email executable
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

set(_xdgemail_REQUIRED_VARS)
set(_xdgemail_deps XdgMime XdgOpen)

set(_xdgemail_QUIET)
if (XdgEmail_FIND_QUIETLY)
    set(_xdgemail_QUIET QUIET)
endif()

set(_xdgemail_REQUIRED)
if (XdgEmail_FIND_REQUIRED)
        set(_xdgemail_REQUIRED REQUIRED)
endif()

find_program(XdgEmail_EXECUTABLE NAMES xdg-email)

if (XdgEmail_EXECUTABLE)
    execute_process(COMMAND ${XdgEmail_EXECUTABLE} --version
        OUTPUT_VARIABLE _xdgemail_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(REGEX REPLACE "xdg-email ([0-9]\\.[0-9]\\.[0-9]+).*"
    "\\1" XdgEmail_VERSION_STRING "${_xdgemail_version}")

if (XdgEmail_FIND_REQUIRED)
    list(APPEND _xdgemail_REQUIRED_VARS ${XdgEmail_EXECUTABLE})
endif()

foreach(module ${_xdgemail_deps})
    find_package(${module} ${XdgEmail_FIND_VERSION} ${_xdgemail_QUIET} ${_xdgemail_REQUIRED})
    if (XdgEmail_FIND_REQUIRED)
        list(APPEND _xdgemail_REQUIRED_VARS ${${module}_EXECUTABLE})
    endif()
endforeach()

find_package_handle_standard_args(XdgEmail
    REQUIRED_VARS _xdgemail_REQUIRED_VARS
    VERSION_VAR XdgEmail_VERSION_STRING
)

if (XdgEmail_FOUND AND NOT TARGET XdgEmail::XdgEmail)
    add_executable(XdgEmail::XdgEmail IMPORTED)
    set_target_properties(XdgEmail::XdgEmail PROPERTIES
        IMPORTED_LOCATION "${XdgEmail_EXECUTABLE}"
    )
endif()

mark_as_advanced(XdgEmail_EXECUTABLE)
