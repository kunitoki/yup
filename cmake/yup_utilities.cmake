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

function (_yup_strip_list input_list output_variable)
    set (inner_list "" PARENT_SCOPE)
    foreach (item ${input_list})
        string (STRIP ${item} stripped_item)
        if (${stripped_item} STREQUAL "")
            continue()
        endif()
        list (APPEND inner_list ${stripped_item})
    endforeach()
    set (${output_variable} "${inner_list}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_make_short_version version output_variable)
    string (REPLACE "." ";" version_list ${version})
    list (LENGTH version_list version_list_length)
    math (EXPR version_list_last_index "${version_list_length} - 1")
    list (REMOVE_AT version_list ${version_list_last_index})
    string (JOIN "." version_short ${version_list})
    set (${output_variable} "${version_short}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_comma_or_space_separated_list input_list output_variable)
    string (REPLACE "," " " temp1_list ${input_list})
    string (REPLACE " " ";" temp2_list ${temp1_list})
    _yup_strip_list ("${temp2_list}" final_list)
    set (${output_variable} "${final_list}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_boolean_property input_bool output_variable)
    string (STRIP "${input_bool}" ${input_bool})
    string (TOLOWER "${input_bool}" ${input_bool})
    if ("${input_bool}" STREQUAL "on" OR "${input_bool}" STREQUAL "true" OR "${input_bool}" STREQUAL "1")
        set (${output_variable} ON PARENT_SCOPE)
    else()
        set (${output_variable} OFF PARENT_SCOPE)
    endif()
endfunction()

#==============================================================================

function (_yup_version_string_to_version_code version_string output_variable)
    string (REPLACE "." ";" version_parts ${version_string})
    list (LENGTH version_parts num_parts)
    set (major_version 0)
    set (minor_version 0)
    set (patch_version 0)

    if (${num_parts} GREATER 0)
        list (GET version_parts 0 major_version)
    endif()
    if (${num_parts} GREATER 1)
        list (GET version_parts 1 minor_version)
    endif()
    if (${num_parts} GREATER 2)
        list (GET version_parts 2 patch_version)
    endif()

    math (EXPR major_version_number "${major_version}")
    math (EXPR minor_version_number "${minor_version}")
    math (EXPR patch_version_number "${patch_version}")

    math (EXPR version_code "${major_version_number} * 100000 + ${minor_version_number} * 1000 + ${patch_version}")
    set (${output_variable} ${version_code} PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_file_to_byte_array file_path output_variable)
    file (READ ${file_path} hex_contents HEX)
    string (REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" separated_hex ${hex_contents})

    list (JOIN separated_hex ", 0x" formatted_hex)
    string (PREPEND formatted_hex "0x")
    string (APPEND formatted_hex "")

    set (${output_variable} "${formatted_hex}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_get_package_config_libs package_name output_variable)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (${package_name} REQUIRED IMPORTED_TARGET ${package_name})
    set (${output_variable} "PkgConfig::${package_name}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_glob_recurse folder output_variable)
    file (GLOB_RECURSE all_files "${folder}")

    set (non_hidden_files "")
    foreach (item ${all_files})
        get_filename_component (file_name ${item} NAME)
        if (NOT ${file_name} MATCHES "^\\..*$")
            list (APPEND non_hidden_files ${item})
        endif()
    endforeach()

    set (${output_variable} "${non_hidden_files}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_convert_png_to_icns png_path icons_path output_variable)
    set (temp_iconset_path "${icons_path}.iconset")
    set (output_iconset_path "${icons_path}.icns")

    # TODO - check png_path has png extension

    add_custom_command(
        OUTPUT "${output_iconset_path}"
        COMMAND mkdir -p "${temp_iconset_name}"
        COMMAND sips -z 16 16     -s format png "${png_path}" --out "${temp_iconset_path}/icon_16x16.png"
        COMMAND sips -z 32 32     -s format png "${png_path}" --out "${temp_iconset_path}/icon_32x32.png"
        COMMAND sips -z 32 32     -s format png "${png_path}" --out "${temp_iconset_path}/icon_16x16@2x.png"
        COMMAND sips -z 64 64     -s format png "${png_path}" --out "${temp_iconset_path}/icon_32x32@2x.png"
        COMMAND sips -z 128 128   -s format png "${png_path}" --out "${temp_iconset_path}/icon_128x128.png"
        COMMAND sips -z 256 256   -s format png "${png_path}" --out "${temp_iconset_path}/icon_128x128@2x.png"
        COMMAND sips -z 256 256   -s format png "${png_path}" --out "${temp_iconset_path}/icon_256x256.png"
        COMMAND sips -z 512 512   -s format png "${png_path}" --out "${temp_iconset_path}/icon_256x256@2x.png"
        COMMAND sips -z 512 512   -s format png "${png_path}" --out "${temp_iconset_path}/icon_512x512.png"
        COMMAND sips -z 1024 1024 -s format png "${png_path}" --out "${temp_iconset_path}/icon_512x512@2x.png"
        COMMAND iconutil -c icns "${temp_iconset_path}"
        COMMAND rm -R "${temp_iconset_path}")

    set (${output_variable} "${output_iconset_path}" PARENT_SCOPE)
endfunction()
