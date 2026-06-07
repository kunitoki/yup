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
        target_compile_options (SDL2-static PRIVATE
            -Wno-deprecated-declarations
            -Wno-gnu-folding-constant)
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

function (_yup_fetch_apple_ausdk)
    if (NOT TARGET base-sdk-auv2)
        if (NOT AUDIOUNIT_SDK_ROOT)
            _yup_message (STATUS "Fetching Apple AudioUnitSDK")
            _yup_fetchcontent_declare (AudioUnitSDK
                GIT_REPOSITORY https://github.com/apple/AudioUnitSDK.git
                GIT_TAG        AudioUnitSDK-1.1.0)
            FetchContent_MakeAvailable (AudioUnitSDK)
            set (AUDIOUNIT_SDK_ROOT "${audiounitsdk_SOURCE_DIR}")
        endif()

        set (AUSDK_SRC "${AUDIOUNIT_SDK_ROOT}/src/AudioUnitSDK")

        add_library (base-sdk-auv2 STATIC
            "${AUSDK_SRC}/AUBase.cpp"
            "${AUSDK_SRC}/AUBuffer.cpp"
            "${AUSDK_SRC}/AUBufferAllocator.cpp"
            "${AUSDK_SRC}/AUEffectBase.cpp"
            "${AUSDK_SRC}/AUInputElement.cpp"
            "${AUSDK_SRC}/AUMIDIBase.cpp"
            "${AUSDK_SRC}/AUMIDIEffectBase.cpp"
            "${AUSDK_SRC}/AUOutputElement.cpp"
            "${AUSDK_SRC}/AUPlugInDispatch.cpp"
            "${AUSDK_SRC}/AUScopeElement.cpp"
            "${AUSDK_SRC}/ComponentBase.cpp"
            "${AUSDK_SRC}/MusicDeviceBase.cpp")

        target_include_directories (base-sdk-auv2 PUBLIC "${AUDIOUNIT_SDK_ROOT}/include")
        target_compile_features (base-sdk-auv2 PUBLIC cxx_std_17)
        target_compile_options (base-sdk-auv2 PRIVATE -Wno-deprecated-declarations)

        set_target_properties (base-sdk-auv2 PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            FOLDER "Thirdparty")
    endif()
endfunction()

#==============================================================================

function (_yup_fetch_clap)
    if (NOT TARGET clap)
        _yup_message (STATUS "Fetching CLAP SDK")
        _yup_fetchcontent_declare (clap
            GIT_REPOSITORY https://github.com/free-audio/clap.git
            GIT_TAG main)

        FetchContent_MakeAvailable (clap)
    endif()

    if (TARGET clap-tests)
        set_target_properties (clap-tests PROPERTIES FOLDER "Tests")
    endif()
endfunction()

#==============================================================================

function (_yup_fetch_vst3sdk)
    if (NOT TARGET sdk)
        _yup_message (STATUS "Fetching VST3 SDK")

        set (SMTG_CREATE_MODULE_INFO OFF)
        set (SMTG_ADD_VST3_UTILITIES OFF)
        set (SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
        set (SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
        set (SMTG_ENABLE_VSTGUI_SUPPORT OFF)
        set (SMTG_CREATE_PLUGIN_LINK OFF)
        if (NOT YUP_PLATFORM_MAC OR XCODE)
            set (SMTG_RUN_VST_VALIDATOR ON)
        else()
            set (SMTG_RUN_VST_VALIDATOR OFF)
        endif()

        _yup_fetchcontent_declare (vst3sdk
            GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
            GIT_TAG master)

        FetchContent_MakeAvailable (vst3sdk)
    endif()

    if (NOT TARGET yup_audio_plugin_host_vst3sdk)
        add_library (yup_audio_plugin_host_vst3sdk INTERFACE)
        target_link_libraries (yup_audio_plugin_host_vst3sdk INTERFACE sdk)

        set (vst3sdk_source_dir "")
        if (DEFINED vst3sdk_SOURCE_DIR)
            set (vst3sdk_source_dir "${vst3sdk_SOURCE_DIR}")
        elseif (TARGET sdk)
            get_target_property (vst3sdk_source_dir sdk SOURCE_DIR)
        endif()

        set (vst3sdk_memorystream_source "${vst3sdk_source_dir}/public.sdk/source/common/memorystream.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_memorystream_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_memorystream_source}")
        endif()

        set (vst3sdk_parameterchanges_source "${vst3sdk_source_dir}/public.sdk/source/vst/hosting/parameterchanges.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_parameterchanges_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_parameterchanges_source}")
        endif()

        set (vst3sdk_eventlist_source "${vst3sdk_source_dir}/public.sdk/source/vst/hosting/eventlist.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_eventlist_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_eventlist_source}")
        endif()
    endif()
endfunction()

#==============================================================================

function (_yup_target_list_contains target_list target_name output_variable)
    foreach (target IN LISTS target_list)
        if ("${target}" STREQUAL "${target_name}" OR "${target}" STREQUAL "yup::${target_name}")
            set (${output_variable} ON PARENT_SCOPE)
            return()
        endif()

        if (TARGET "${target}")
            get_target_property (aliased_target "${target}" ALIASED_TARGET)
            if ("${aliased_target}" STREQUAL "${target_name}")
                set (${output_variable} ON PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()

    set (${output_variable} OFF PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_definitions_enable definitions definition_name output_variable)
    set (enabled OFF)

    foreach (definition IN LISTS definitions)
        string (REGEX REPLACE "^-D" "" normalized_definition "${definition}")

        if (normalized_definition MATCHES "^${definition_name}($|=)")
            set (enabled ON)

            if (normalized_definition MATCHES "^${definition_name}=")
                string (REGEX REPLACE "^${definition_name}=(.*)$" "\\1" definition_value "${normalized_definition}")
                string (STRIP "${definition_value}" definition_value)
                string (REGEX REPLACE "^\"(.*)\"$" "\\1" definition_value "${definition_value}")
                string (REGEX REPLACE "^'(.*)'$" "\\1" definition_value "${definition_value}")
                string (TOUPPER "${definition_value}" definition_value)

                if ("${definition_value}" STREQUAL "0"
                    OR "${definition_value}" STREQUAL "OFF"
                    OR "${definition_value}" STREQUAL "FALSE"
                    OR "${definition_value}" STREQUAL "NO")
                    set (enabled OFF)
                endif()
            endif()
        endif()
    endforeach()

    set (${output_variable} "${enabled}" PARENT_SCOPE)
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
