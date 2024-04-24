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

yup_add_module (thirdparty/harfbuzz)
yup_add_module (thirdparty/sheenbidi)
yup_add_module (thirdparty/rive)
yup_add_module (thirdparty/rive-pls-renderer)

yup_add_module (modules/juce_core)
yup_add_module (modules/juce_events)
yup_add_module (modules/juce_audio_basics)
yup_add_module (modules/juce_audio_devices)

add_subdirectory (example)
