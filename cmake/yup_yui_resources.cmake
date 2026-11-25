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
# Embeds .yui files and the files they read into an OBJECT library, through
# yup_add_embedded_binary_resources, for YuiLoader::loadResource.
#
# Usage:
#   yup_add_yui_resources (<library_name>
#       [BASE_DIR         <dir>]        # default: CMAKE_CURRENT_SOURCE_DIR
#       [NAMESPACE        <namespace>]  # default: yup
#       [VERIFY]                        # check FILES when configuring
#       [VERIFY_MODELS    <file>]       # with VERIFY, the models FILES bind to
#       FILES             <file>...     # .yui files
#       [ASSETS           <file>...]    # files embedded as they are
#       [SHADERS          <file>...]    # fragment shaders of `shader` blocks
#       [MATERIAL_SHADERS <file>...])   # material shaders of `shader` blocks
#
# The generated <library_name>.h declares, in NAMESPACE:
#
#   const uint8_t* <c_identifier>_find (const char* path, std::size_t& size);
#
# where <c_identifier> is MAKE_C_IDENTIFIER applied to <library_name>. It
# returns the bytes embedded under a path, or nullptr: a YuiLoader finds with
# it the texts of a document and the files the document reads. A file is
# embedded under its "/" normalized path relative to BASE_DIR, as written by
# `import` and by the properties naming a file. Every file has to be inside
# BASE_DIR and is listed once. Names are compared without their case for that,
# and two files can't differ by it alone: the file systems of macOS and Windows
# don't tell them apart, nor what is generated for them.
#
# FILES and ASSETS are embedded when configuring, and editing one configures
# again. ASSETS are the files a document reads besides its texts, such as
# models, textures and compiled shader bundles.
#
# SHADERS are the GLSL fragment stages `shader` blocks name with `fragment`.
# Each is compiled at build time by yup_shader_bundler, behind the vertex stage
# of GpuImageShader, and its bundle is embedded as "<path>.ysl": the file a
# document reads before the source, which isn't embedded.
#
# MATERIAL_SHADERS are the GLSL materials `shader` blocks name with `material`.
# Each is compiled at build time inside the material template of yup_renderer,
# read from the modules beside this file, once per tier of the renderer: its
# bundles are embedded as "<path>.raster.ysl" and "<path>.full.ysl", and the
# source isn't. A material is one file besides the library of the renderer
# (`lib/...`): the GLSL files of its folder are on the include path here, and
# editing one builds it again, but they are not where a material is compiled
# from its source (a development build, yup_yui_tool, VERIFY).
#
# Editing a shader only builds its bundles again, without configuring, unless
# VERIFY is given.
#
# VERIFY checks FILES with `yup_yui_tool verify` when configuring, which fails
# with the errors found. The tool loads them as the library serves them: it
# finds the files listed here and no other, so a file a document reads that is
# not listed fails the configuration, instead of being missing when the
# application runs. It compiles the SHADERS and MATERIAL_SHADERS they name from
# their sources, with the shader transpiler it is built with: one that does
# not compile fails as well, and editing one configures again. The tool is
# built for the host the first time, into the build tree, so the option costs a
# build of yup_gui, of yup_gui_markup, of yup_renderer, of yup_audio_gui and of that transpiler. VERIFY_MODELS is
# the JSON file declaring the models the files bind to (see `--models`).

