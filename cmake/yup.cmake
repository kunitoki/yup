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

#==============================================================================

include_guard (GLOBAL)

#==============================================================================

include (FetchContent)

#==============================================================================

function (_yup_message type)
    list (JOIN ARGN "" final_message)
    message (${type} "YUP -- ${final_message}")
endfunction()

#==============================================================================

function (_yup_setup_platform)
    if (IOS OR CMAKE_SYSTEM_NAME MATCHES "iOS" OR CMAKE_TOOLCHAIN_FILE MATCHES ".*ios\.cmake$")
        set (yup_platform "ios")

    elseif (ANDROID OR YUP_TARGET_ANDROID)
        set (yup_platform "android")

    elseif (EMSCRIPTEN OR CMAKE_TOOLCHAIN_FILE MATCHES ".*Emscripten\.cmake$")
        set (yup_platform "emscripten")

    elseif (APPLE)
        set (yup_platform "osx")

    elseif (UNIX)
        set (yup_platform "linux")

    elseif (WIN32)
        if (CMAKE_SYSTEM_NAME MATCHES "WindowsStore")
            set (yup_platform "uwp")
        else()
            set (yup_platform "windows")
        endif()

    else()
        set (yup_platform "unknown")

    endif()

    _yup_message (STATUS "Setting up for ${yup_platform} platform")
    _yup_message (STATUS "Running on cmake ${CMAKE_VERSION}")

    set (yup_platform "${yup_platform}" PARENT_SCOPE)
endfunction()

