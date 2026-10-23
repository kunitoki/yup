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

cmake_minimum_required(VERSION 3.31)

# ==== Prepare target
set (target_name example_plugin)
set (target_version "2.0.0")

project (${target_name} VERSION ${target_version})

# ==== Auto-detect AAX SDK
set (plugin_create_aax OFF)

if (DEFINED YUP_AAX_SDK_ROOT OR DEFINED ENV{YUP_AAX_SDK_ROOT})
    if (DEFINED ENV{YUP_AAX_SDK_ROOT} AND NOT DEFINED YUP_AAX_SDK_ROOT)
        set (YUP_AAX_SDK_ROOT "$ENV{YUP_AAX_SDK_ROOT}" CACHE PATH "Path to the AAX SDK")
    endif()
    message (STATUS "AAX SDK using YUP_AAX_SDK_ROOT: ${YUP_AAX_SDK_ROOT}")
    set (plugin_create_aax ON)
else()
    set (aax_sdk_candidate "${CMAKE_CURRENT_LIST_DIR}/../../../AAX/aax-sdk-2-9-0")
    if (EXISTS "${aax_sdk_candidate}/Interfaces/AAX.h")
        set (YUP_AAX_SDK_ROOT "${aax_sdk_candidate}" CACHE PATH "Path to the AAX SDK")
        set (plugin_create_aax ON)
        message (STATUS "AAX SDK auto-detected at ${aax_sdk_candidate}")
    else()
        message (STATUS "AAX SDK not found — AAX target disabled")
    endif()
endif()

# ==== Embed the waveform display shader
yup_add_shader_bundle (${target_name}_shaders
    VERT "${CMAKE_CURRENT_LIST_DIR}/data/shaders/fullscreen.vert"
    FRAG "${CMAKE_CURRENT_LIST_DIR}/data/shaders/synth_waveform.frag"
    NAMESPACE prism
    OUTPUT_NAME SynthWaveformShaderBundle
    RESOURCE_NAME synthWaveformShader)

# ==== Embed the bloom shaders, one bundle per fragment stage
yup_add_shader_bundle (${target_name}_bloom_prefilter_shaders
    VERT "${CMAKE_CURRENT_LIST_DIR}/data/shaders/fullscreen.vert"
    FRAG "${CMAKE_CURRENT_LIST_DIR}/data/shaders/prism_bloom_prefilter.frag"
    NAMESPACE prism
    OUTPUT_NAME SynthBloomPrefilterShaderBundle
    RESOURCE_NAME synthBloomPrefilterShader)

yup_add_shader_bundle (${target_name}_bloom_blur_shaders
    VERT "${CMAKE_CURRENT_LIST_DIR}/data/shaders/fullscreen.vert"
    FRAG "${CMAKE_CURRENT_LIST_DIR}/data/shaders/prism_bloom_blur.frag"
    NAMESPACE prism
    OUTPUT_NAME SynthBloomBlurShaderBundle
    RESOURCE_NAME synthBloomBlurShader)

yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "yup.audio.${target_name}"
    TARGET_APP_NAMESPACE "yup.audio"
    TARGET_CXX_STANDARD 20
    PLUGIN_ID "yup.audio.Prism"
    PLUGIN_NAME "Prism"
    PLUGIN_VENDOR "yup.audio"
    PLUGIN_EMAIL "kunitoki@gmail.com"
    PLUGIN_VERSION "${target_version}"
    PLUGIN_DESCRIPTION "A two oscillator spectral synthesiser."
    PLUGIN_URL "https://github.com/kunitoki/yup"
    PLUGIN_IS_SYNTH ON
    PLUGIN_IS_MONO OFF
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_AU ON
    PLUGIN_CREATE_AUv3 ON
    PLUGIN_CREATE_LV2 ON
    PLUGIN_CREATE_STANDALONE ON
    PLUGIN_CREATE_AAX ${plugin_create_aax}
    PLUGIN_AAX_MANUFACTURER_ID YUPP
    PLUGIN_AAX_PRODUCT_ID YpSy
    PLUGIN_AAX_PLUGIN_ID_NATIVE YpSN
    PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE YpSA
    PLUGIN_AAX_CATEGORY AAX_ePlugInCategory_Example
    PLUGIN_COPY_AFTER_BUILD ON
    MODULES
        yup::yup_gui
        yup::yup_audio_processors
        yup::yup_audio_gui
        pffft_library
        ${target_name}_shaders
        ${target_name}_bloom_prefilter_shaders
        ${target_name}_bloom_blur_shaders)

# ==== Prepare sources
file (GLOB_RECURSE sources
    "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/*.h")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
# The shader bundles are OBJECT libraries, whose objects only reach targets linking them directly
target_sources (${target_name}_shared PUBLIC ${sources}
    $<TARGET_OBJECTS:${target_name}_shaders>
    $<TARGET_OBJECTS:${target_name}_bloom_prefilter_shaders>
    $<TARGET_OBJECTS:${target_name}_bloom_blur_shaders>)
