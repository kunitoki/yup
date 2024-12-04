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

cmake_minimum_required (VERSION 3.28)

# ==== Prepare target
set (target_name example_render)
set (target_version "1.0.0")

set (rive_file "data/seasynth.riv")

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    LINK_OPTIONS ${link_options}
    PRELOAD_FILES
        "${CMAKE_CURRENT_LIST_DIR}/${rive_file}@data/artboard.riv"
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
)

# ==== Prepare sources
file (GLOB_RECURSE sources "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
