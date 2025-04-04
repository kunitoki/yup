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

function (_yup_fetch_sdl2)
    if (TARGET sdl2::sdl2)
        return()
    endif()

    _yup_message (STATUS "Fetching SDL2")

    FetchContent_Declare (SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.32.4
        SOURCE_DIR ${CMAKE_BINARY_DIR}/externals/SDL2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)

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

function (_yup_fetch_sdl3)
    if (TARGET sdl3::sdl3)
        return()
    endif()

    _yup_message (STATUS "Fetching SDL3")

    FetchContent_Declare (SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.10
        SOURCE_DIR ${CMAKE_BINARY_DIR}/externals/SDL2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)

    set (BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set (SDL_SHARED OFF CACHE BOOL "" FORCE)
    set (SDL_STATIC ON CACHE BOOL "" FORCE)
    set (SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
    set (SDL_TESTS OFF CACHE BOOL "" FORCE)
    set (SDL_AUDIO_ENABLED_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set (SDL_DISKAUDIO OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable (SDL3)

    if (APPLE)
        target_compile_options (SDL3-static PRIVATE -Wno-deprecated-declarations)
    endif()

    set_target_properties (SDL3-static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "Thirdparty")

    #set_target_properties (SDL3main PROPERTIES FOLDER "Thirdparty")
    #set_target_properties (SDL3_test PROPERTIES FOLDER "Thirdparty")
    #set_target_properties (sdl_headers_copy PROPERTIES FOLDER "Thirdparty")
    #set_target_properties (uninstall PROPERTIES FOLDER "Thirdparty")

    add_library (sdl3::sdl3 ALIAS SDL3-static)
endfunction()

#==============================================================================

function (_yup_fetch_perfetto)
    if (TARGET perfetto::perfetto)
        return()
    endif()

    _yup_message (STATUS "Fetching Perfetto")

    FetchContent_Declare (Perfetto
        GIT_REPOSITORY https://android.googlesource.com/platform/external/perfetto
        GIT_TAG v42.0
        SOURCE_DIR ${CMAKE_BINARY_DIR}/externals/Perfetto
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE)

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
