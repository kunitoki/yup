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

macro (_yup_fetchcontent_declare name)
    cmake_parse_arguments (_yup_fcd "" "GIT_REPOSITORY;GIT_TAG" "GIT_SUBMODULES" ${ARGN})

    set (_yup_fcd_submodules_args "")
    if (DEFINED _yup_fcd_GIT_SUBMODULES)
        set (_yup_fcd_submodules_args GIT_SUBMODULES ${_yup_fcd_GIT_SUBMODULES})
    endif()

    FetchContent_Declare(
        "${name}"
        GIT_REPOSITORY "${_yup_fcd_GIT_REPOSITORY}"
        GIT_TAG        "${_yup_fcd_GIT_TAG}"
        GIT_SUBMODULES_RECURSE ON
        ${_yup_fcd_submodules_args}
        SOURCE_DIR "${CMAKE_BINARY_DIR}/externals/${name}")
endmacro()

#==============================================================================

function (_yup_fetch_sdl)
    if (TARGET sdl::sdl)
        return()
    endif()

    _yup_message (STATUS "Fetching SDL3")

    _yup_fetchcontent_declare (SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.12)

    set (BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set (SDL_SHARED OFF CACHE BOOL "" FORCE)
    set (SDL_STATIC ON CACHE BOOL "" FORCE)
    set (SDL_STATIC_PIC ON CACHE BOOL "" FORCE)
    set (SDL_TEST_LIBRARYS OFF CACHE BOOL "" FORCE)
    set (SDL_TESTS OFF CACHE BOOL "" FORCE)
    set (SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set (SDL_DISABLE_INSTALL_DOCS ON CACHE BOOL "" FORCE)
    set (SDL_INSTALL_EXAMPLES OFF CACHE BOOL "" FORCE)
    set (SDL_AUDIO_ENABLED_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set (SDL_DISKAUDIO OFF CACHE BOOL "" FORCE)
    set (SDL_HIDAPI_LIBUSB OFF CACHE BOOL "" FORCE)
    set (SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    set (SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
    set (SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set (SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
    set (SDL_X11_XTEST OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable (SDL3)

    if (APPLE)
        target_compile_options (SDL3-static PRIVATE
            -Wno-deprecated-declarations
            -Wno-gnu-folding-constant)
    endif()

    set_target_properties (SDL3-static PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        FOLDER "Thirdparty")

    set_target_properties (SDL3_test PROPERTIES FOLDER "Tests")

    add_library (sdl::sdl ALIAS SDL3-static)
endfunction()

#==============================================================================

function (_yup_fetch_perfetto)
    if (TARGET perfetto::perfetto)
        return()
    endif()

    _yup_message (STATUS "Fetching Perfetto")

    _yup_fetchcontent_declare (Perfetto
        GIT_REPOSITORY https://android.googlesource.com/platform/external/perfetto
        GIT_TAG v49.0)

    FetchContent_MakeAvailable (Perfetto)

    add_library (perfetto STATIC)
    target_compile_features (perfetto PUBLIC cxx_std_20)

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

macro (_yup_fetch_python use_static_libs modules)
    if (TARGET Python::Python OR TARGET Python::Module)
        return()
    endif()

    set (Python_USE_STATIC_LIBS "${use_static_libs}")

    find_package (Python QUIET COMPONENTS ${modules})

    if (NOT Python_FOUND)
        string (REPLACE "Development.Module" "Development" fallback_modules "${modules}")
        if (NOT "${fallback_modules}" STREQUAL "${modules}")
            find_package (Python QUIET COMPONENTS ${fallback_modules})
        endif()

        if (NOT Python_FOUND)
            find_package (Python QUIET COMPONENTS Interpreter Development)
        endif()

        if (NOT Python_FOUND)
            find_package (Python QUIET COMPONENTS Interpreter)
        endif()
    endif()

    if (NOT Python_FOUND)
        find_package (Python REQUIRED COMPONENTS ${modules})
    endif()
endmacro()

#==============================================================================

function (_yup_find_fftw3 target_name)
    if (TARGET PkgConfig::FFTW AND TARGET FFTW::Float)
    else()
        find_package (PkgConfig REQUIRED)
        pkg_check_modules (FFTW IMPORTED_TARGET REQUIRED fftw3)
        find_library (FFTWF_LIB NAMES "fftw3f" PATHS ${PKG_FFTW_LIBRARY_DIRS} ${LIB_INSTALL_DIR})

        if (FFTWF_LIB)
            add_library (FFTW::Float INTERFACE IMPORTED)
            set_target_properties (FFTW::Float
                PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FFTW_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${FFTWF_LIB}")
        else()
            _yup_message (FATAL_ERROR "FFTW3 library not found")
        endif()
    endif()

    target_include_directories (${target_name} PRIVATE PkgConfig::FFTW)
    target_link_libraries (${target_name} PRIVATE FFTW::Float)
endfunction()

#==============================================================================

function (_yup_collect_audio_plugin_host_dependencies definitions output_variable)
    set (dependencies "")

    _yup_definitions_enable ("${definitions}" YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP enable_clap)
    if (enable_clap)
        _yup_fetch_clap()
        list (APPEND dependencies clap)
    endif()

    _yup_definitions_enable ("${definitions}" YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3 enable_vst3)
    if (enable_vst3)
        _yup_fetch_vst3sdk()
        list (APPEND dependencies yup_audio_plugin_host_vst3sdk)
    endif()

    _yup_definitions_enable ("${definitions}" YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2 enable_lv2)
    if (enable_lv2)
        _yup_fetch_lv2()
        list (APPEND dependencies lv2-headers lilv-static)
    endif()

    _yup_definitions_enable ("${definitions}" YUP_AUDIO_PLUGIN_HOST_ENABLE_AU enable_au)
    if (enable_au AND YUP_PLATFORM_MAC)
        list (APPEND dependencies
            "-framework AudioUnit"
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework CoreFoundation")
    endif()

    set (${output_variable} "${dependencies}" PARENT_SCOPE)
endfunction()
