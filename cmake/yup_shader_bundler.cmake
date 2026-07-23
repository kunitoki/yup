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
# Builds the yup_shader_bundler console tool for the host once and caches the
# resulting executable path in the global property YUP_SHADER_BUNDLER_EXECUTABLE.
#
# The tool is built in its own binary tree using the host compiler (no toolchain
# file is forwarded), so it is runnable at configure time even when the outer
# build is cross-compiling (Android, iOS, WebAssembly).

function (_yup_build_shader_bundler_tool output_variable)
    get_property (cached_exe GLOBAL PROPERTY YUP_SHADER_BUNDLER_EXECUTABLE)
    if (cached_exe AND EXISTS "${cached_exe}")
        set (${output_variable} "${cached_exe}" PARENT_SCOPE)
        return()
    endif()

    get_filename_component (tool_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/shader_bundler" ABSOLUTE)
    set (tool_build_dir "${CMAKE_BINARY_DIR}/_host_tools/shader_bundler")

    # ==== Configure the host build (no toolchain file -> host compiler)
    _yup_message (STATUS "Configuring host shader bundler tool")
    _yup_message (STATUS " * tool_source_dir: ${tool_source_dir}")
    _yup_message (STATUS " * tool_build_dir: ${tool_build_dir}")
    _yup_execute_process_or_fail (
        "${CMAKE_COMMAND}"
            -S "${tool_source_dir}"
            -B "${tool_build_dir}"
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_TOOLCHAIN_FILE=)

    # ==== Build it
    _yup_message (STATUS "Building host shader bundler tool")
    _yup_execute_process_or_fail (
        "${CMAKE_COMMAND}"
            --build "${tool_build_dir}"
            --config Release
            --parallel 4)

    # ==== Locate the produced executable
    set (exe_name "yup_shader_bundler")
    if (CMAKE_HOST_WIN32)
        set (exe_name "yup_shader_bundler.exe")
    endif()

    file (GLOB_RECURSE candidate_exes "${tool_build_dir}/${exe_name}")
    list (FILTER candidate_exes EXCLUDE REGEX "\\.dSYM/")
    list (LENGTH candidate_exes num_candidates)

    if (num_candidates EQUAL 0)
        message (FATAL_ERROR "Failed to locate built shader bundler tool in ${tool_build_dir}")
    endif()

    list (GET candidate_exes 0 tool_exe)

    set_property (GLOBAL PROPERTY YUP_SHADER_BUNDLER_EXECUTABLE "${tool_exe}")
    _yup_message (STATUS " * shader bundler executable: ${tool_exe}")

    set (${output_variable} "${tool_exe}" PARENT_SCOPE)
endfunction()

#==============================================================================
# Compiles a vertex/fragment GLSL shader pair into a .ysl bundle (at configure
# time) and embeds it into an OBJECT library that can be linked into a target.
#
# Usage:
#   yup_add_shader_bundle (<library_name>
#       VERT           <path to .vert file>
#       FRAG           <path to .frag file>
#       [OUTPUT_NAME   <basename>]      # default: <library_name>
#       [RESOURCE_NAME <symbol>]        # default: <library_name>
#       [NAMESPACE     <namespace>]     # default: yup
#       [ENTRY         <entry point>]   # default: main
#       [GLSL_VERSION  <version>]       # default: 450
#       [OPTIONS       <flag>...])      # extra flags forwarded to yup_shader_bundler
#
# OPTIONS forwards any additional yup_shader_bundler flags verbatim, e.g.
#   OPTIONS --spirv-opt perf --target-langs msl,hlsl -DMY_DEFINE=1 -I${CMAKE_SOURCE_DIR}/shaders
#
# After the call, link against <library_name> and include the generated header
# "<OUTPUT_NAME>.h", which exposes:
#     extern const uint8_t  <RESOURCE_NAME>_data[];
#     extern const size_t   <RESOURCE_NAME>_size;
# The bytes can be loaded at runtime with ShaderBundle::loadFromData().

function (yup_add_shader_bundle library_name)
    set (options "")
    set (one_value_args VERT FRAG OUTPUT_NAME RESOURCE_NAME NAMESPACE ENTRY GLSL_VERSION)
    set (multi_value_args OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT YUP_ARG_VERT)
        message (FATAL_ERROR "yup_add_shader_bundle: VERT argument is required")
    endif()
    if (NOT YUP_ARG_FRAG)
        message (FATAL_ERROR "yup_add_shader_bundle: FRAG argument is required")
    endif()

    _yup_set_default (YUP_ARG_OUTPUT_NAME "${library_name}")
    _yup_set_default (YUP_ARG_RESOURCE_NAME "${library_name}")
    _yup_set_default (YUP_ARG_NAMESPACE "yup")
    _yup_set_default (YUP_ARG_ENTRY "main")
    _yup_set_default (YUP_ARG_GLSL_VERSION "450")

    get_filename_component (vert_path "${YUP_ARG_VERT}" ABSOLUTE)
    get_filename_component (frag_path "${YUP_ARG_FRAG}" ABSOLUTE)

    if (NOT EXISTS "${vert_path}")
        message (FATAL_ERROR "yup_add_shader_bundle: vertex shader not found: ${vert_path}")
    endif()
    if (NOT EXISTS "${frag_path}")
        message (FATAL_ERROR "yup_add_shader_bundle: fragment shader not found: ${frag_path}")
    endif()

    # ==== Ensure the host tool is available (built and cached once)
    _yup_build_shader_bundler_tool (shader_bundler_exe)

    # ==== Generate the .ysl bundle at configure time
    set (bundle_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUTPUT_NAME}.ysl")

    _yup_message (STATUS "Generating shader bundle ${bundle_path}")
    _yup_execute_process_or_fail (
        "${shader_bundler_exe}"
            --vert "${vert_path}"
            --frag "${frag_path}"
            --output "${bundle_path}"
            --entry "${YUP_ARG_ENTRY}"
            --glsl-version "${YUP_ARG_GLSL_VERSION}"
            ${YUP_ARG_OPTIONS})

    # ==== Embed the generated bundle into an object library
    yup_add_embedded_binary_resources (
        ${library_name}
        OUT_DIR ShaderBundles
        HEADER "${YUP_ARG_OUTPUT_NAME}.h"
        NAMESPACE ${YUP_ARG_NAMESPACE}
        RESOURCE_NAMES
            ${YUP_ARG_RESOURCE_NAME}
        RESOURCES
            "${bundle_path}")
endfunction()
