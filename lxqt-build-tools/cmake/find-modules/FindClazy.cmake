#.rst:
# FindClazy
# ------------
#
# Try to find the Clazy package
#
# This will define the following variables:
#
# ``Clazy_FOUND``
#     True if (the requested version of) Clazy is available
#
# ``Clazy_VERSION``
#     The version of Clazy
#
# ``Clazy_EXECUTABLE``
#     The clazy executable
#
# ``Clazy_STANDALONE_EXECUTABLE
#     The clazy executable
#
# ``Clazy_PLUGIN``
#     The clazy library
#
# If ``Clazy_FOUND`` is TRUE, it will also define the following imported
# targets:
#
# ``Clazy::Clazy``
#     The clazy executable
#
# ``Clazy::Standalone``
#     The clazy-standalone executable
#
# ``Clazy::Plugin``
#     The clazy plugin
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

set(_clazy_REQUIRED_VARS)

find_program(Clazy_EXECUTABLE NAMES clazy)

if (Clazy_EXECUTABLE)
    execute_process(COMMAND ${Clazy_EXECUTABLE} --version
        OUTPUT_VARIABLE _clazy_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

string(REGEX REPLACE "clazy version: ([0-9]\\.[0-9]+).*"
    "\\1" Clazy_VERSION_STRING "${_clazy_version}")

if (Clazy_VERSION_STRING VERSION_LESS 1.5)
    if (Clazy_FIND_REQUIRED)
        message(FATAL_ERROR "Clazy versions prior to 1.5 not supported. Found Clazy version: ${Clazy_VERSION_STRING}")
    else()
        message(WARNING "Clazy versions prior to 1.5 not supported. Found Clazy version: ${Clazy_VERSION_STRING}")
    endif()
endif()

if (Clazy_FIND_REQUIRED)
    list(APPEND _clazy_REQUIRED_VARS ${Clazy_EXECUTABLE})
endif()

find_program(Clazy_STANDALONE NAMES clazy-standalone)
if (Clazy_FIND_REQUIRED)
    list(APPEND _clazy_REQUIRED_VARS ${Clazy_STANDALONE})
endif()

find_library(Clazy_PLUGIN NAMES ClazyPlugin ClazyPlugin${CMAKE_SHARED_LIBRARY_SUFFIX})
if (Clazy_FIND_REQUIRED)
    list(APPEND _clazy_REQUIRED_VARS ${Clazy_PLUGIN})
endif()

find_package_handle_standard_args(Clazy
    REQUIRED_VARS _clazy_REQUIRED_VARS
    VERSION_VAR Clazy_VERSION_STRING
)

if (Clazy_FOUND)
    if (NOT TARGET Clazy::Clazy)
        add_executable(Clazy::Clazy IMPORTED)
        set_target_properties(Clazy::Clazy PROPERTIES
            IMPORTED_LOCATION "${Clazy_EXECUTABLE}"
        )
    endif()

    if (NOT TARGET Clazy::Standalone)
        add_executable(Clazy::Standalone IMPORTED)
        set_target_properties(Clazy::Standalone PROPERTIES
            IMPORTED_LOCATION "${Clazy_STANDALONE}"
        )
    endif()

    if (NOT TARGET Clazy::Plugin)
        add_library(Clazy::Plugin SHARED IMPORTED)
        set_target_properties(Clazy::Plugin PROPERTIES
            IMPORTED_LOCATION "${Clazy_PLUGIN}"
        )
    endif()
endif()
