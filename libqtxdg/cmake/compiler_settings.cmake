#=============================================================================
# Copyright 2015 Luís Pereira <luis.artur.pereira@gmail.com>
# Copyright 2013 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#-----------------------------------------------------------------------------
# Honor visibility properties for all target types.
#
# The ``<LANG>_VISIBILITY_PRESET`` and
# ``VISIBILITY_INLINES_HIDDEN`` target properties affect visibility
# of symbols during dynamic linking.  When first introduced these properties
# affected compilation of sources only in shared libraries, module libraries,
# and executables with the ``ENABLE_EXPORTS`` property set.  This
# was sufficient for the basic use cases of shared libraries and executables
# with plugins.  However, some sources may be compiled as part of static
# libraries or object libraries and then linked into a shared library later.
# CMake 3.3 and above prefer to honor these properties for sources compiled
# in all target types.  This policy preserves compatibility for projects
# expecting the properties to work only for some target types.
#
# The ``OLD`` behavior for this policy is to ignore the visibility properties
# for static libraries, object libraries, and executables without exports.
# The ``NEW`` behavior for this policy is to honor the visibility properties
# for all target types.
#
# This policy was introduced in CMake version 3.3.  CMake version
# 3.3.0 warns when the policy is not set and uses ``OLD`` behavior. Use
# the ``cmake_policy()`` command to set it to ``OLD`` or ``NEW``
# explicitly.
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Turn on more aggrassive optimizations not supported by CMake
# References: https://wiki.qt.io/Performance_Tip_Startup_Time
#-----------------------------------------------------------------------------
if (CMAKE_COMPILER_IS_GNUCXX OR LXQT_COMPILER_IS_CLANGCXX)
    # -flto: use link-time optimizations to generate more efficient code
    if (CMAKE_COMPILER_IS_GNUCXX)
        set(LTO_FLAGS "-flto -fuse-linker-plugin")
        # When building static libraries with LTO in gcc >= 4.9,
        # "gcc-ar" and "gcc-ranlib" should be used instead of "ar" and "ranlib".
        # references:
        #   https://gcc.gnu.org/gcc-4.9/changes.html
        #   http://hubicka.blogspot.tw/2014/04/linktime-optimization-in-gcc-2-firefox.html
        #   https://github.com/monero-project/monero/pull/1065/commits/1855213c8fb8f8727f4107716aab8e7ba826462b
        if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")  # gcc >= 4.9
            set(CMAKE_AR "gcc-ar")
            set(CMAKE_RANLIB "gcc-ranlib")
        endif()
    elseif (LXQT_COMPILER_IS_CLANGCXX)
        # The link-time optimization of clang++/llvm seems to be too aggrassive.
        # After testing, it breaks the signal/slots of QObject sometimes.
        # So disable it for now until there is a solution.
        # set(LTO_FLAGS "-flto")
    endif()
    # apply these options to "Release" build type only
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${LTO_FLAGS}")
endif()
