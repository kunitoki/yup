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
# Embeds .yui files into an OBJECT library, through
# yup_add_embedded_binary_resources, for YuiLoader::loadResource. The generated
# <library_name>.h declares, in NAMESPACE (yup by default):
#
#   const uint8_t* <c_identifier>_find (const char* path, std::size_t& size);
#
# where <c_identifier> is MAKE_C_IDENTIFIER applied to <library_name>. It
# returns the bytes of a file by its "/" normalized path relative to BASE_DIR
# (the current source directory by default), as written by `import`, or nullptr.
# The files are embedded when configuring, and editing one configures again.
#
# VERIFY checks the files with `yup_yui_tool verify` when configuring, which
# fails with the errors found. The tool is built for the host the first time,
# into the build tree, so the option costs a build of yup_gui. VERIFY_MODELS
# is the JSON file declaring the models the files bind to (see `--models`).

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

function (yup_add_yui_resources library_name)
    set (options VERIFY)
    set (one_value_args BASE_DIR NAMESPACE VERIFY_MODELS)
    set (multi_value_args FILES)
    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT YUP_ARG_FILES)
        _yup_message (FATAL_ERROR "yup_add_yui_resources: FILES argument is required")
    endif()

    _yup_set_default (YUP_ARG_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    _yup_set_default (YUP_ARG_NAMESPACE yup)
    get_filename_component (base_dir "${YUP_ARG_BASE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    cmake_path (NORMAL_PATH base_dir)

    string (MAKE_C_IDENTIFIER "${library_name}" library_identifier)

    set (resource_names "")
    set (resource_files "")
    set (resource_entries "")

    foreach (file IN LISTS YUP_ARG_FILES)
        get_filename_component (file_path "${file}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        cmake_path (NORMAL_PATH file_path)
        if (NOT EXISTS "${file_path}")
            _yup_message (FATAL_ERROR "yup_add_yui_resources: file not found: ${file_path}")
        endif()

        cmake_path (IS_PREFIX base_dir "${file_path}" NORMALIZE is_under_base_dir)
        if (NOT is_under_base_dir OR "${file_path}" STREQUAL "${base_dir}")
            _yup_message (FATAL_ERROR "yup_add_yui_resources: file '${file_path}' is outside BASE_DIR '${base_dir}'")
        endif()

        file (RELATIVE_PATH resource_path "${base_dir}" "${file_path}")
        file (TO_CMAKE_PATH "${resource_path}" resource_path)
        string (MAKE_C_IDENTIFIER "${library_identifier}_${resource_path}" resource_name)
        string (REPLACE "\\" "\\\\" escaped_resource_path "${resource_path}")
        string (REPLACE "\"" "\\\"" escaped_resource_path "${escaped_resource_path}")

        list (APPEND resource_names "${resource_name}")
        list (APPEND resource_files "${file_path}")
        string (APPEND resource_entries "        { \"${escaped_resource_path}\", ${resource_name}_data, ${resource_name}_size },\n")
    endforeach()

    set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${resource_files})

    if (YUP_ARG_VERIFY)
        set (verify_arguments "")

        if (YUP_ARG_VERIFY_MODELS)
            get_filename_component (models_path "${YUP_ARG_VERIFY_MODELS}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
            set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${models_path}")
            list (APPEND verify_arguments --models "${models_path}")
        endif()

        _yup_build_yui_tool (yui_tool)
        _yup_execute_process_or_fail ("${yui_tool}" verify ${verify_arguments} ${resource_files})
    endif()

    yup_add_embedded_binary_resources (${library_name}
        OUT_DIR "${library_name}"
        HEADER "${library_identifier}Data.h"
        NAMESPACE ${YUP_ARG_NAMESPACE}
        RESOURCE_NAMES ${resource_names}
        RESOURCES ${resource_files})

    set (output_path "${CMAKE_CURRENT_BINARY_DIR}/${library_name}")
    set (scratch_path "${output_path}/scratch")

    file (WRITE "${scratch_path}/${library_name}.h"
        "#pragma once\n"
        "\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "\n"
        "namespace ${YUP_ARG_NAMESPACE}\n"
        "{\n"
        "\n"
        "/** Returns the bytes of an embedded .yui file and sets their size, or returns nullptr. */\n"
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
