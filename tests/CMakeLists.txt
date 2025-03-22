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
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable (googletest)
set_target_properties (gtest PROPERTIES FOLDER "Tests")
set_target_properties (gtest_main PROPERTIES FOLDER "Tests")
set_target_properties (gmock PROPERTIES FOLDER "Tests")
set_target_properties (gmock_main PROPERTIES FOLDER "Tests")

if ("${yup_platform}" MATCHES "^(emscripten)$")
    target_compile_options (gmock PRIVATE -pthread)
    target_compile_options (gtest PRIVATE -pthread)
endif()

# ==== Create executable
set (target_name yup_tests)
set (target_version "1.0.0")
set (target_modules
    juce_core
    juce_events
    juce_audio_basics
    juce_audio_devices
    yup_data_model)

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
        GTest::gmock_main
)

# ==== Setup sources
file (GLOB_RECURSE sources
      "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
      "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

if ("${yup_platform}" MATCHES "^(osx|ios)$")
    file (GLOB_RECURSE apple_sources
        "${CMAKE_CURRENT_LIST_DIR}/*.mm")
    list (APPEND sources ${apple_sources})
endif()

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
