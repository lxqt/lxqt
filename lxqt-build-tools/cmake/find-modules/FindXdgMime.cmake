#.rst:
# FindXdgMime
# ------------
#
# Try to find the xdg-mime executable
#
# This will define the following variables:
#
# ``XdgMime_FOUND``
#     True if (the requested version of) XdgMime is available
#
# ``XdgMime_VERSION``
#     The version of XdgMime
#
# ``XdgMime_EXECUTABLE``
#     The xdg-mime executable
#
# If ``XdgMime_FOUND`` is TRUE, it will also define the following imported
# target:
#
# ``XdgMime::XdgMime``
#     The xdg-mime executable
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

set(_xdgmime_REQUIRED_VARS)

find_program(XdgMime_EXECUTABLE NAMES xdg-mime)

if (XdgMime_EXECUTABLE)
    execute_process(COMMAND ${XdgMime_EXECUTABLE} --version
        OUTPUT_VARIABLE _xdgmime_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(REGEX REPLACE "xdg-mime ([0-9]\\.[0-9]\\.[0-9]+).*"
    "\\1" XdgMime_VERSION_STRING "${_xdgmime_version}")

if (XdgMime_FIND_REQUIRED)
    list(APPEND _xdgmime_REQUIRED_VARS ${XdgMime_EXECUTABLE})
endif()

find_package_handle_standard_args(XdgMime
    REQUIRED_VARS _xdgmime_REQUIRED_VARS
    VERSION_VAR XdgMime_VERSION_STRING
)

if (XdgMime_FOUND AND NOT TARGET XdgMime::XdgMime)
    add_executable(XdgMime::XdgMime IMPORTED)
    set_target_properties(XdgMime::XdgMime PROPERTIES
        IMPORTED_LOCATION "${XdgMime_EXECUTABLE}"
    )
endif()

mark_as_advanced(XdgMime_EXECUTABLE)
