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
# funtion lxqt_translate_ts(qmFiles
#                           [USE_QT5 [Yes | No]]
#                           [UPDATE_TRANSLATIONS [Yes | No]]
#                           SOURCES <sources>
#                           [UPDATE_OPTIONS] update_options
#                           [TEMPLATE] translation_template
#                           [TRANSLATION_DIR] translation_directory
#                           [INSTALL_DIR] install_directory
#                           [COMPONENT] component
#                           [PULL_TRANSLATIONS [Yes | No]]
#                           [CLEAN_TRANSLATIONS [Yes | No]]
#                           [REPO_SUBDIR] repository_subdirectory
#                           [TRANSLATIONS_REPO] remote_translation_repo
#                           [TRANSLATIONS_REFSPEC] translations_remote_branch
#                    )
#     Output:
#       qmFiles The generated compiled translations (.qm) files
#
#     Input:
#       USE_QT5 Optional flag to choose between Qt4 and Qt5. Defaults to Qt5
#
#       UPDATE_TRANSLATIONS Optional flag. Setting it to Yes, extracts and
#                           compiles the translations. Setting it No, only
#                           compiles them.
#
#       UPDATE_OPTIONS Optional options to lupdate when UPDATE_TRANSLATIONS
#                       is True.
#
#       TEMPLATE Optional translations files base name. Defaults to
#                ${PROJECT_NAME}. An .ts extensions is added.
#
#       TRANSLATION_DIR Optional path to the directory with the .ts files,
#                        relative to the CMakeList.txt. Defaults to
#                        "translations".
#
#       INSTALL_DIR Optional destination of the file compiled files (qmFiles).
#                    If not present no installation is performed
#
#       COMPONENT Optional install component. Only effective if INSTALL_DIR
#                   present. Defaults to "Runtime".
#
#       PULL_TRANSLATIONS Optional flag. If set, the translations are pulled
#           from external repository in cmake phase (not in build/make time)
#           into directory "${TRANSLATION_DIR}/${REPO_SUBDIR}".
#
#       CLEAN_TRANSLATIONS Optional flag. If set, the externally pulled
#                          translations are removed.
#
#       REPO_SUBDIR Optional path in the "translations repository" to directory
#           with translations. Only effective if PULL_TRANSLATIONS enabled.
#           Defaults to "${TEMPLATE}".
#
#       TRANSLATIONS_REPO External git repository with translations - only the ${TEMPLATE} directory
#           is pulled (using the "sparse checkout").
#           Optional (defaults to "https://github.com/lxde/lxqt-l10n.git").
#
#       TRANSLATIONS_REFSPEC Optional refspec of external repository. Used in git pull.
#           Defaults to "master".

# CMake v2.8.3 needed to use the CMakeParseArguments module
cmake_minimum_required(VERSION 2.8.3 FATAL_ERROR)

# We use our patched version to round a annoying bug.
include(Qt5PatchedLinguistToolsMacros)

option(PULL_TRANSLATIONS "Pull translations" Yes)
option(CLEAN_TRANSLATIONS "Clean translations" No)

