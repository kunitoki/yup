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
set (target_version "1.0.0")

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

yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    TARGET_CXX_STANDARD 20
    PLUGIN_ID "org.yup.YupSynth"
    PLUGIN_NAME "YupSynth"
    PLUGIN_VENDOR "org.yup"
    PLUGIN_EMAIL "kunitoki@gmail.com"
    PLUGIN_VERSION "${target_version}"
    PLUGIN_DESCRIPTION "The best audio plugin ever."
    PLUGIN_URL "https://github.com/kunitoki/yup"
    PLUGIN_IS_SYNTH ON
    PLUGIN_IS_MONO OFF
    PLUGIN_CREATE_CLAP ON
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_AU ON
    PLUGIN_CREATE_AUv3 ON
    PLUGIN_CREATE_STANDALONE ON
    PLUGIN_CREATE_AAX ${plugin_create_aax}
    DEFINITIONS
        YupPlugin_AAX_ManufacturerID='YUPP'
        YupPlugin_AAX_ProductID='YpSy'
        YupPlugin_AAX_PlugInID_Native='YpSN'
        YupPlugin_AAX_PlugInID_AudioSuite='YpSA'
        YupPlugin_AAXCategory=AAX_ePlugInCategory_Example
    MODULES
        yup::yup_gui
        yup::yup_audio_processors)

# ==== Prepare sources
file (GLOB_RECURSE sources
    "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/*.h")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name}_shared PUBLIC ${sources})
