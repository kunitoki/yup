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

function (yup_add_embedded_binary_resources library_name)
    set (options "")
    set (one_value_args OUT_DIR HEADER NAMESPACE)
    set (multi_value_args RESOURCE_NAMES RESOURCES)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (binary_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}")
    set (binary_header_path "${binary_path}/${YUP_ARG_HEADER}")
    set (binary_sources "")

    add_library (${library_name} OBJECT)

    target_include_directories (${library_name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features (${library_name} PUBLIC cxx_std_17)

    set_target_properties (${library_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    file (WRITE "${binary_header_path}"
        "#pragma once\n"
        "\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "\n")

    if (DEFINED YUP_ARG_NAMESPACE)
        file (APPEND "${binary_header_path}"
            "namespace ${YUP_ARG_NAMESPACE}\n"
            "{\n"
            "\n")
    endif()

    foreach (resource_name resource IN ZIP_LISTS YUP_ARG_RESOURCE_NAMES YUP_ARG_RESOURCES)
        set (full_resource_unit_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}/${resource_name}.cpp")
        set (full_resource_hex_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}/${resource_name}.inc")

        # Add symbol to header
        file (APPEND "${binary_header_path}"
            "extern const uint8_t ${resource_name}_data[];\n"
            "extern const std::size_t ${resource_name}_size;\n"
            "\n")

        # Write .cpp
        file (WRITE "${full_resource_unit_path}"
            "#include \"${YUP_ARG_HEADER}\"\n"
            "\n"
            "#include <cstdint>\n"
            "\n")

        if (DEFINED YUP_ARG_NAMESPACE)
            file (APPEND "${full_resource_unit_path}"
                "namespace ${YUP_ARG_NAMESPACE}\n"
                "{\n"
                "\n")
        endif()

        file (APPEND "${full_resource_unit_path}"
            "const uint8_t ${resource_name}_data[] = \n"
            "{\n"
            "#include \"${resource_name}.inc\"\n"
            "};\n"
            "\n"
            "const std::size_t ${resource_name}_size = sizeof (${resource_name}_data);\n"
            "\n")

        if (DEFINED YUP_ARG_NAMESPACE)
            file (APPEND "${full_resource_unit_path}"
                "\n"
                "} // namespace ${YUP_ARG_NAMESPACE}\n")
        endif()

        _yup_file_to_byte_array (${resource} resource_byte_array)
        file (WRITE "${full_resource_hex_path}" "${resource_byte_array}")

        list (APPEND binary_sources "${full_resource_unit_path}")
        list (APPEND resources_hex_files "${full_resource_hex_path}")
    endforeach()

    if (DEFINED YUP_ARG_NAMESPACE)
        file (APPEND "${binary_header_path}"
            "} // namespace ${YUP_ARG_NAMESPACE}\n")
    endif()

    target_sources (${library_name}
        PUBLIC "${binary_header_path}"
        PRIVATE "${binary_sources}")

    target_include_directories (${library_name} PUBLIC "${binary_path}")

    add_custom_target ("${library_name}_content" DEPENDS "${resources_hex_files}")
    add_dependencies (${library_name} "${library_name}_content")

endfunction()
