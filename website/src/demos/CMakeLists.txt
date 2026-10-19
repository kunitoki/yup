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

cmake_minimum_required (VERSION 3.31)

# ==== Set target name and version
set (target_name website_demos)
set (target_version "1.0.0")

project (${target_name} VERSION ${target_version})

# ==== Embed the shader bundles, exposed as website::<Name>Shader_data / _size by <ShaderBundles/<Name>Shader.h>
#      A shader without its own .vert uses the fullscreen triangle of the graphics example.
set (shaders_dir "${CMAKE_CURRENT_LIST_DIR}/shaders")
set (fullscreen_vertex "${CMAKE_CURRENT_LIST_DIR}/../../../examples/graphics/data/shaders/fullscreen.vert")
file (GLOB shader_includes "${shaders_dir}/*.glsl")

set (shader_libraries "")
foreach (shader IN ITEMS Surface Ripple)
    string (TOLOWER "${shader}" shader_file)

    set (vertex_shader "${fullscreen_vertex}")
    if (EXISTS "${shaders_dir}/${shader_file}.vert")
        set (vertex_shader "${shaders_dir}/${shader_file}.vert")
    endif()

    yup_add_shader_bundle (${target_name}_${shader_file}_shader
        VERT "${vertex_shader}"
        FRAG "${shaders_dir}/${shader_file}.frag"
        NAMESPACE website
        OUTPUT_NAME ${shader}Shader
        RESOURCE_NAME ${shader}Shader
        TEMP_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        DEPENDS ${shader_includes}
        OPTIONS "-I${shaders_dir}")

    list (APPEND shader_libraries ${target_name}_${shader_file}_shader)
endforeach()

# ==== Embed the app snippet the get-started page shows, for the "start" demo (website::appSnippet_data / _size)
yup_add_embedded_binary_resources (${target_name}_snippets
    OUT_DIR Snippets
    HEADER AppSnippet.h
    NAMESPACE website
    RESOURCE_NAMES appSnippet
    RESOURCES "${CMAKE_CURRENT_LIST_DIR}/../partials/snippets/app.cpp")

# ==== Optimize release builds for size with link time optimization, to keep the page download small
set (size_options "")
if (YUP_PLATFORM_EMSCRIPTEN)
    set (size_options $<$<CONFIG:Release>:-Os> $<$<CONFIG:Release>:-flto>)
endif()

# ==== Prepare target, hosted by the website page instead of the shell
yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Website"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    CUSTOM_SHELL "${CMAKE_CURRENT_LIST_DIR}/shell.html"
    COMPILE_OPTIONS ${size_options}
    LINK_OPTIONS ${size_options}
    MODULES
        yup::yup_core
        yup::yup_events
        yup::yup_graphics
        yup::yup_gui
        ${shader_libraries}
        ${target_name}_snippets)

# ==== Prepare sources
file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/source/*.h" "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp" "${shaders_dir}/*")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})

# ==== Hand the module over to the website, which serves website/src/public as its root
set (website_public_dir "${CMAKE_CURRENT_LIST_DIR}/../public/yup")
add_custom_command (
    TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${website_public_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy
        "$<TARGET_FILE_DIR:${target_name}>/${target_name}.js"
        "$<TARGET_FILE_DIR:${target_name}>/${target_name}.wasm"
        "${website_public_dir}")
