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

include (cmake/yup.cmake)
_yup_setup_platform()

if ("${yup_platform}" MATCHES "^(osx)$")
    set (CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

project (yup VERSION 1.0)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# Options
option (YUP_TARGET_ANDROID "Target Android project" OFF)
option (YUP_TARGET_ANDROID_BUILD_GRADLE "When building for Android, build the gradle infrastructure" OFF)
option (YUP_ENABLE_VULKAN "Enable the vulkan renderer on platforms that support it" ON)
option (YUP_ENABLE_PROFILING "Enable the profiling code using Perfetto SDK" OFF)
option (YUP_BUILD_EXAMPLES "Build the examples" ON)
option (YUP_BUILD_TESTS "Build the tests" ON)

# Dependencies modules
_yup_add_default_modules (${CMAKE_CURRENT_LIST_DIR})

# Enable profiling
if (YUP_ENABLE_PROFILING)
    _yup_message (STATUS "Enabling profiling")
    _yup_fetch_perfetto()
endif()

# Targets
if (YUP_BUILD_EXAMPLES)
    _yup_message (STATUS "Building examples")
    add_subdirectory (examples/app)
    add_subdirectory (examples/console)
    add_subdirectory (examples/graphics)
    add_subdirectory (examples/render)
    if (NOT "${yup_platform}" MATCHES "^(emscripten|android)$" AND NOT YUP_TARGET_ANDROID)
        add_subdirectory (examples/plugin)
    endif()
endif()

if (YUP_BUILD_TESTS AND NOT YUP_TARGET_ANDROID)
    _yup_message (STATUS "Building tests")
    add_subdirectory (tests)
endif()
