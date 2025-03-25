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

cmake_minimum_required(VERSION 3.28)

# ==== Prepare target
set (target_name example_plugin)
set (target_version "1.0.0")

project (${target_name} VERSION ${target_version})

yup_add_embedded_binary_resources (
    "${target_name}_binary_data"
    OUT_DIR BinaryData
    HEADER BinaryData.h
    NAMESPACE yup
    RESOURCE_NAMES
        RobotoRegularFont
    RESOURCES
        ${CMAKE_CURRENT_LIST_DIR}/data/Roboto-Regular.ttf
)

yup_audio_plugin (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    PLUGIN_ID "org.yup.YupCLAP"
    PLUGIN_NAME "YupCLAPPZ"
    PLUGIN_VENDOR "org.yup"
    PLUGIN_VERSION "${target_version}"
    PLUGIN_DESCRIPTION "The best audio plugin ever."
    PLUGIN_URL "https://github.com/kunitoki/yup"
    PLUGING_IS_SYNTH ON
    PLUGIN_IS_MONO OFF
    PLUGIN_CREATE_CLAP OFF
    PLUGIN_CREATE_VST3 ON
    PLUGIN_CREATE_STANDALONE OFF
    MODULES
        juce_core
        juce_events
        juce_audio_basics
        juce_audio_devices
        yup_graphics
        yup_gui
        yup_audio_processors
        harfbuzz
        sheenbidi
        rive
        rive_renderer
        "${target_name}_binary_data"
)

# ==== Prepare sources
file (GLOB_RECURSE sources "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
