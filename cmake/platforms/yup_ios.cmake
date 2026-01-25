# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
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
# iOS Platform Configuration
#==============================================================================

# Handle iOS simulator builds when not using the full YUP iOS toolchain
# This ensures the correct SDK is used even when using FetchContent
if (DEFINED PLATFORM AND PLATFORM MATCHES "^SIMULATOR")
    # Determine the correct SDK name based on platform
    set (SDK_NAME "iphonesimulator")

    # Query xcodebuild for the simulator SDK path if sysroot is not set correctly
    if (NOT DEFINED CMAKE_OSX_SYSROOT OR CMAKE_OSX_SYSROOT STREQUAL "" OR CMAKE_OSX_SYSROOT MATCHES "iphoneos")
        find_program (XCODEBUILD_EXECUTABLE xcodebuild)
        if (XCODEBUILD_EXECUTABLE)
            execute_process (
                COMMAND ${XCODEBUILD_EXECUTABLE} -version -sdk ${SDK_NAME} Path
                OUTPUT_VARIABLE SIMULATOR_SDK_PATH
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if (SIMULATOR_SDK_PATH)
                set (CMAKE_OSX_SYSROOT "${SIMULATOR_SDK_PATH}" CACHE PATH "iOS Simulator SDK path" FORCE)
                _yup_message (STATUS "Using iOS Simulator SDK: ${SIMULATOR_SDK_PATH}")
            endif()
        endif()
    endif()
endif()
