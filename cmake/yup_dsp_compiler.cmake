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

function (_yup_build_dsp_compiler_tool output_variable)
    get_property (cached_exe GLOBAL PROPERTY YUP_DSP_COMPILER_EXECUTABLE)
    if (cached_exe AND EXISTS "${cached_exe}")
        set (${output_variable} "${cached_exe}" PARENT_SCOPE)
        return()
    endif()

    set (tool_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/ydsp_compiler")
    set (tool_build_dir "${CMAKE_BINARY_DIR}/_host_tools/ydsp_compiler")
    _yup_execute_process_or_fail ("${CMAKE_COMMAND}" -S "${tool_source_dir}" -B "${tool_build_dir}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=)
    _yup_execute_process_or_fail ("${CMAKE_COMMAND}" --build "${tool_build_dir}" --config Release --parallel 4)

    set (exe_name yup_dsp_compiler)
    if (CMAKE_HOST_WIN32)
        set (exe_name yup_dsp_compiler.exe)
    endif()

    file (GLOB_RECURSE candidates "${tool_build_dir}/${exe_name}")
    list (GET candidates 0 tool_exe)
    set_property (GLOBAL PROPERTY YUP_DSP_COMPILER_EXECUTABLE "${tool_exe}")
    set (${output_variable} "${tool_exe}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (yup_add_ydsp_bundle library_name)
    set (one_value_args SOURCE OUTPUT_NAME RESOURCE_NAME NAMESPACE)
    set (multi_value_args TARGETS OPTIONS)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if (NOT YUP_ARG_SOURCE)
        message (FATAL_ERROR "yup_add_ydsp_bundle: SOURCE argument is required")
    endif()

    _yup_set_default (YUP_ARG_OUTPUT_NAME "${library_name}")
    _yup_set_default (YUP_ARG_RESOURCE_NAME "${library_name}")
    _yup_set_default (YUP_ARG_NAMESPACE yup)
    _yup_build_dsp_compiler_tool (compiler_exe)
    get_filename_component (source_path "${YUP_ARG_SOURCE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    set (bundle_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUTPUT_NAME}.ydsb")
    if (NOT EXISTS "${source_path}")
        message (FATAL_ERROR "yup_add_ydsp_bundle: source file not found: ${source_path}")
    endif()
    set (target_arguments)
    foreach (target IN LISTS YUP_ARG_TARGETS)
        list (APPEND target_arguments --target "${target}")
    endforeach()
    _yup_execute_process_or_fail ("${compiler_exe}" "${source_path}" --output "${bundle_path}" ${target_arguments} ${YUP_ARG_OPTIONS})

    yup_add_embedded_binary_resources (${library_name}
        OUT_DIR YdspBundles
        HEADER "${YUP_ARG_OUTPUT_NAME}.h"
        NAMESPACE ${YUP_ARG_NAMESPACE}
        RESOURCE_NAMES ${YUP_ARG_RESOURCE_NAME}
        RESOURCES "${bundle_path}")
endfunction()
