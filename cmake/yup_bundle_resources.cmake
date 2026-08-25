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

function (yup_add_bundled_resources target_name)
    set (options "")
    set (one_value_args "")
    set (multi_value_args RESOURCES)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    foreach (resource_pair IN LISTS YUP_ARG_RESOURCES)
        string (REPLACE "@" ";" resource_pair_parts "${resource_pair}")
        list (GET resource_pair_parts 0 source_file)
        list (GET resource_pair_parts 1 dest_path)

        get_filename_component (source_name "${source_file}" NAME)
        get_filename_component (dest_name "${dest_path}" NAME)

        if (NOT source_name STREQUAL dest_name)
            set (staged_file "${CMAKE_CURRENT_BINARY_DIR}/bundled_resources/${dest_path}")
            configure_file ("${source_file}" "${staged_file}" COPYONLY)
            set (source_file "${staged_file}")
        endif()

        if (YUP_PLATFORM_APPLE)
            get_filename_component (dest_dir "${dest_path}" DIRECTORY)

            if (YUP_PLATFORM_IOS)
                if (dest_dir)
                    set (macosx_package_location "${dest_dir}")
                else()
                    set (macosx_package_location ".")
                endif()
            elseif (dest_dir)
                set (macosx_package_location "Resources/${dest_dir}")
            else()
                set (macosx_package_location "Resources")
            endif()

            set_source_files_properties ("${source_file}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "${macosx_package_location}")

            target_sources (${target_name} PRIVATE "${source_file}")

        elseif (YUP_TARGET_ANDROID)
            configure_file ("${source_file}" "${CMAKE_CURRENT_BINARY_DIR}/app/src/main/assets/${dest_path}" COPYONLY)

        elseif (YUP_PLATFORM_EMSCRIPTEN)
            target_link_options (${target_name} PRIVATE "--preload-file=${source_file}@${dest_path}")
        endif()
    endforeach()
endfunction()
