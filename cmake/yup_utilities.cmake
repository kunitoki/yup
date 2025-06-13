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

function (_yup_set_default var value)
    if (NOT DEFINED ${var})
        set (${var} "${value}" PARENT_SCOPE)
    endif()
endfunction()

#==============================================================================

function (_yup_bool_to_string value output_variable)
    if (value)
        set (${output_variable} "true" PARENT_SCOPE)
    else()
        set (${output_variable} "false" PARENT_SCOPE)
    endif()
endfunction()

#==============================================================================

function (_yup_valid_identifier_string identifier output_variable)
    string (REPLACE "_" "-" identifier ${identifier})
    set (${output_variable} "${identifier}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_strip_list input_list output_variable)
    set (inner_list "")
    foreach (item ${input_list})
        string (STRIP ${item} stripped_item)
        if (${stripped_item} STREQUAL "")
            continue()
        endif()
        list (APPEND inner_list ${stripped_item})
    endforeach()
    set (${output_variable} "${inner_list}" PARENT_SCOPE)
endfunction()

function (_yup_comma_or_space_separated_list input_list output_variable)
    set (final_list "")
    list (LENGTH input_list input_list_len)
    if (${input_list_len} GREATER 1)
        string (REPLACE "," " " input_list ${input_list})
    endif()
    if (${input_list_len} GREATER 0)
        string (REPLACE " " ";" input_list ${input_list})
        _yup_strip_list ("${input_list}" final_list)
    endif()
    set (${output_variable} "${final_list}" PARENT_SCOPE)
endfunction()

function (_yup_join_list_with_separator input_list separator prefix suffix output_variable)
    set (result_string "")
    set (local_separator "")

    foreach (item ${input_list})
        string (APPEND result_string "${local_separator}${prefix}${item}${suffix}")
        set (local_separator "${separator}")
    endforeach()

    set (${output_variable} "${result_string}" PARENT_SCOPE)
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
    string (REPLACE "." ";" version_parts "${version_string}")
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
    file (READ "${file_path}" hex_contents HEX)
    string (REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" separated_hex ${hex_contents})

    list (JOIN separated_hex ", 0x" formatted_hex)
    string (PREPEND formatted_hex "0x")
    string (APPEND formatted_hex "")

    set (${output_variable} "${formatted_hex}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_generate_random_filename prefix extension output_variable)
    # Generate a random number
    math (RANDOM random_number)

    # Format the filename
    set (random_filename "${prefix}${random_number}${extension}")

    # Return the generated filename
    set (${output_variable} "${random_filename}" PARENT_SCOPE)
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

function (_yup_resolve_variable_path input_path output_variable)
    string (REPLACE "{HOME}" "$ENV{HOME}" input_path ${input_path})
    string (REPLACE "{ANDROID_NDK}" "${ANDROID_NDK}" input_path ${input_path})
    if (YUP_ENABLE_VULKAN)
        string (REPLACE "{Vulkan_INCLUDE_DIR}" "${Vulkan_INCLUDE_DIR}" input_path ${input_path})
    endif()
    set (${output_variable} "${input_path}" PARENT_SCOPE)
endfunction()

function (_yup_resolve_variable_paths input_list output_list)
    set (resolved_list "")

    foreach (item IN LISTS input_list)
        _yup_resolve_variable_path ("${item}" resolved_item)
        list (APPEND resolved_list "${resolved_item}")
    endforeach()

    set (${output_list} "${resolved_list}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_merge_plist original_plist subset_xml_string output_plist)
    if (NOT EXISTS "${original_plist}")
        message (FATAL_ERROR "Original plist file does not exist: ${original_plist}")
    endif()

    file (COPY "${original_plist}" DESTINATION "${output_plist}")

    _yup_generate_random_filename ("${CMAKE_BINARY_DIR}/temp_plist_" ".plist" temp_plist)
    file (WRITE "${temp_plist}" "${subset_xml_string}")

    execute_process(
        COMMAND /usr/libexec/PlistBuddy -c "Merge ${temp_plist}" "${output_plist}"
        RESULT_VARIABLE result
        ERROR_VARIABLE error_message)

    if (NOT result EQUAL 0)
        message (FATAL_ERROR "Failed to merge plist: ${error_message}")
    endif()

    file (REMOVE "${temp_plist}")
endfunction()

#==============================================================================

function (_yup_execute_process_or_fail)
    execute_process (
        COMMAND ${ARGN}
        RESULT_VARIABLE result
        ERROR_VARIABLE error_message
        OUTPUT_QUIET)

    if (NOT result EQUAL 0)
        _yup_join_list_with_separator ("${command}" " " "" "" command_string)
        message (FATAL_ERROR "Failed to execute command '${command_string}': ${error_message}")
    endif()
endfunction()

#==============================================================================

function (_yup_convert_png_to_icns png_path icons_path output_variable)
    set (temp_iconset_path "${icons_path}.iconset")
    set (output_iconset_path "${icons_path}.icns")

    file (REMOVE_RECURSE "${temp_iconset_path}")
    file (MAKE_DIRECTORY "${temp_iconset_path}")

    _yup_execute_process_or_fail (/usr/bin/sips -z 16 16 "${png_path}" --out "${temp_iconset_path}/icon_16x16.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 32 32 "${png_path}" --out "${temp_iconset_path}/icon_32x32.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 32 32 "${png_path}" --out "${temp_iconset_path}/icon_16x16@2x.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 64 64 "${png_path}" --out "${temp_iconset_path}/icon_32x32@2x.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 128 128 "${png_path}" --out "${temp_iconset_path}/icon_128x128.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 256 256 "${png_path}" --out "${temp_iconset_path}/icon_128x128@2x.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 256 256 "${png_path}" --out "${temp_iconset_path}/icon_256x256.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 512 512 "${png_path}" --out "${temp_iconset_path}/icon_256x256@2x.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 512 512 "${png_path}" --out "${temp_iconset_path}/icon_512x512.png")
    _yup_execute_process_or_fail (/usr/bin/sips -z 1024 1024 "${png_path}" --out "${temp_iconset_path}/icon_512x512@2x.png")
    _yup_execute_process_or_fail (/usr/bin/iconutil -c icns -o "${output_iconset_path}" "${temp_iconset_path}")

    file (REMOVE_RECURSE "${temp_iconset_path}")

    set (${output_variable} "${output_iconset_path}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_setup_coverage_flags target_name)
    if (YUP_ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        _yup_message (STATUS "Enabling coverage for target: ${target_name}")

        target_compile_options (${target_name} INTERFACE
            --coverage
            -fprofile-arcs
            -fprofile-update=atomic
            -ftest-coverage
            -fno-elide-constructors)

        target_link_options (${target_name} INTERFACE --coverage)

        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_link_libraries (${target_name} INTERFACE gcov)
        endif()
    endif()
endfunction()

#==============================================================================

function (_yup_setup_coverage_targets test_target_name modules_list)
    if (YUP_ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        find_program (LCOV_PATH lcov)
        find_program (GENHTML_PATH genhtml)

        if (NOT LCOV_PATH)
            _yup_message (WARNING "lcov not found! Coverage reports will not be generated.")
            return()
        endif()

        # Create coverage directory
        file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/coverage)

        # Overall coverage target
        add_custom_target (coverage_clean
            COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters
            COMMENT "Cleaning coverage data")

        # Per-module coverage targets
        foreach (module_name ${modules_list})
            set (module_coverage_dir ${CMAKE_BINARY_DIR}/coverage/${module_name})
            file (MAKE_DIRECTORY ${module_coverage_dir})

            add_custom_target (coverage_${module_name}
                COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage for ${module_name}"
                COMMAND ${LCOV_PATH}
                    --directory ${CMAKE_BINARY_DIR} --capture
                    --output-file ${module_coverage_dir}/coverage.info
                    --ignore-errors mismatch,gcov,source,negative
                COMMAND ${LCOV_PATH}
                    --extract ${module_coverage_dir}/coverage.info "*/modules/${module_name}/*"
                    --output-file ${module_coverage_dir}/coverage_filtered.info
                    --ignore-errors mismatch,gcov,source,negative
                COMMAND ${LCOV_PATH}
                    --remove ${module_coverage_dir}/coverage_filtered.info "*/thirdparty/*" "*/build/*" "*/tests/*" "*/examples/*"
                    --output-file ${module_coverage_dir}/coverage_final.info
                    --ignore-errors mismatch,gcov,source,negative
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${test_target_name}
                COMMENT "Processing coverage data for ${module_name}")

            if (GENHTML_PATH)
                add_custom_target (coverage_html_${module_name}
                    COMMAND ${GENHTML_PATH}
                        ${module_coverage_dir}/coverage_final.info --output-directory ${module_coverage_dir}/html
                    DEPENDS coverage_${module_name}
                    COMMENT "Generating HTML coverage report for ${module_name}")
            endif()
        endforeach()

        # Combined coverage target
        add_custom_target (coverage_all
            COMMAND ${CMAKE_COMMAND} -E echo "Generating combined coverage report"
            COMMAND ${LCOV_PATH}
                --directory ${CMAKE_BINARY_DIR} --capture
                --output-file ${CMAKE_BINARY_DIR}/coverage/coverage.info
                --ignore-errors mismatch,gcov,source,negative
            COMMAND ${LCOV_PATH}
                --remove ${CMAKE_BINARY_DIR}/coverage/coverage.info "*/thirdparty/*" "*/build/*" "*/tests/*" "*/examples/*"
                --output-file ${CMAKE_BINARY_DIR}/coverage/coverage_final.info
                --ignore-errors mismatch,gcov,source,negative
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${test_target_name}
            COMMENT "Processing combined coverage data")

        if (GENHTML_PATH)
            add_custom_target (coverage_html_all
                COMMAND ${GENHTML_PATH}
                    ${CMAKE_BINARY_DIR}/coverage/coverage_final.info --output-directory ${CMAKE_BINARY_DIR}/coverage/html
                DEPENDS coverage_all
                COMMENT "Generating combined HTML coverage report")
        endif()
    endif()
endfunction()
