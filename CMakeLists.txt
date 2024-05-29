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

include(cmake/yup.cmake)
_yup_setup_platform()

if ("${yup_platform}" MATCHES "^(osx)$")
    set (CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

project (yup VERSION 1.0)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

option (YUP_BUILD_EXAMPLES "Build the examples" ON)
option (YUP_BUILD_TESTS "Build the tests" ON)

# Dependencies modules
yup_add_module (thirdparty/glad)
yup_add_module (thirdparty/harfbuzz)
yup_add_module (thirdparty/sheenbidi)
yup_add_module (thirdparty/rive)
yup_add_module (thirdparty/rive_pls_renderer)

# Original juce modules
yup_add_module (modules/juce_core)
yup_add_module (modules/juce_events)
yup_add_module (modules/juce_audio_basics)
yup_add_module (modules/juce_audio_devices)

# New yup modules
yup_add_module (modules/yup_audio_processors)
yup_add_module (modules/yup_graphics)
yup_add_module (modules/yup_gui)
yup_add_module (modules/yup_audio_plugin_client)

# Targets
if (YUP_BUILD_EXAMPLES)
    message (STATUS "YUP -- Building examples")
    add_subdirectory (examples/app)
    add_subdirectory (examples/console)
    add_subdirectory (examples/graphics)
    add_subdirectory (examples/render)
    if (NOT "${yup_platform}" STREQUAL "emscripten")
        add_subdirectory (examples/plugin)
    endif()
endif()

if (YUP_BUILD_TESTS)
    message (STATUS "YUP -- Building tests")
    add_subdirectory (tests)
endif()
