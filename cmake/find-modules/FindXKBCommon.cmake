#.rst:
# FindXKBCommon
# -----------
#
# Try to find XKBCommon.
#
# This is a component-based find module, which makes use of the COMPONENTS
# and OPTIONAL_COMPONENTS arguments to find_module.  The following components
# are available::
#
#   XKBCommon  X11
#
# If no components are specified, this module will act as though all components
# were passed to OPTIONAL_COMPONENTS.
#
# This module will define the following variables, independently of the
# components searched for or found:
#
# ``XKBCommon_FOUND``
#     TRUE if (the requested version of) XKBCommon is available
# ``XKBCommon_VERSION``
#     Found XKBCommon version
# ``XKBCommon_TARGETS``
#     A list of all targets imported by this module (note that there may be more
#     than the components that were requested)
# ``XKBCommon_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the imported
#     targets
# ``XKBCommon_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the targets are
#     not used for linking
# ``XKBCommon_DEFINITIONS``
#     This should be passed to target_compile_options() if the targets are not
#     used for linking
#
# For each searched-for components, ``XKBCommon_<component>_FOUND`` will be set to
# TRUE if the corresponding XKBCommon library was found, and FALSE otherwise.  If
# ``XKBCommon_<component>_FOUND`` is TRUE, the imported target
# ``XKBCommon::<component>`` will be defined.

#=============================================================================
# Copyright 2017 Luís Pereira <luis.artur.pereira@gmail.com>
#
# Documentation adapted from the KF5 FindWayland module.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

include(ECMFindModuleHelpers)

ecm_find_package_version_check(XKBCommon)

set(XKBCommon_known_components
    XKBCommon
    X11)

unset(XKBCommon_XKBCommon_component_deps)
set(XKBCommon_XKBCommon_pkg_config "xkbcommon")
set(XKBCommon_XKBCommon_lib "xkbcommon")
set(XKBCommon_XKBCommon_header "xkbcommon/xkbcommon.h")

set(XKBCommon_X11_component_deps XKBCommon)
set(XKBCommon_X11_pkg_config "xkbcommon-x11")
set(XKBCommon_X11_lib "xkbcommon-x11")
set(XKBCommon_X11_header "xkbcommon/xkbcommon-x11.h")

ecm_find_package_parse_components(XKBCommon
    RESULT_VAR XKBCommon_components
    KNOWN_COMPONENTS ${XKBCommon_known_components}
)
ecm_find_package_handle_library_components(XKBCommon
    COMPONENTS ${XKBCommon_components}
)

find_package_handle_standard_args(XKBCommon
    FOUND_VAR XKBCommon_FOUND
    REQUIRED_VARS XKBCommon_LIBRARIES XKBCommon_INCLUDE_DIRS
    VERSION_VAR XKBCommon_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(XKBCommon PROPERTIES
    URL "https://xkbcommon.org"
    DESCRIPTION "A library to handle keyboard descriptions"
)
