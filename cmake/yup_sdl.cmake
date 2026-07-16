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

function (_yup_sdl_make_symbols_force_include_options output_variable patch_header)
    set (force_include_options "")

    foreach (source_language IN ITEMS C CXX OBJC OBJCXX)
        list (APPEND force_include_options
            "$<$<COMPILE_LANG_AND_ID:${source_language},MSVC>:/FI${patch_header}>"
            "$<$<COMPILE_LANG_AND_ID:${source_language},AppleClang,Clang,GNU>:-include>"
            "$<$<COMPILE_LANG_AND_ID:${source_language},AppleClang,Clang,GNU>:${patch_header}>")
    endforeach()

    set (${output_variable} "${force_include_options}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_sdl_get_target_property output_variable target_name property_name)
    get_target_property (property_value ${target_name} ${property_name})
    if (property_value)
        set (${output_variable} "${property_value}" PARENT_SCOPE)
    else()
        set (${output_variable} "" PARENT_SCOPE)
    endif()
endfunction()

#==============================================================================

function (_yup_sdl_collect_private_include_dirs output_variable sdl_source_dir)
    set (include_dirs "")

    foreach (include_dir IN ITEMS
        "${sdl_source_dir}/src"
        "${sdl_source_dir}/src/hidapi"
        "${sdl_source_dir}/src/video/khronos")
        if (EXISTS "${include_dir}")
            list (APPEND include_dirs "${include_dir}")
        endif()
    endforeach()

    file (GLOB_RECURSE sdl_private_headers CONFIGURE_DEPENDS
        "${sdl_source_dir}/src/*.h"
        "${sdl_source_dir}/src/*.hpp")

    foreach (sdl_private_header IN LISTS sdl_private_headers)
        get_filename_component (include_dir "${sdl_private_header}" DIRECTORY)
        list (APPEND include_dirs "${include_dir}")
    endforeach()

    list (REMOVE_DUPLICATES include_dirs)
    set (${output_variable} "${include_dirs}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_sdl_apply_build_options private_sdl_target)
    if (NOT TARGET sdl-build-options)
        return()
    endif()

    _yup_sdl_get_target_property (sdl_build_include_dirs sdl-build-options INTERFACE_INCLUDE_DIRECTORIES)
    if (sdl_build_include_dirs)
        target_include_directories (${private_sdl_target} SYSTEM PRIVATE ${sdl_build_include_dirs})
    endif()

    _yup_sdl_get_target_property (sdl_build_system_include_dirs sdl-build-options INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
    if (sdl_build_system_include_dirs)
        target_include_directories (${private_sdl_target} SYSTEM PRIVATE ${sdl_build_system_include_dirs})
    endif()

    _yup_sdl_get_target_property (sdl_build_compile_definitions sdl-build-options INTERFACE_COMPILE_DEFINITIONS)
    if (sdl_build_compile_definitions)
        target_compile_definitions (${private_sdl_target} PRIVATE ${sdl_build_compile_definitions})
    endif()

    _yup_sdl_get_target_property (sdl_build_compile_options sdl-build-options INTERFACE_COMPILE_OPTIONS)
    if (sdl_build_compile_options)
        target_compile_options (${private_sdl_target} PRIVATE ${sdl_build_compile_options})
    endif()

    _yup_sdl_get_target_property (sdl_build_link_libraries sdl-build-options INTERFACE_LINK_LIBRARIES)
    if (sdl_build_link_libraries)
        target_link_libraries (${private_sdl_target} PRIVATE ${sdl_build_link_libraries})
    endif()
endfunction()

#==============================================================================

function (_yup_sdl_configure_private_static_target target_name force_include_options output_target)
    if (NOT TARGET sdl::sdl)
        _yup_message (FATAL_ERROR "Cannot configure private SDL symbols for ${target_name}: sdl::sdl target is not available.")
    endif()

    _yup_sdl_get_target_property (sdl_sources sdl::sdl SOURCES)
    if (NOT sdl_sources)
        _yup_message (FATAL_ERROR "Cannot configure private SDL symbols for ${target_name}: sdl::sdl has no source files.")
    endif()

    set (private_sdl_target "${target_name}_sdl")
    add_library (${private_sdl_target} STATIC ${sdl_sources})

    if (APPLE)
        set (sdl_objc_sources "")
        foreach (sdl_source IN LISTS sdl_sources)
            if (sdl_source MATCHES "^.*\\.m$")
                list (APPEND sdl_objc_sources "${sdl_source}")
            endif()
        endforeach()

        if (sdl_objc_sources)
            set_property (SOURCE ${sdl_objc_sources}
                TARGET_DIRECTORY ${private_sdl_target}
                APPEND_STRING PROPERTY COMPILE_FLAGS " -x objective-c")
        endif()
    endif()

    if (TARGET sdl_headers_copy)
        add_dependencies (${private_sdl_target} sdl_headers_copy)
    endif()

    _yup_sdl_get_target_property (sdl_include_dirs sdl::sdl INCLUDE_DIRECTORIES)
    if (sdl_include_dirs)
        target_include_directories (${private_sdl_target} PRIVATE ${sdl_include_dirs})
    endif()

    _yup_sdl_get_target_property (sdl_source_dir sdl::sdl SOURCE_DIR)
    if (sdl_source_dir)
        _yup_sdl_collect_private_include_dirs (sdl_private_include_dirs "${sdl_source_dir}")
        if (sdl_private_include_dirs)
            target_include_directories (${private_sdl_target} SYSTEM PRIVATE ${sdl_private_include_dirs})
        endif()
    endif()

    _yup_sdl_apply_build_options (${private_sdl_target})

    _yup_sdl_get_target_property (sdl_interface_include_dirs sdl::sdl INTERFACE_INCLUDE_DIRECTORIES)
    if (sdl_interface_include_dirs)
        target_include_directories (${private_sdl_target} INTERFACE ${sdl_interface_include_dirs})
    endif()

    _yup_sdl_get_target_property (sdl_compile_definitions sdl::sdl COMPILE_DEFINITIONS)
    if (sdl_compile_definitions)
        target_compile_definitions (${private_sdl_target} PRIVATE ${sdl_compile_definitions})
    endif()

    _yup_sdl_get_target_property (sdl_interface_compile_definitions sdl::sdl INTERFACE_COMPILE_DEFINITIONS)
    if (sdl_interface_compile_definitions)
        target_compile_definitions (${private_sdl_target} INTERFACE ${sdl_interface_compile_definitions})
    endif()

    _yup_sdl_get_target_property (sdl_compile_options sdl::sdl COMPILE_OPTIONS)
    target_compile_options (${private_sdl_target} PRIVATE
        ${sdl_compile_options}
        ${force_include_options})

    _yup_sdl_get_target_property (sdl_interface_compile_options sdl::sdl INTERFACE_COMPILE_OPTIONS)
    if (sdl_interface_compile_options)
        target_compile_options (${private_sdl_target} INTERFACE ${sdl_interface_compile_options})
    endif()

    _yup_sdl_get_target_property (sdl_link_libraries sdl::sdl LINK_LIBRARIES)
    if (sdl_link_libraries)
        target_link_libraries (${private_sdl_target} PRIVATE ${sdl_link_libraries})
    endif()

    _yup_sdl_get_target_property (sdl_interface_link_libraries sdl::sdl INTERFACE_LINK_LIBRARIES)
    if (sdl_interface_link_libraries)
        target_link_libraries (${private_sdl_target} INTERFACE ${sdl_interface_link_libraries})
    endif()

    _yup_sdl_get_target_property (sdl_link_options sdl::sdl LINK_OPTIONS)
    if (sdl_link_options)
        target_link_options (${private_sdl_target} PRIVATE ${sdl_link_options})
    endif()

    _yup_sdl_get_target_property (sdl_interface_link_options sdl::sdl INTERFACE_LINK_OPTIONS)
    if (sdl_interface_link_options)
        target_link_options (${private_sdl_target} INTERFACE ${sdl_interface_link_options})
    endif()

    _yup_sdl_get_target_property (sdl_position_independent_code sdl::sdl POSITION_INDEPENDENT_CODE)
    if (sdl_position_independent_code)
        set_target_properties (${private_sdl_target} PROPERTIES
            POSITION_INDEPENDENT_CODE "${sdl_position_independent_code}")
    endif()

    _yup_sdl_get_target_property (sdl_debug_postfix sdl::sdl DEBUG_POSTFIX)
    if (sdl_debug_postfix)
        set_target_properties (${private_sdl_target} PROPERTIES
            DEBUG_POSTFIX "${sdl_debug_postfix}")
    endif()

    set_target_properties (${private_sdl_target} PROPERTIES
        OUTPUT_NAME "${target_name}_sdl-static"
        FOLDER "Thirdparty")

    set_property (TARGET ${private_sdl_target} APPEND PROPERTY COMPATIBLE_INTERFACE_BOOL SDL3_SHARED)
    set_property (TARGET ${private_sdl_target} PROPERTY INTERFACE_SDL3_SHARED FALSE)
    set_property (TARGET ${private_sdl_target} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING "SDL_VERSION")
    set_property (TARGET ${private_sdl_target} PROPERTY INTERFACE_SDL_VERSION "SDL3")

    set (${output_target} "${private_sdl_target}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_sdl_configure_symbols_patch target_name output_patch_target output_sdl_target)
    string (MAKE_C_IDENTIFIER "${target_name}" YupPluginPrefix)
    if (YupPluginPrefix MATCHES "^[0-9]")
        set (YupPluginPrefix "_${YupPluginPrefix}")
    endif()

    set (patch_header "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_sdl-symbols-patch.h")
    configure_file ("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/resources/sdl-symbols-patch.h.in" "${patch_header}" @ONLY)

    set (patch_target "${target_name}_sdl_symbols_patch")
    add_library (${patch_target} INTERFACE)
    target_sources (${patch_target} INTERFACE "${patch_header}")

    _yup_sdl_make_symbols_force_include_options (force_include_options "${patch_header}")
    target_compile_options (${patch_target} INTERFACE ${force_include_options})

    _yup_sdl_configure_private_static_target ("${target_name}" "${force_include_options}" private_sdl_target)

    set (${output_patch_target} "${patch_target}" PARENT_SCOPE)
    set (${output_sdl_target} "${private_sdl_target}" PARENT_SCOPE)
endfunction()

#==============================================================================
