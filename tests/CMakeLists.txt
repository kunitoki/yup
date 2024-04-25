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

set (target_name yup_tests)

# ==== Setup googletests
include (FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable (googletest)

# ==== Setup sources
file(GLOB_RECURSE sources
    "${CMAKE_CURRENT_LIST_DIR}/*.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.mm")

_yup_source_group (${sources} ${CMAKE_CURRENT_LIST_DIR})

# ==== Create executable
enable_testing()

add_executable (${target_name} ${sources})

target_compile_features (${target_name} PUBLIC cxx_std_17)

if ("${yup_platform}" STREQUAL "emscripten")
    target_link_options (${target_name} PUBLIC
        -sWASM=1 -sASSERTIONS=1 -sERROR_ON_UNDEFINED_SYMBOLS=1 -sDEMANGLE_SUPPORT=1 -sSTACK_OVERFLOW_CHECK=2 -sFORCE_FILESYSTEM=1 -sNODERAWFS=0)
endif()

target_compile_definitions (${target_name} PUBLIC
    $<$<CONFIG:DEBUG>:DEBUG=1>
    $<$<CONFIG:RELEASE>:NDEBUG=1>
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries (${target_name} PUBLIC
    juce_core
    juce_events
    juce_audio_basics
    juce_audio_devices
    GTest::gtest_main)

# ==== Discover tests
include (GoogleTest)
gtest_discover_tests (${target_name})
