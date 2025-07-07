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

enable_testing()

# ==== Setup googletests
if (YUP_PLATFORM_EMSCRIPTEN)
    set (gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    set (gtest_disable_warnings ON CACHE BOOL "" FORCE)
    set (gtest_hide_internal_symbols ON CACHE BOOL "" FORCE)
    add_definitions (-DTEST_HAS_PTHREAD=0)
endif()

set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

_yup_fetchcontent_declare (googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)

FetchContent_MakeAvailable (googletest)

if (YUP_PLATFORM_EMSCRIPTEN)
    target_compile_options (gmock PRIVATE -pthread)
    target_compile_options (gtest PRIVATE -pthread)
endif()

set_target_properties (gtest PROPERTIES FOLDER "Tests")
set_target_properties (gtest_main PROPERTIES FOLDER "Tests")
set_target_properties (gmock PROPERTIES FOLDER "Tests")
set_target_properties (gmock_main PROPERTIES FOLDER "Tests")

# ==== Create executable
set (target_name yup_tests)
set (target_version "1.0.0")
set (target_console OFF)
set (target_gtest_modules "")
set (target_modules
    yup_core
    yup_audio_basics
    yup_audio_devices
    yup_events
    yup_data_model
    yup_graphics)

if (NOT YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND target_modules yup_gui)
    if (YUP_PLATFORM_DESKTOP)
        list (APPEND target_modules yup_python)
    endif()
    list (APPEND target_gtest_modules
        GTest::gtest
        GTest::gmock)
else()
    set (target_console ON)
    list (APPEND target_gtest_modules
        GTest::gtest_main
        GTest::gmock_main)
endif()

if (YUP_PLATFORM_MACOS OR YUP_PLATFORM_WINDOWS)
    set (python_tools_path "${CMAKE_CURRENT_LIST_DIR}/../python/tools")
    set (python_target_name "${target_name}_python_stdlib")

    yup_prepare_python_stdlib (${python_target_name} ${python_tools_path} python_standard_library)

    yup_add_embedded_binary_resources (
        ${target_name}_python_stdlib_binary
        OUT_DIR PythonStandardLibrary
        HEADER PythonStandardLibrary.h
        NAMESPACE yup
        RESOURCE_NAMES
            PythonStandardLibrary
        RESOURCES
            ${python_standard_library})

    list (APPEND target_modules "${target_name}_python_stdlib_binary")
endif()

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Tests"
    TARGET_CONSOLE ${target_console}
    DEFINITIONS
        YUP_USE_CURL=0
    MODULES
        ${target_modules}
        ${target_gtest_modules})

# ==== Setup sources
set (sources "")
foreach (module ${target_modules})
    file (GLOB_RECURSE module_sources
        "${CMAKE_CURRENT_LIST_DIR}/${module}/*.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/${module}/*.cpp")
    list (APPEND sources ${module_sources})

    if (YUP_PLATFORM_APPLE)
        file (GLOB_RECURSE apple_sources
            "${CMAKE_CURRENT_LIST_DIR}/${module}/*.mm")
        list (APPEND sources ${apple_sources})
    endif()
endforeach()

if (NOT YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND sources "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
endif()

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
set_target_properties (${target_name} PROPERTIES
    XCODE_SCHEME_ARGUMENTS "--gtest_filter=*"
    VS_DEBUGGER_COMMAND_ARGUMENTS "--gtest_filter=*")

# ==== Setup coverage support
if (YUP_ENABLE_COVERAGE)
    _yup_setup_coverage_flags (${target_name})
    foreach (module ${target_modules})
        _yup_setup_coverage_flags (${module})
    endforeach()

    _yup_setup_coverage_targets (${target_name} "${target_modules}")
endif()
