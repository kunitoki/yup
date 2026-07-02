# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   to use, copy, modify, and/or distribute this software for any purpose with or
#   without fee is hereby granted provided that the above copyright notice and
#   this permission notice appear in all copies.
#
#   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#   DISCLAIMED.
#
# ==============================================================================

cmake_minimum_required (VERSION 3.31)

set (target_name example_audiograph)
set (target_version "1.0.0")

project (${target_name} VERSION ${target_version})

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    DEFINITIONS
        YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_AU=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_AUV3=1
        YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3=1
    MODULES
        yup::yup_core
        yup::yup_audio_basics
        yup::yup_audio_devices
        yup::yup_dsp
        yup::yup_events
        yup::yup_graphics
        yup::yup_gui
        yup::yup_audio_formats
        yup::yup_audio_gui
        yup::yup_audio_processors
        yup::yup_audio_graph
        yup::yup_audio_plugin_host
        dr_libs)

file (GLOB_RECURSE sources
    "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/*.h")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
