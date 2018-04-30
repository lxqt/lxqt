#=============================================================================
# Copyright (c) 2018 Luís Pereira <luis.artur.pereira@gmail.com>
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

function(lxqt_prevent_in_source_builds)

    # Handle smarties with symlinks
    get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
    get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

    if("${srcdir}" STREQUAL "${bindir}")
        # We are in a in source build
        message("##############################################################")
        message("# In Source Build detected.")
        message("# LXQt does not support in source builds.")
        message("# Out of source build is required.")
        message("#")
        message("# The general approach to out of source builds is:")
        message("#       mkdir build")
        message("#       cd build")
        message("#       cmake <path to sources>")
        message("#       make")
        message("#")
        message("# An in source build was attemped. Some files were created.")
        message("# Use 'git status' to check them. Remove them with:")
        message("#       cd <path to sources>")
        message("#")
        message("#       # Don’t actually remove anything, just show what would be done")
        message("#       git clean -n -d")
        message("#")
        message("#       # Actually remove the files")
        message("#       git clean -f -d")
        message("#")
        message("#       checkout files out of the index")
        message("#       git checkout --")
        message("##############################################################")

        message(FATAL_ERROR "Aborting configuration")
    endif()
endfunction()

lxqt_prevent_in_source_builds()
