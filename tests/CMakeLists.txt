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

if (YUP_PLATFORM_EMSCRIPTEN)
    add_definitions (-DTEST_HAS_PTHREAD=0)
endif()

# ==== Setup googletests
_yup_fetch_googletest()

# ==== Create executable
set (target_name yup_tests)
set (target_version "1.0.0")
set (target_modules
    juce_core
    juce_events
    juce_audio_basics
    juce_audio_devices
    yup_graphics)

enable_testing()

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Tests"
    TARGET_CONSOLE ON
    DEFINITIONS
        JUCE_USE_CURL=0
    MODULES
        ${target_modules}
        GTest::gtest_main
        GTest::gmock_main)

# ==== Setup sources
file (GLOB_RECURSE sources
      "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

if (YUP_PLATFORM_OSX OR YUP_PLATFORM_IOS)
    file (GLOB_RECURSE apple_sources
        "${CMAKE_CURRENT_LIST_DIR}/*.mm")
    list (APPEND sources ${apple_sources})
endif()

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
set_target_properties (${target_name} PROPERTIES
    XCODE_SCHEME_ARGUMENTS "--gtest_filter=*"
    VS_DEBUGGER_COMMAND_ARGUMENTS "--gtest_filter=*")