function(lxqt_translate_ts qmFiles)
    set(oneValueArgs
        USE_QT5
        UPDATE_TRANSLATIONS
        TEMPLATE
        TRANSLATION_DIR
        INSTALL_DIR
        COMPONENT
        PULL_TRANSLATIONS
        CLEAN_TRANSLATIONS
        REPO_SUBDIR
        TRANSLATIONS_REPO
        TRANSLATIONS_REFSPEC
    )
    set(multiValueArgs SOURCES UPDATE_OPTIONS)
    cmake_parse_arguments(TR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED TR_UPDATE_TRANSLATIONS)
        set(TR_UPDATE_TRANSLATIONS "No")
    endif()

    if (NOT DEFINED TR_UPDATE_OPTIONS)
        set(TR_UPDATE_OPTIONS "")
    endif()

    if (NOT DEFINED TR_USE_QT5)
        set(TR_USE_QT5 "Yes")
    endif()

    if(NOT DEFINED TR_TEMPLATE)
        set(TR_TEMPLATE "${PROJECT_NAME}")
    endif()

    if (NOT DEFINED TR_TRANSLATION_DIR)
        set(TR_TRANSLATION_DIR "translations")
    endif()
    get_filename_component(TR_TRANSLATION_DIR "${TR_TRANSLATION_DIR}" ABSOLUTE)

    if (NOT DEFINED TR_CLEAN_TRANSLATIONS)
        set(TR_CLEAN_TRANSLATIONS "No")
    endif()
    if (NOT DEFINED TR_PULL_TRANSLATIONS)
        set(TR_PULL_TRANSLATIONS "No")
    endif()
    if (NOT DEFINED TR_REPO_SUBDIR)
        set(TR_REPO_SUBDIR "${TR_TEMPLATE}")
    endif()

    if (NOT DEFINED TR_TRANSLATIONS_REPO)
        set(TR_TRANSLATIONS_REPO "https://github.com/lxde/lxqt-l10n.git")
    endif()

    if (NOT DEFINED TR_TRANSLATIONS_REFSPEC)
        set(TR_TRANSLATIONS_REFSPEC "master")
    endif()

    if (TR_CLEAN_TRANSLATIONS)
        message(STATUS "Cleaning translations dir '${TR_TRANSLATION_DIR}' ...")
        set(DIR_TO_REMOVE "${TR_TRANSLATION_DIR}/${TR_REPO_SUBDIR}")
        get_filename_component(PARENT_DIR "${DIR_TO_REMOVE}" DIRECTORY)
        while (NOT "${PARENT_DIR}" STREQAL "${TR_TRANSLATION_DIR}")
            set(DIR_TO_REMOVE "${PARENT_DIR}")
            get_filename_component(PARENT_DIR "${DIR_TO_REMOVE}" DIRECTORY)
        endwhile ()
        #TODO: is there a way to check successfulness of file command !?!
        file(REMOVE_RECURSE "${TR_TRANSLATION_DIR}/.git" "${DIR_TO_REMOVE}")
    endif ()

    if (TR_PULL_TRANSLATIONS)
        find_package(Git REQUIRED)
        if (NOT (GIT_FOUND AND GIT_VERSION_STRING VERSION_GREATER "1.7.0"))
            message(FATAL_ERROR "Git > 1.7.0 is needed For pulling translations!")
        endif ()
        if (NOT EXISTS "${TR_TRANSLATION_DIR}/${TR_REPO_SUBDIR}")
            message(STATUS "Setting git repository in the translations dir '${TR_TRANSLATION_DIR}' ...")
            if (EXISTS "${TR_TRANSLATION_DIR}/.git")
                execute_process(COMMAND rm -Rf .git
                    WORKING_DIRECTORY  "${TR_TRANSLATION_DIR}"
                    RESULT_VARIABLE ex_result
                    )

                if (NOT "${ex_result}" EQUAL 0)
                    message(FATAL_ERROR "Initialization(cleanup) of translations dir failed!")
                endif ()
            endif()

            # make sure the dir exist, otherwise git init will fail
            file(MAKE_DIRECTORY "${TR_TRANSLATION_DIR}")

            execute_process(COMMAND "${GIT_EXECUTABLE}" init
                WORKING_DIRECTORY  "${TR_TRANSLATION_DIR}"
                RESULT_VARIABLE ex_result
                )
            if (NOT "${ex_result}" EQUAL 0)
                message(FATAL_ERROR "Initialization(init) of translations dir failed!")
            endif ()

            execute_process(COMMAND "${GIT_EXECUTABLE}" remote add  origin "${TR_TRANSLATIONS_REPO}"
                WORKING_DIRECTORY  "${TR_TRANSLATION_DIR}"
                RESULT_VARIABLE ex_result
                )
            if (NOT "${ex_result}" EQUAL 0)
                message(FATAL_ERROR "Initialization(remote) of translations dir failed!")
            endif ()

            execute_process(COMMAND "${GIT_EXECUTABLE}" config core.sparseCheckout true
                WORKING_DIRECTORY  "${TR_TRANSLATION_DIR}"
                RESULT_VARIABLE ex_result
                )
            if (NOT "${ex_result}" EQUAL 0)
                message(FATAL_ERROR "Initialization(config) of translations dir failed!")
            endif ()

            file(WRITE "${TR_TRANSLATION_DIR}/.git/info/sparse-checkout" "${TR_REPO_SUBDIR}")
        endif ()

        message(STATUS "Pulling the translations...")
        execute_process(COMMAND "${GIT_EXECUTABLE}" pull origin "${TR_TRANSLATIONS_REFSPEC}"
            WORKING_DIRECTORY  "${TR_TRANSLATION_DIR}"
            RESULT_VARIABLE ex_result
            )
        if (NOT "${ex_result}" EQUAL 0)
            message(FATAL_ERROR "Pulling translations failed!")
        endif ()
    endif ()

    #project/module can use it's own translations (not from external)
    if (EXISTS "${TR_TRANSLATION_DIR}/${TR_REPO_SUBDIR}/")
        file(GLOB tsFiles "${TR_TRANSLATION_DIR}/${TR_REPO_SUBDIR}/*_*.ts")
        set(templateFile "${TR_TRANSLATION_DIR}/${TR_REPO_SUBDIR}/${TR_TEMPLATE}.ts")
    else ()
        file(GLOB tsFiles "${TR_TRANSLATION_DIR}/${TR_TEMPLATE}_*.ts")
        set(templateFile "${TR_TRANSLATION_DIR}/${TR_TEMPLATE}.ts")
    endif ()

    if(TR_USE_QT5)
        # Qt5
        if (TR_UPDATE_TRANSLATIONS)
            qt5_patched_create_translation(QMS
                ${TR_SOURCES}
                ${templateFile}
                OPTIONS ${TR_UPDATE_OPTIONS}
            )
            qt5_patched_create_translation(QM
                ${TR_SOURCES}
                ${tsFiles}
                OPTIONS ${TR_UPDATE_OPTIONS}
            )
        else()
            qt5_patched_add_translation(QM ${tsFiles})
        endif()
    else()
        # Qt4
        if(TR_UPDATE_TRANSLATIONS)
            qt4_create_translation(QMS
                ${TR_SOURCES}
                ${templateFile}
                OPTIONS ${TR_UPDATE_OPTIONS}
            )
            qt4_create_translation(QM
                ${TR_SOURCES}
                ${tsFiles}
                OPTIONS ${TR_UPDATE_OPTIONS}
            )
        else()
            qt4_add_translation(QM ${tsFiles})
        endif()
    endif()

    if(TR_UPDATE_TRANSLATIONS)
        add_custom_target("update_${TR_TEMPLATE}_ts" ALL DEPENDS ${QMS})
    endif()

    if(DEFINED TR_INSTALL_DIR)
        if(NOT DEFINED TR_COMPONENT)
            set(TR_COMPONENT "Runtime")
        endif()

        install(FILES ${QM}
            DESTINATION "${TR_INSTALL_DIR}"
            COMPONENT "${TR_COMPONENT}"
        )
    endif()

    set(${qmFiles} ${QM} PARENT_SCOPE)
endfunction()
