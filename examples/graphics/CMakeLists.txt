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

cmake_minimum_required (VERSION 3.31)

# ==== Set target name and version
set (target_name example_graphics)
set (target_version "2.0.0")

project (${target_name} VERSION ${target_version})

set (rive_file "data/alien.riv")
set (lottie_file "data/goal.lottie")

# ==== Prepare Android build
set (link_libraries "")
if (ANDROID)
    include (../../cmake/yup.cmake)
    yup_add_default_modules ("${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

# ==== Embed data files on platforms without access to the source tree at runtime
if (ANDROID OR YUP_PLATFORM_IOS)
    yup_add_embedded_binary_resources (
        "${target_name}_binary_data"
        OUT_DIR BinaryData
        HEADER BinaryData.h
        NAMESPACE yup
        RESOURCE_NAMES
            RiveFile
            LottieFile
        RESOURCES
            "${CMAKE_CURRENT_LIST_DIR}/${rive_file}"
            "${CMAKE_CURRENT_LIST_DIR}/${lottie_file}")

    set (link_libraries "${target_name}_binary_data")
endif()

# ==== Embed shaders
yup_add_shader_bundle (
    "${target_name}_binary_shaders"
    VERT ${CMAKE_CURRENT_LIST_DIR}/data/shaders/cube.vert
    FRAG ${CMAKE_CURRENT_LIST_DIR}/data/shaders/cube.frag
    NAMESPACE yup
    OUTPUT_NAME
        ShaderBundle
    RESOURCE_NAME
        ShaderBundleFile)
list (APPEND link_libraries "${target_name}_binary_shaders")

# ==== Prepare target
set (additional_modules "")
set (additional_definitions "")
if (YUP_PLATFORM_DESKTOP AND NOT YUP_PLATFORM_WINDOWS)
    set (additional_modules yup::yup_python)
endif()

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    PTHREAD_POOL_SIZE 16
    STACK_SIZE 4194304
    DEFINITIONS
        YUP_EXAMPLE_GRAPHICS_RIVE_FILE="${rive_file}"
        YUP_EXAMPLE_GRAPHICS_LOTTIE_FILE="${lottie_file}"
        ${additional_definitions}
    PRELOAD_FILES
        "${CMAKE_CURRENT_LIST_DIR}/${rive_file}@${rive_file}"
        "${CMAKE_CURRENT_LIST_DIR}/${lottie_file}@${lottie_file}"
    MODULES
        yup::yup_core
        yup::yup_audio_basics
        yup::yup_audio_devices
        yup::yup_dsp
        yup::yup_events
        yup::yup_graphics
        yup::yup_animation
        yup::yup_gui
        yup::yup_audio_gui
        yup::yup_audio_processors
        yup::yup_audio_formats
        yup::yup_shading
        yup::yup_ai
        bungee_library
        pffft_library
        opus_library
        flac_library
        hmp3_library
        dr_libs
        libvorbis
        libpng
        libwebp
        libjpeg
        libgif
        libtiff
        glslang
        spirv_cross
        spirv_tools
        ${additional_modules}
        ${link_libraries})

# ==== Prepare sources
if (NOT YUP_TARGET_ANDROID)
    file (GLOB_RECURSE sources
        "${CMAKE_CURRENT_LIST_DIR}/source/examples/*.h"
        "${CMAKE_CURRENT_LIST_DIR}/source/main.cpp")
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
    target_sources (${target_name} PRIVATE ${sources})
endif()