function (_yup_build_yui_tool output_variable)
    get_property (tool_executable GLOBAL PROPERTY YUP_YUI_TOOL_EXECUTABLE)

    if (NOT tool_executable)
        set (tool_executable_name "yup_yui_tool")
        if (CMAKE_HOST_WIN32)
            string (APPEND tool_executable_name ".exe")
        endif()

        set (tool_executable "${CMAKE_BINARY_DIR}/_yup_host_tools/bin/${tool_executable_name}")

        _yup_message (STATUS "Building host yup_yui_tool")
        _yup_execute_process_or_fail ("${CMAKE_COMMAND}"
            "-DSOURCE_DIR=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/yui_tool"
            "-DBUILD_DIR=${CMAKE_BINARY_DIR}/_yup_host_tools/yui_tool"
            "-DEXECUTABLE_NAME=${tool_executable_name}"
            "-DOUTPUT=${tool_executable}"
            -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tools/yup_build_host_tool.cmake")

        set_property (GLOBAL PROPERTY YUP_YUI_TOOL_EXECUTABLE "${tool_executable}")
    endif()

    set (${output_variable} "${tool_executable}" PARENT_SCOPE)
endfunction()

#==============================================================================
# Adds the command compiling a shader of yup_add_yui_resources at build time
# into <bundle_path>.inc, the byte array of the .ysl bundle embedded under
# <embedded_path>. <argument> is the one the shader was given to: SHADERS pairs
# the fragment stage with the vertex stage of GpuImageShader, MATERIAL_SHADERS
# compiles the material inside the template of yup_renderer, for the full tier
# when the bundle is its ".full.ysl" one.

function (_yup_add_yui_shader_bundle argument shader_path bundle_path embedded_path)
    _yup_add_shader_bundler_host_target (bundler_executable bundler_target)
    get_filename_component (modules_path "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../modules" ABSOLUTE)

    # ==== The tool runs in a folder of the bundle, where it finds nothing to include but what it is given
    file (MAKE_DIRECTORY "${bundle_path}")

    if (argument STREQUAL "SHADERS")
        set (vertex_path "${modules_path}/yup_rhi/shaders/yup_GpuImageShader.vert")
        set (bundler_args --vert "${vertex_path}" --frag "${shader_path}")
        set (depend_paths "${vertex_path}")
    else()
        set (shader_dir "${modules_path}/yup_renderer/shaders")
        cmake_path (GET shader_path FILENAME material_name)
        cmake_path (GET shader_path PARENT_PATH material_dir)

        # ==== The template includes the material under a fixed name: a file of that name including it keeps the name
        #      of the material in the errors, and lets it include the files of its folder
        file (WRITE "${bundle_path}/yup_MaterialSource.glsl" "#include \"${material_name}\"\n")

        # The name is looked for beside that file, then among the shaders of the renderer, before the folder of the
        # material: found there, that file would include itself without end, or a shader of the renderer be compiled
        if (EXISTS "${bundle_path}/${material_name}" OR EXISTS "${shader_dir}/${material_name}")
            _yup_message (FATAL_ERROR "yup_add_yui_resources: material shader '${shader_path}' has a name yup_renderer keeps for its own shaders")
        endif()

        set (bundler_args
            --vert "${shader_dir}/yup_MaterialShader.vert"
            --frag "${shader_dir}/yup_MaterialShader.frag"
            -DYUP_3D_MATERIAL_SOURCE)
        if (embedded_path MATCHES "\\.full\\.ysl$")
            list (APPEND bundler_args -DYUP_3D_TIER_FULL)
        endif()

        # The shaders of the renderer come first: a file of the material is never found in the place of one of them
        list (APPEND bundler_args "-I${shader_dir}" "-I${bundle_path}" "-I${material_dir}")

        # ==== Built again when the library of the template changes, globbed as for the bundles of the module, or a
        #      GLSL file beside the material: what it includes from elsewhere is not followed
        file (GLOB_RECURSE library_files CONFIGURE_DEPENDS "${shader_dir}/lib/*")
        file (GLOB neighbor_files CONFIGURE_DEPENDS "${material_dir}/*.glsl")
        list (REMOVE_ITEM neighbor_files "${shader_path}")

        set (depend_paths
            "${shader_dir}/yup_MaterialShader.vert"
            "${shader_dir}/yup_MaterialShader.frag"
            ${library_files}
            ${neighbor_files})
    endif()

    add_custom_command (
        OUTPUT "${bundle_path}.inc"
        COMMAND "${bundler_executable}" ${bundler_args} --output-inc "${bundle_path}.inc"
        DEPENDS ${bundler_target} "${bundler_executable}" "${shader_path}" ${depend_paths}
        WORKING_DIRECTORY "${bundle_path}"
        COMMENT "Building shader bundle ${embedded_path}"
        VERBATIM)
endfunction()

function (yup_add_yui_resources library_name)
    set (options VERIFY)
    set (one_value_args BASE_DIR NAMESPACE VERIFY_MODELS)
    set (multi_value_args FILES ASSETS SHADERS MATERIAL_SHADERS)
    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT YUP_ARG_FILES)
        _yup_message (FATAL_ERROR "yup_add_yui_resources: FILES argument is required")
    endif()

    _yup_set_default (YUP_ARG_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    _yup_set_default (YUP_ARG_NAMESPACE yup)
    get_filename_component (base_dir "${YUP_ARG_BASE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    cmake_path (NORMAL_PATH base_dir)

    string (MAKE_C_IDENTIFIER "${library_name}" library_identifier)

    set (output_path "${CMAKE_CURRENT_BINARY_DIR}/${library_name}")
    set (scratch_path "${output_path}/scratch")

    set (resource_names "")
    set (resource_keys "")
    set (resource_sources "")
    set (resource_files "")
    set (resource_includes "")
    set (resource_entries "")
    set (source_keys "")
    set (embedded_list "")
    set (yui_files "")

    foreach (argument IN ITEMS FILES ASSETS SHADERS MATERIAL_SHADERS)
        foreach (file IN LISTS YUP_ARG_${argument})
            get_filename_component (file_path "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
            cmake_path (NORMAL_PATH file_path)
            if (NOT EXISTS "${file_path}")
                _yup_message (FATAL_ERROR "yup_add_yui_resources: file not found: ${file_path}")
            endif()

            cmake_path (IS_PREFIX base_dir "${file_path}" NORMALIZE is_under_base_dir)
            if (NOT is_under_base_dir OR "${file_path}" STREQUAL "${base_dir}")
                _yup_message (FATAL_ERROR "yup_add_yui_resources: file '${file_path}' is outside BASE_DIR '${base_dir}'")
            endif()

            # ==== Names are compared without their case: macOS and Windows keep one file for `Main.yui` and `main.yui`,
            #      in the sources and among the files generated for them
            string (TOLOWER "${file_path}" source_key)
            if (source_key IN_LIST source_keys)
                _yup_message (FATAL_ERROR "yup_add_yui_resources: file '${file_path}' is listed again in ${argument} "
                    "(the case of a name does not count)")
            endif()
            list (APPEND source_keys "${source_key}")

            file (RELATIVE_PATH resource_path "${base_dir}" "${file_path}")
            file (TO_CMAKE_PATH "${resource_path}" resource_path)
            string (APPEND embedded_list "${resource_path}\n")

            # ==== FILES are the ones verified, and a shader is embedded as the bundles it is compiled into
            set (embedded_paths "${resource_path}")
            if (argument STREQUAL "FILES")
                list (APPEND yui_files "${file_path}")
            elseif (argument STREQUAL "SHADERS")
                set (embedded_paths "${resource_path}.ysl")
            elseif (argument STREQUAL "MATERIAL_SHADERS")
                set (embedded_paths "${resource_path}.raster.ysl" "${resource_path}.full.ysl")
            endif()

            foreach (embedded_path IN LISTS embedded_paths)
                string (MAKE_C_IDENTIFIER "${library_identifier}_${embedded_path}" resource_name)
                string (TOLOWER "${resource_name}" resource_key)

                # One symbol for two files: a path embedded twice, or paths differing by their case or by what a symbol
                # can't hold
                list (FIND resource_keys "${resource_key}" clash_index)
                if (NOT clash_index EQUAL -1)
                    list (GET resource_sources ${clash_index} clash_file)
                    _yup_message (FATAL_ERROR "yup_add_yui_resources: '${file_path}' can't be embedded as '${embedded_path}': "
                        "'${clash_file}' already has the symbol '${resource_name}' (the case of a name does not count)")
                endif()

                string (REPLACE "\\" "\\\\" escaped_resource_path "${embedded_path}")
                string (REPLACE "\"" "\\\"" escaped_resource_path "${escaped_resource_path}")

                list (APPEND resource_names "${resource_name}")
                list (APPEND resource_keys "${resource_key}")
                list (APPEND resource_sources "${file_path}")
                string (APPEND resource_entries "        { \"${escaped_resource_path}\", ${resource_name}_data, ${resource_name}_size },\n")

                # ==== The file as it is, embedded when configuring, or a bundle built from it
                if (embedded_path STREQUAL resource_path)
                    list (APPEND resource_files "${file_path}")
                else()
                    set (bundle_path "${output_path}/shaders/${resource_name}")
                    _yup_add_yui_shader_bundle (${argument} "${file_path}" "${bundle_path}" "${embedded_path}")
                    list (APPEND resource_includes "${bundle_path}.inc")
                endif()
            endforeach()
        endforeach()
    endforeach()

    set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${resource_files})

    if (YUP_ARG_VERIFY)
        set (verify_arguments "")

        if (YUP_ARG_VERIFY_MODELS)
            get_filename_component (models_path "${YUP_ARG_VERIFY_MODELS}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
            set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${models_path}")
            list (APPEND verify_arguments --models "${models_path}")
        endif()

        # ==== The documents are loaded as the library serves them: with the files listed here and no other, a shader
        #      standing for the bundles built from it, which the tool compiles. So each is verified again when it changes
        set (embedded_list_name "${library_identifier}_embedded.txt")
        file (WRITE "${scratch_path}/${embedded_list_name}" "${embedded_list}")
        configure_file ("${scratch_path}/${embedded_list_name}" "${output_path}/${embedded_list_name}" COPYONLY)
        list (APPEND verify_arguments --embedded-base "${base_dir}" --embedded "${output_path}/${embedded_list_name}")
        set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${resource_sources})

        _yup_build_yui_tool (yui_tool)
        _yup_execute_process_or_fail ("${yui_tool}" verify ${verify_arguments} ${yui_files})
    endif()

    yup_add_embedded_binary_resources (${library_name}
        OUT_DIR "${library_name}"
        HEADER "${library_identifier}Data.h"
        NAMESPACE ${YUP_ARG_NAMESPACE}
        RESOURCE_NAMES ${resource_names}
        RESOURCES ${resource_files}
        INCLUDES ${resource_includes})

    file (WRITE "${scratch_path}/${library_name}.h"
        "#pragma once\n"
        "\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "\n"
        "namespace ${YUP_ARG_NAMESPACE}\n"
        "{\n"
        "\n"
        "/** Returns the bytes embedded under a path and sets their size, or returns nullptr. */\n"
        "const uint8_t* ${library_identifier}_find (const char* path, std::size_t& size);\n"
        "\n"
        "} // namespace ${YUP_ARG_NAMESPACE}\n")

    file (WRITE "${scratch_path}/${library_identifier}_find.cpp"
        "#include \"${library_name}.h\"\n"
        "#include \"${library_identifier}Data.h\"\n"
        "\n"
        "#include <cstring>\n"
        "\n"
        "namespace ${YUP_ARG_NAMESPACE}\n"
        "{\n"
        "\n"
        "const uint8_t* ${library_identifier}_find (const char* path, std::size_t& size)\n"
        "{\n"
        "    if (path == nullptr)\n"
        "    {\n"
        "        size = 0;\n"
        "        return nullptr;\n"
        "    }\n"
        "\n"
        "    struct Entry\n"
        "    {\n"
        "        const char* path;\n"
        "        const uint8_t* data;\n"
        "        std::size_t size;\n"
        "    };\n"
        "\n"
        "    static const Entry entries[] =\n"
        "    {\n"
        "${resource_entries}"
        "    };\n"
        "\n"
        "    for (const auto& entry : entries)\n"
        "    {\n"
        "        if (std::strcmp (entry.path, path) == 0)\n"
        "        {\n"
        "            size = entry.size;\n"
        "            return entry.data;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    size = 0;\n"
        "    return nullptr;\n"
        "}\n"
        "\n"
        "} // namespace ${YUP_ARG_NAMESPACE}\n")

    # Copied only when the content differs, to avoid recompiling
    configure_file ("${scratch_path}/${library_name}.h" "${output_path}/${library_name}.h" COPYONLY)
    configure_file ("${scratch_path}/${library_identifier}_find.cpp" "${output_path}/${library_identifier}_find.cpp" COPYONLY)

    target_sources (${library_name}
        PUBLIC "${output_path}/${library_name}.h"
        PRIVATE "${output_path}/${library_identifier}_find.cpp")
endfunction()
