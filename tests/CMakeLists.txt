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

if ("${yup_platform}" MATCHES "^(emscripten)$")
    add_definitions (-DTEST_HAS_PTHREAD=0)
    set (gtest_disable_pthreads ON CACHE BOOL "" FORCE)
endif()

# ==== Setup googletests
include (FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable (googletest)

# ==== Create executable
set (target_name yup_tests)

enable_testing()

yup_standalone_app (
    TARGET_NAME ${target_name}
    DEFINITIONS
        JUCE_USE_CURL=0
    MODULES
        juce_core
        juce_events
        juce_audio_basics
        juce_audio_devices
        GTest::gtest_main
        GTest::gmock_main
)

# ==== Setup sources
file (GLOB_RECURSE sources
      "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
      "${CMAKE_CURRENT_LIST_DIR}/*.mm")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
