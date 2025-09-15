# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2025 - kunitoki@gmail.com
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

# ==============================================================================

set (PLUGINVAL_VERSION "v1.0.4")

# ==============================================================================

function (yup_setup_pluginval)
    if (NOT YUP_ENABLE_PLUGINVAL)
        return()
    endif()

    # Only supported on desktop platforms
    if (NOT YUP_PLATFORM_DESKTOP)
        _yup_message (WARNING "pluginval is only supported on desktop platforms")
        return()
    endif()

    # Determine platform-specific download URL and executable name
    if (YUP_PLATFORM_WINDOWS)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (PLUGINVAL_PLATFORM "Windows")
            set (PLUGINVAL_ARCHIVE "pluginval_Windows.zip")
        else()
            _yup_message (WARNING "pluginval does not support 32-bit Windows")
            return()
        endif()
        set (PLUGINVAL_EXECUTABLE "pluginval.exe")
    elseif (YUP_PLATFORM_MAC)
        set (PLUGINVAL_PLATFORM "macOS")
        set (PLUGINVAL_ARCHIVE "pluginval_macOS.zip")
        set (PLUGINVAL_EXECUTABLE "pluginval.app/Contents/MacOS/pluginval")
    elseif (YUP_PLATFORM_LINUX)
        set (PLUGINVAL_PLATFORM "Linux")
        set (PLUGINVAL_ARCHIVE "pluginval_Linux.zip")
        set (PLUGINVAL_EXECUTABLE "pluginval")
    else()
        _yup_message (WARNING "Unsupported platform for pluginval")
        return()
    endif()

    # Set up download URL
    set (PLUGINVAL_URL "https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/${PLUGINVAL_ARCHIVE}")

    # Set up local paths
    set (PLUGINVAL_DIR "${CMAKE_BINARY_DIR}/pluginval")
    set (PLUGINVAL_ARCHIVE_PATH "${PLUGINVAL_DIR}/${PLUGINVAL_ARCHIVE}")
    set (PLUGINVAL_EXECUTABLE_PATH "${PLUGINVAL_DIR}/${PLUGINVAL_EXECUTABLE}")

    # Create pluginval directory
    file (MAKE_DIRECTORY "${PLUGINVAL_DIR}")

    # Download pluginval if not already present
    if (NOT EXISTS "${PLUGINVAL_EXECUTABLE_PATH}")
        _yup_message (STATUS "Downloading pluginval ${PLUGINVAL_VERSION} for ${PLUGINVAL_PLATFORM}")

        # Download the archive
        file (DOWNLOAD "${PLUGINVAL_URL}" "${PLUGINVAL_ARCHIVE_PATH}"
              SHOW_PROGRESS
              STATUS DOWNLOAD_STATUS)

        # Check if download was successful
        list (GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR)
        if (NOT DOWNLOAD_ERROR EQUAL 0)
            list (GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MESSAGE)
            _yup_message (FATAL_ERROR "Failed to download pluginval: ${DOWNLOAD_ERROR_MESSAGE}")
        endif()

        # Extract the archive
        _yup_message (STATUS "Extracting pluginval archive")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${PLUGINVAL_ARCHIVE_PATH}"
            WORKING_DIRECTORY "${PLUGINVAL_DIR}"
            RESULT_VARIABLE EXTRACT_RESULT)

        if (NOT EXTRACT_RESULT EQUAL 0)
            _yup_message (FATAL_ERROR "Failed to extract pluginval archive")
        endif()

        # Make executable (Posix platforms)
        if (YUP_PLATFORM_POSIX)
            execute_process(
                COMMAND chmod +x "${PLUGINVAL_EXECUTABLE_PATH}"
                RESULT_VARIABLE CHMOD_RESULT)

            if (NOT CHMOD_RESULT EQUAL 0)
                _yup_message (WARNING "Failed to make pluginval executable")
            endif()
        endif()

        # Clean up archive
        file (REMOVE "${PLUGINVAL_ARCHIVE_PATH}")
    endif()

    # Verify pluginval executable exists
    if (NOT EXISTS "${PLUGINVAL_EXECUTABLE_PATH}")
        _yup_message (FATAL_ERROR "pluginval executable not found at: ${PLUGINVAL_EXECUTABLE_PATH}")
    endif()

    # Set global variable for use in other cmake files
    set (PLUGINVAL_EXECUTABLE "${PLUGINVAL_EXECUTABLE_PATH}" CACHE INTERNAL "Path to pluginval executable")

    _yup_message (STATUS "pluginval is available at: ${PLUGINVAL_EXECUTABLE_PATH}")
endfunction()

# ==============================================================================

function (yup_validate_plugin target_name plugin_path)
    if (NOT YUP_ENABLE_PLUGINVAL OR NOT PLUGINVAL_EXECUTABLE)
        return()
    endif()

    set (validation_target_name "${target_name}_pluginval")

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[PLUGINVAL] Starting validation of ${target_name}..."
        COMMAND "${PLUGINVAL_EXECUTABLE}" --strictness-level 5 --validate-in-process --output-dir "${CMAKE_BINARY_DIR}/pluginval_reports" "${plugin_path}"
        COMMAND ${CMAKE_COMMAND} -E echo "[PLUGINVAL] Validation of ${target_name} completed"
        COMMENT "Running pluginval validation on ${target_name}"
        VERBATIM)
endfunction()
