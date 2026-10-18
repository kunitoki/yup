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
    set (tool_build_dir "${CMAKE_BINARY_DIR}/_yup_host_tools/shader_bundler")

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
        _yup_message (FATAL_ERROR "Failed to locate built shader bundler tool in ${tool_build_dir}")
    endif()

    list (GET candidate_exes 0 tool_exe)

    file (SHA256 "${tool_exe}" tool_hash)
    set_property (GLOBAL PROPERTY YUP_SHADER_BUNDLER_EXECUTABLE "${tool_exe}")
    set_property (GLOBAL PROPERTY YUP_SHADER_BUNDLER_SHA256 "${tool_hash}")
    _yup_message (STATUS " * shader bundler executable: ${tool_exe}")

    set (${output_variable} "${tool_exe}" PARENT_SCOPE)
endfunction()

#==============================================================================
# Compiles a vertex/fragment GLSL shader pair, or a compute shader, into a .ysl
# bundle (at configure time) and embeds it into an OBJECT library that can be
# linked into a target. The bundle is only regenerated when the tool, the
# arguments or the content of a stage or DEPENDS file changed.
#
# Usage:
#   yup_add_shader_bundle (<library_name>
#       VERT           <path to .vert file>
#       FRAG           <path to .frag file>
#       | COMPUTE      <path to .comp file>
#       [OUTPUT_NAME   <basename>]      # default: <library_name>
#       [RESOURCE_NAME <symbol>]        # default: <library_name>
#       [NAMESPACE     <namespace>]     # default: yup
#       [ENTRY         <entry point>]   # default: main
#       [GLSL_VERSION  <version>]       # default: 450
#       [BUNDLE_RESOURCE <variable>]    # don't embed, see below
#       [BUNDLE_DESTINATION <path>]     # default: <OUTPUT_NAME>.ysl
#       [TEMP_FOLDER   <path>]          # default: CMAKE_CURRENT_BINARY_DIR
#       [DEPENDS       <file>...]       # extra inputs, e.g. #included files
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
#
# With BUNDLE_RESOURCE no library is created: the .ysl is left in
# TEMP_FOLDER and <variable> is set to "<ysl path>@<BUNDLE_DESTINATION>",
# ready to be passed to the BUNDLE_RESOURCES of yup_standalone_app. Load it at
# runtime with ShaderBundle::loadFromFile().

