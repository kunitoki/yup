# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2024 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   To use, copy, modify, and/or distribute this software for any purpose with or
#   without fee is hereby granted provided that the above copyright notice and
#   this permission notice appear in all copies.
#
#   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#   DISCLAIMED.
#
# ==============================================================================

#==============================================================================

macro (_yup_fetchcontent_declare name GIT_REPOSITORY git_repository GIT_TAG git_tag)
    FetchContent_Declare(
		"${name}"
		GIT_REPOSITORY "${git_repository}"
        GIT_TAG "${git_tag}"
        GIT_SUBMODULES_RECURSE ON
        SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/${name}")

    #if (NOT DEFINED FETCHCONTENT_BASE_DIR)
    #    set (FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/externals")
    #endif()
    #FetchContent_Declare(
	#	"${name}"
	#	DOWNLOAD_COMMAND
	#		cd "${FETCHCONTENT_BASE_DIR}/${name}-src" &&
	#		git init &&
	#		git fetch --depth=1 --progress "${git_repository}" "${git_tag}" &&
	#		git reset --hard FETCH_HEAD)
endmacro()

#==============================================================================

function (_yup_fetch_sdl2)
    if (TARGET sdl2::sdl2)
        return()
    endif()

    _yup_fetchcontent_declare (SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.8)

    set (BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set (SDL_SHARED OFF CACHE BOOL "" FORCE)
    set (SDL_STATIC ON CACHE BOOL "" FORCE)
    set (SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
    set (SDL_TESTS OFF CACHE BOOL "" FORCE)
    set (SDL_AUDIO_ENABLED_BY_DEFAULT OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable (SDL2)

    if (APPLE)
        target_compile_options (SDL2-static PRIVATE -Wno-deprecated-declarations)
    endif()

    set_target_properties (SDL2-static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "Thirdparty")

    set_target_properties (SDL2main PROPERTIES FOLDER "Thirdparty")
    set_target_properties (SDL2_test PROPERTIES FOLDER "Thirdparty")
    set_target_properties (sdl_headers_copy PROPERTIES FOLDER "Thirdparty")
    set_target_properties (uninstall PROPERTIES FOLDER "Thirdparty")

    add_library (sdl2::sdl2 ALIAS SDL2-static)
endfunction()

#==============================================================================

function (_yup_fetch_perfetto)
    if (TARGET perfetto::perfetto)
        return()
    endif()

    _yup_fetchcontent_declare (Perfetto
        GIT_REPOSITORY https://android.googlesource.com/platform/external/perfetto
        GIT_TAG v49.0)

    FetchContent_MakeAvailable (Perfetto)

    add_library (perfetto STATIC)
    target_compile_features (perfetto PUBLIC cxx_std_17)

    target_sources (perfetto
        PRIVATE "$<BUILD_INTERFACE:${perfetto_SOURCE_DIR}/sdk/perfetto.cc>"
        PUBLIC "$<BUILD_INTERFACE:${perfetto_SOURCE_DIR}/sdk/perfetto.h>")

    target_include_directories (perfetto PUBLIC
        "$<BUILD_INTERFACE:${perfetto_SOURCE_DIR}/sdk>")

    set_target_properties (perfetto PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "Thirdparty")

    if (WIN32)
        target_compile_definitions (perfetto PUBLIC NOMINMAX=1 WIN32_LEAN_AND_MEAN=1)
        if (MSVC)
            target_compile_options (perfetto PRIVATE /bigobj PUBLIC /Zc:__cplusplus /permissive-)
        endif()
    endif()

    add_library (perfetto::perfetto ALIAS perfetto)
endfunction()

#==============================================================================

function (_yup_fetch_python use_static_libs modules)
    if (TARGET Python::Python OR TARGET Python::Module)
        return()
    endif()

    set (Python_USE_STATIC_LIBS "${use_static_libs}")
    find_package (Python REQUIRED COMPONENTS ${modules})

endfunction()

