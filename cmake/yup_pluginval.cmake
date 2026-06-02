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
set (CLAP_VALIDATOR_VERSION "0.3.2")

# ==============================================================================

function (_yup_setup_validation_tool tool_name tool_version tool_platform tool_archive tool_executable tool_url executable_cache_variable)
    if (NOT YUP_PLATFORM_DESKTOP)
        _yup_message (WARNING "${tool_name} is only supported on desktop platforms")
        return()
    endif()

    set (tool_dir "${CMAKE_BINARY_DIR}/${tool_name}")
    set (tool_archive_path "${tool_dir}/${tool_archive}")
    set (tool_executable_path "${tool_dir}/${tool_executable}")

    file (MAKE_DIRECTORY "${tool_dir}")

    if (NOT EXISTS "${tool_executable_path}")
        _yup_message (STATUS "Downloading ${tool_name} ${tool_version} for ${tool_platform}")

        file (DOWNLOAD "${tool_url}" "${tool_archive_path}"
              SHOW_PROGRESS
              STATUS download_status)

        list (GET download_status 0 download_error)
        if (NOT download_error EQUAL 0)
            list (GET download_status 1 download_error_message)
            _yup_message (FATAL_ERROR "Failed to download ${tool_name}: ${download_error_message}")
        endif()

        _yup_message (STATUS "Extracting ${tool_name} archive")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${tool_archive_path}"
            WORKING_DIRECTORY "${tool_dir}"
            RESULT_VARIABLE extract_result)

        if (NOT extract_result EQUAL 0)
            _yup_message (FATAL_ERROR "Failed to extract ${tool_name} archive")
        endif()

        if (YUP_PLATFORM_POSIX)
            execute_process(
                COMMAND chmod +x "${tool_executable_path}"
                RESULT_VARIABLE chmod_result)

            if (NOT chmod_result EQUAL 0)
                _yup_message (WARNING "Failed to make ${tool_name} executable")
            endif()
        endif()

        file (REMOVE "${tool_archive_path}")
    endif()

    if (NOT EXISTS "${tool_executable_path}")
        _yup_message (FATAL_ERROR "${tool_name} executable not found at: ${tool_executable_path}")
    endif()

    set (${executable_cache_variable} "${tool_executable_path}" CACHE INTERNAL "Path to ${tool_name} executable")

    _yup_message (STATUS "${tool_name} is available at: ${tool_executable_path}")
endfunction()

# ==============================================================================

function (yup_setup_pluginval)
    if (NOT YUP_ENABLE_PLUGINVAL)
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

    _yup_setup_validation_tool (
        pluginval
        ${PLUGINVAL_VERSION}
        ${PLUGINVAL_PLATFORM}
        ${PLUGINVAL_ARCHIVE}
        ${PLUGINVAL_EXECUTABLE}
        ${PLUGINVAL_URL}
        PLUGINVAL_EXECUTABLE)
endfunction()

# ==============================================================================

function (yup_setup_clap_validator)
    if (NOT YUP_ENABLE_CLAP_VALIDATOR)
        return()
    endif()

    if (YUP_PLATFORM_WINDOWS)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            set (CLAP_VALIDATOR_PLATFORM "Windows")
            set (CLAP_VALIDATOR_ARCHIVE "clap-validator-${CLAP_VALIDATOR_VERSION}-windows.zip")
        else()
            _yup_message (WARNING "clap-validator does not support 32-bit Windows")
            return()
        endif()
        set (CLAP_VALIDATOR_EXECUTABLE "clap-validator.exe")
    elseif (YUP_PLATFORM_MAC)
        set (CLAP_VALIDATOR_PLATFORM "macOS")
        set (CLAP_VALIDATOR_ARCHIVE "clap-validator-${CLAP_VALIDATOR_VERSION}-macos-universal.tar.gz")
        set (CLAP_VALIDATOR_EXECUTABLE "binaries/clap-validator")
    elseif (YUP_PLATFORM_LINUX)
        set (CLAP_VALIDATOR_PLATFORM "Linux")
        set (CLAP_VALIDATOR_ARCHIVE "clap-validator-${CLAP_VALIDATOR_VERSION}-ubuntu-18.04.tar.gz")
        set (CLAP_VALIDATOR_EXECUTABLE "clap-validator")
    else()
        _yup_message (WARNING "Unsupported platform for clap-validator")
        return()
    endif()

    set (CLAP_VALIDATOR_URL "https://github.com/free-audio/clap-validator/releases/download/${CLAP_VALIDATOR_VERSION}/${CLAP_VALIDATOR_ARCHIVE}")

    _yup_setup_validation_tool (
        clap-validator
        ${CLAP_VALIDATOR_VERSION}
        ${CLAP_VALIDATOR_PLATFORM}
        ${CLAP_VALIDATOR_ARCHIVE}
        ${CLAP_VALIDATOR_EXECUTABLE}
        ${CLAP_VALIDATOR_URL}
        CLAP_VALIDATOR_EXECUTABLE)
endfunction()

# ==============================================================================

function (yup_validate_pluginval target_name plugin_path)
    if (NOT YUP_ENABLE_PLUGINVAL OR NOT PLUGINVAL_EXECUTABLE)
        return()
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[PLUGINVAL] Starting validation of ${target_name}..."
        COMMAND
            "${PLUGINVAL_EXECUTABLE}"
            --strictness-level 5
            --validate-in-process
            --skip-gui-tests
            --validate "${plugin_path}"
        COMMAND ${CMAKE_COMMAND} -E echo "[PLUGINVAL] Validation of ${target_name} completed"
        COMMENT "Running pluginval validation on ${target_name}"
        VERBATIM)
endfunction()

# ==============================================================================

function (yup_validate_smtg_vst3_plugin target_name plugin_path)
    if (NOT YUP_ENABLE_VST3_VALIDATOR)
        return()
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[SMTG] Validator started..."
        COMMAND
            $<TARGET_FILE:validator>
            "${plugin_path}"
            WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E echo "[SMTG] Validator finished."
        COMMENT "Running SMTG VST3 validation on ${target_name}"
        VERBATIM)
endfunction()

# ==============================================================================

function (yup_validate_clap_plugin target_name plugin_path)
    if (NOT YUP_ENABLE_CLAP_VALIDATOR OR NOT CLAP_VALIDATOR_EXECUTABLE)
        return()
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[CLAP-VALIDATOR] Starting validation of ${target_name}..."
        COMMAND "${CLAP_VALIDATOR_EXECUTABLE}" validate "${plugin_path}"
        COMMAND ${CMAKE_COMMAND} -E echo "[CLAP-VALIDATOR] Validation of ${target_name} completed"
        COMMENT "Running clap-validator validation on ${target_name}"
        VERBATIM)
endfunction()

# ==============================================================================

function (yup_validate_au_plugin target_name plugin_name au_type au_subtype au_manufacturer)
    if (NOT YUP_ENABLE_AUVAL_VALIDATOR OR NOT YUP_PLATFORM_MAC)
        return()
    endif()

    find_program (AUVAL_EXECUTABLE auval)

    if (AUVAL_EXECUTABLE)
        add_custom_command (TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "[AUVAL] Validating ${plugin_name}..."
            COMMAND "${AUVAL_EXECUTABLE}" -strict -v
                "${au_type}"
                "${au_subtype}"
                "${au_manufacturer}"
            COMMAND ${CMAKE_COMMAND} -E echo "[AUVAL] Validation of ${plugin_name} completed"
            COMMENT "Running auval validation on ${target_name}"
            VERBATIM)
    else()
        _yup_message (WARNING "auval not found; skipping AU validation")
    endif()
endfunction()
