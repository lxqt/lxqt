#=============================================================================
# Copyright 2014 Luís Pereira <luis.artur.pereira@gmail.com>
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
#
# These functions enables "automatic" translation loading in LXQt Qt5 apps
#   and libs. They generate a .cpp file that takes care of everything. The
#   user doesn't have to do anything in the source code.
#
# Typical use:
#   include(LXQtTranslationLoader)
#   lxqt_app_translation_loader(lxqt-app_QM_LOADER ${PROJECT_NAME})
#   add_executable(${PROJECT_NAME}
#       ${lxqt-app_QM_LOADER}
#       ...
#   )


# lxqt_app_translation_loader(<source_files> <catalog_name>)
#       <source_files> The generated .cpp file is added to <source_files>
#       <catalog_name> Translations catalog to be loaded
function(lxqt_app_translation_loader source_files catalog_name)
    configure_file(
        ${LXQT_CMAKE_MODULES_DIR}/LXQtAppTranslationLoader.cpp.in
        LXQtAppTranslationLoader.cpp @ONLY
    )
    set(${source_files} ${${source_files}} ${CMAKE_CURRENT_BINARY_DIR}/LXQtAppTranslationLoader.cpp PARENT_SCOPE)
endfunction()

# lxqt_lib_translation_loader(<source_files> <catalog_name>)
#       <source_files> The generated .cpp file is added to <source_files>
#       <catalog_name> Translations catalog to be loaded
function(lxqt_lib_translation_loader source_files catalog_name)
    configure_file(
        ${LXQT_CMAKE_MODULES_DIR}/LXQtLibTranslationLoader.cpp.in
        LXQtLibTranslationLoader.cpp @ONLY
    )
    set(${source_files} ${${source_files}} ${CMAKE_CURRENT_BINARY_DIR}/LXQtLibTranslationLoader.cpp PARENT_SCOPE)
endfunction()