function (yup_add_shader_bundle library_name)
    set (options "")
    set (one_value_args VERT FRAG COMPUTE OUTPUT_NAME RESOURCE_NAME NAMESPACE ENTRY GLSL_VERSION BUNDLE_RESOURCE BUNDLE_DESTINATION TEMP_FOLDER)
    set (multi_value_args OPTIONS DEPENDS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (YUP_ARG_COMPUTE AND (YUP_ARG_VERT OR YUP_ARG_FRAG))
        _yup_message (FATAL_ERROR "yup_add_shader_bundle: COMPUTE can't be combined with VERT or FRAG")
    endif()
    if (NOT YUP_ARG_COMPUTE AND NOT (YUP_ARG_VERT AND YUP_ARG_FRAG))
        _yup_message (FATAL_ERROR "yup_add_shader_bundle: either VERT and FRAG, or COMPUTE, are required")
    endif()

    _yup_set_default (YUP_ARG_OUTPUT_NAME "${library_name}")
    _yup_set_default (YUP_ARG_RESOURCE_NAME "${library_name}")
    _yup_set_default (YUP_ARG_NAMESPACE "yup")
    _yup_set_default (YUP_ARG_ENTRY "main")
    _yup_set_default (YUP_ARG_GLSL_VERSION "450")
    _yup_set_default (YUP_ARG_BUNDLE_DESTINATION "${YUP_ARG_OUTPUT_NAME}.ysl")
    _yup_set_default (YUP_ARG_TEMP_FOLDER "${CMAKE_CURRENT_BINARY_DIR}")

    set (stage_args "")
    set (stage_paths "")
    set (stage_arg_names VERT FRAG COMPUTE)
    set (stage_names vertex fragment compute)
    foreach (stage_arg stage IN ZIP_LISTS stage_arg_names stage_names)
        if (NOT YUP_ARG_${stage_arg})
            continue()
        endif()

        get_filename_component (stage_path "${YUP_ARG_${stage_arg}}" ABSOLUTE)
        if (NOT EXISTS "${stage_path}")
            _yup_message (FATAL_ERROR "yup_add_shader_bundle: ${stage} shader not found: ${stage_path}")
        endif()

        list (APPEND stage_args --stage ${stage} "${stage_path}")
        list (APPEND stage_paths "${stage_path}")
    endforeach()

    set (depend_paths "")
    foreach (depend IN LISTS YUP_ARG_DEPENDS)
        get_filename_component (depend_path "${depend}" ABSOLUTE)
        if (NOT EXISTS "${depend_path}")
            _yup_message (FATAL_ERROR "yup_add_shader_bundle: dependency not found: ${depend_path}")
        endif()

        list (APPEND depend_paths "${depend_path}")
    endforeach()

    # ==== Re-run the configure step when an input changes
    set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${stage_paths} ${depend_paths})

    # ==== Ensure the host tool is available (built and cached once)
    _yup_build_shader_bundler_tool (shader_bundler_exe)

    # ==== Generate the .ysl bundle at configure time, unless tool, arguments and inputs are unchanged
    set (bundle_path "${YUP_ARG_TEMP_FOLDER}/${YUP_ARG_OUTPUT_NAME}.ysl")
    set (bundle_key_path "${bundle_path}.sha256")

    set (bundler_command
        "${shader_bundler_exe}"
            ${stage_args}
            --output "${bundle_path}"
            --entry "${YUP_ARG_ENTRY}"
            --glsl-version "${YUP_ARG_GLSL_VERSION}"
            ${YUP_ARG_OPTIONS})

    get_property (bundle_key GLOBAL PROPERTY YUP_SHADER_BUNDLER_SHA256)
    string (APPEND bundle_key "${bundler_command}")
    foreach (input_path IN LISTS stage_paths depend_paths)
        file (SHA256 "${input_path}" input_hash)
        string (APPEND bundle_key "${input_path}=${input_hash}")
    endforeach()
    string (SHA256 bundle_key "${bundle_key}")

    set (previous_bundle_key "")
    if (EXISTS "${bundle_path}" AND EXISTS "${bundle_key_path}")
        file (READ "${bundle_key_path}" previous_bundle_key)
    endif()

    if (bundle_key STREQUAL previous_bundle_key)
        _yup_message (STATUS "Shader bundle ${bundle_path} is up to date")
    else()
        _yup_message (STATUS "Generating shader bundle ${bundle_path}")
        file (REMOVE "${bundle_key_path}")
        _yup_execute_process_or_fail ("${CMAKE_COMMAND}" "-E" "make_directory" "${YUP_ARG_TEMP_FOLDER}")
        _yup_execute_process_or_fail (${bundler_command})
        file (WRITE "${bundle_key_path}" "${bundle_key}")
    endif()

    # ==== Hand the bundle over to BUNDLE_RESOURCES instead of embedding it
    if (YUP_ARG_BUNDLE_RESOURCE)
        set (${YUP_ARG_BUNDLE_RESOURCE} "${bundle_path}@${YUP_ARG_BUNDLE_DESTINATION}" PARENT_SCOPE)
        return()
    endif()

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

#==============================================================================
# Adds the target that builds the host yup_shader_bundler at build time, once
# per build tree, and returns the path of the executable it produces.
#
# Unlike _yup_build_shader_bundler_tool, configuring a project never builds the
# tool: only building a target that needs a shader bundle does.

