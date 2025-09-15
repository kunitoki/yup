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
    set (platforms "")

    if (IOS OR CMAKE_SYSTEM_NAME MATCHES "iOS" OR CMAKE_TOOLCHAIN_FILE MATCHES ".*ios\.cmake$")
        set (platform "ios")
        list (APPEND platforms "ios" "apple" "posix" "mobile")

    elseif (ANDROID OR YUP_TARGET_ANDROID)
        set (platform "android")
        list (APPEND platforms "android" "posix" "mobile")

    elseif (EMSCRIPTEN OR CMAKE_TOOLCHAIN_FILE MATCHES ".*Emscripten\.cmake$")
        set (platform "emscripten")
        list (APPEND platforms "emscripten" "posix" "web")

    elseif (APPLE)
        set (platform "mac")
        list (APPEND platforms "mac" "apple" "posix" "desktop")

    elseif (WIN32)
        if (CMAKE_SYSTEM_NAME MATCHES "WindowsStore")
            set (platform "uwp")
            list (APPEND platforms "uwp" "msft" "desktop")
        else()
            set (platform "windows")
            list (APPEND platforms "windows" "msft" "desktop")
        endif()

    elseif (UNIX)
        set (platform "linux")
        list (APPEND platforms "linux" "posix" "desktop")

    else()
        _yup_message (FATAL_ERROR "Invalid unsupported platform")

    endif()

    _yup_message (STATUS "Setting up for ${platform} platform")
    _yup_message (STATUS "Running on cmake ${CMAKE_VERSION}")

    set (YUP_PLATFORM "${platform}" PARENT_SCOPE)

    foreach (platform_name ${platforms})
        string (TOUPPER "${platform_name}" platform_name_upper)
        set (YUP_PLATFORM_${platform_name_upper} ON PARENT_SCOPE)
    endforeach()

endfunction()

#==============================================================================

include (${CMAKE_CURRENT_LIST_DIR}/yup_utilities.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_platforms.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_dependencies.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_modules.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_standalone.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_embed_binary.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_android_java.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_python.cmake)
include (${CMAKE_CURRENT_LIST_DIR}/yup_pluginval.cmake)

#==============================================================================

_yup_setup_platform()