function (_yup_add_shader_bundler_host_target output_executable output_target)
    set (tool_target yup_shader_bundler_host)
    set (tool_executable "${CMAKE_BINARY_DIR}/_yup_host_tools/bin/yup_shader_bundler")
    set (tool_executable_name "yup_shader_bundler")
    if (CMAKE_HOST_WIN32)
        string (APPEND tool_executable ".exe")
        string (APPEND tool_executable_name ".exe")
    endif()

    if (NOT TARGET ${tool_target})
        get_filename_component (tool_source_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/shader_bundler" ABSOLUTE)

        add_custom_target (${tool_target}
            COMMAND "${CMAKE_COMMAND}"
                -DSOURCE_DIR=${tool_source_dir}
                -DBUILD_DIR=${CMAKE_BINARY_DIR}/_yup_host_tools/shader_bundler
                -DEXECUTABLE_NAME=${tool_executable_name}
                -DOUTPUT=${tool_executable}
                -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/yup_build_host_tool.cmake"
            BYPRODUCTS "${tool_executable}"
            COMMENT "Building host yup_shader_bundler"
            VERBATIM)

        set_target_properties (${tool_target} PROPERTIES FOLDER "Tools")
    endif()

    set (${output_executable} "${tool_executable}" PARENT_SCOPE)
    set (${output_target} "${tool_target}" PARENT_SCOPE)
endfunction()

#==============================================================================
# Builds the shader bundles a module declares, at build time, for yup_add_module.
#
# Module declaration keys:
#   shaderDirectory:    <dir>       Folder of the stage sources, relative to the module
#   shaderIncludes:     <dir>...    Folders the sources #include from, relative to the module
#   shaderDefinePrefix: <prefix>    Prepended to every define of shaderBundles
#   shaderBundles:      <entry>, ...
#
# An entry is a shader name followed by defines, separated by spaces; entries
# are separated by commas or by continuation lines. A define ending in ? is
# optional: the entry expands into every combination of its optional defines.
#
#   shaderBundles:      ForwardPass AREA_LIGHTS? SOFT_SHADOWS?
#                       ShadowPass MASKED?
#
# The entry Name bundles yup_<Name>.vert (or yup_Fullscreen.vert for a
# fragment-only pass), yup_<Name>.frag and yup_<Name>.comp, whichever exist,
# into <module>_shaders/yup_<Name>[_<define>...].inc (defines in lowercase,
# without the prefix) under the build tree, as a C array initializer:
#
#   constexpr uint8_t forwardPassBundle[] = {
#   #include "yup_renderer_shaders/yup_ForwardPass_area_lights.inc"
#   };

function (_yup_module_add_shader_bundles module_name module_path shader_directory shader_includes shader_define_prefix shader_bundles)
    if (NOT shader_directory)
        _yup_message (FATAL_ERROR "Module ${module_name} declares shaderBundles without a shaderDirectory")
    endif()

    get_filename_component (shader_dir "${shader_directory}" ABSOLUTE BASE_DIR "${module_path}")
    if (NOT IS_DIRECTORY "${shader_dir}")
        _yup_message (FATAL_ERROR "Module ${module_name} shaderDirectory not found: ${shader_dir}")
    endif()

    set (include_args "-I${shader_dir}")
    set (include_files "")
    foreach (include_directory IN LISTS shader_includes)
        get_filename_component (include_dir "${include_directory}" ABSOLUTE BASE_DIR "${module_path}")
        list (APPEND include_args "-I${include_dir}")
        file (GLOB_RECURSE directory_files CONFIGURE_DEPENDS "${include_dir}/*")
        list (APPEND include_files ${directory_files})
    endforeach()

    _yup_add_shader_bundler_host_target (bundler_executable bundler_target)

    set (output_root "${CMAKE_BINARY_DIR}/_yup_shader_bundles")
    set (output_dir "${output_root}/${module_name}_shaders")
    file (MAKE_DIRECTORY "${output_dir}")

    set (bundle_bases "")
    set (bundle_outputs "")

    string (REPLACE "," ";" bundle_entries "${shader_bundles}")
    foreach (bundle_entry IN LISTS bundle_entries)
        string (STRIP "${bundle_entry}" bundle_entry)
        if ("${bundle_entry}" STREQUAL "")
            continue()
        endif()

        string (REGEX REPLACE "[ \t]+" ";" entry_tokens "${bundle_entry}")
        list (POP_FRONT entry_tokens shader_name)

        # ==== Stage sources
        set (stage_args "")
        set (stage_paths "")
        if (EXISTS "${shader_dir}/yup_${shader_name}.vert")
            list (APPEND stage_args --vert "${shader_dir}/yup_${shader_name}.vert")
            list (APPEND stage_paths "${shader_dir}/yup_${shader_name}.vert")
        elseif (EXISTS "${shader_dir}/yup_${shader_name}.frag" AND EXISTS "${shader_dir}/yup_Fullscreen.vert")
            list (APPEND stage_args --vert "${shader_dir}/yup_Fullscreen.vert")
            list (APPEND stage_paths "${shader_dir}/yup_Fullscreen.vert")
        endif()
        if (EXISTS "${shader_dir}/yup_${shader_name}.frag")
            list (APPEND stage_args --frag "${shader_dir}/yup_${shader_name}.frag")
            list (APPEND stage_paths "${shader_dir}/yup_${shader_name}.frag")
        endif()
        if (EXISTS "${shader_dir}/yup_${shader_name}.comp")
            list (APPEND stage_args --stage compute "${shader_dir}/yup_${shader_name}.comp")
            list (APPEND stage_paths "${shader_dir}/yup_${shader_name}.comp")
        endif()

        if (NOT stage_paths)
            _yup_message (FATAL_ERROR "Module ${module_name} shader ${shader_name} has no yup_${shader_name}.vert/.frag/.comp in ${shader_dir}")
        endif()

        # ==== Every combination of the optional defines, in declaration order
        set (optional_count 0)
        foreach (token IN LISTS entry_tokens)
            if (token MATCHES "\\?$")
                math (EXPR optional_count "${optional_count} + 1")
            endif()
        endforeach()
        math (EXPR last_mask "(1 << ${optional_count}) - 1")

        foreach (mask RANGE 0 ${last_mask})
            set (bundle_base "yup_${shader_name}")
            set (define_args "")
            set (optional_index 0)

            foreach (token IN LISTS entry_tokens)
                if (token MATCHES "^(.+)\\?$")
                    set (define "${CMAKE_MATCH_1}")
                    math (EXPR enabled "(${mask} >> ${optional_index}) & 1")
                    math (EXPR optional_index "${optional_index} + 1")
                    if (NOT enabled)
                        continue()
                    endif()
                else()
                    set (define "${token}")
                endif()

                string (TOLOWER "${define}" define_suffix)
                string (APPEND bundle_base "_${define_suffix}")
                list (APPEND define_args "-D${shader_define_prefix}${define}")
            endforeach()

            if (bundle_base IN_LIST bundle_bases)
                continue()
            endif()
            list (APPEND bundle_bases "${bundle_base}")

            set (bundle_output "${output_dir}/${bundle_base}.inc")
            add_custom_command (
                OUTPUT "${bundle_output}"
                COMMAND "${bundler_executable}" ${stage_args} ${define_args} ${include_args} --output-inc "${bundle_output}"
                DEPENDS "${bundler_executable}" ${stage_paths} ${include_files}
                COMMENT "Building shader bundle ${module_name}_shaders/${bundle_base}.inc"
                VERBATIM)

            list (APPEND bundle_outputs "${bundle_output}")
        endforeach()
    endforeach()

    list (LENGTH bundle_outputs bundle_count)
    _yup_message (STATUS " * ${module_name}: ${bundle_count} shader bundles built in ${output_dir}")

    # ==== Consumers compile the module sources: they build the bundles first, and find them
    set (bundles_target ${module_name}_shader_bundles)
    add_custom_target (${bundles_target} DEPENDS ${bundle_outputs})
    add_dependencies (${bundles_target} ${bundler_target})
    set_target_properties (${bundles_target} PROPERTIES FOLDER "ShaderBundles")

    add_dependencies (${module_name} ${bundles_target})
    target_include_directories (${module_name} INTERFACE "${output_root}")
endfunction()
