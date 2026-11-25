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

# ==== Prepare target
set (target_name yup_yui_tool)
set (target_version "2.0.1")

project (${target_name} VERSION ${target_version})

# ==== Bootstrap YUP modules (this is always built on its own: as a host tool, or for the web as the module an editor runs)
get_filename_component (yup_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set (YUP_FETCH_UPSTREAM_MODULES ON CACHE BOOL "Fetch missing YUP module upstream sources" FORCE)
set (YUP_ENABLE_EMSCRIPTEN_THREADS OFF CACHE BOOL "An editor runs the module in pages that can't be cross-origin isolated" FORCE)
include ("${yup_root}/cmake/yup.cmake")

# The tool built for the web compiles the shaders written in GLSL as the native one does, which glslang and spirv_cross make
# about half as large again. Without them it only loads the shaders a build compiled into .ysl bundles
option (YUP_YUI_TOOL_WEB_SHADER_COMPILER "Compile the shaders written in GLSL in the tool built for the web" ON)

# Only add yup_gui and the modules it depends on.
set (yup_modules
    yup_core
    yup_events
    yup_data_model
    yup_expression
    yup_simd
    yup_shading
    yup_rhi
    yup_graphics
    yup_gui
    yup_3d
    yup_renderer)

set (thirdparty_modules
    zlib
    xsimd
    lzav_library
    xxhash_library
    libclipper2
    libpng
    rive
    rive_decoders
    harfbuzz
    sheenbidi_library
    yoga_library
    libhydrogen
    tinygltf
    luau
    glad
    vulkan_library)

# The tool loads what a development build of an application loads: the 3D scenes of yup_renderer with their models and
# their textures, and the shaders written in GLSL, which glslang and spirv_cross make yup_shading compile
list (APPEND thirdparty_modules libjpeg libwebp)

set (shader_compiler_modules glslang spirv_cross)
if (YUP_PLATFORM_EMSCRIPTEN AND NOT YUP_YUI_TOOL_WEB_SHADER_COMPILER)
    set (shader_compiler_modules "")
endif()

list (APPEND thirdparty_modules ${shader_compiler_modules})

# It serves agents by itself with yup_ai, which the module a host calls into has no use for
if (NOT YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND yup_modules yup_ai)
endif()

foreach (module IN LISTS thirdparty_modules)
    yup_add_module ("${yup_root}/thirdparty/${module}" "" "Thirdparty")
endforeach()

foreach (module IN LISTS yup_modules)
    yup_add_module ("${yup_root}/modules/${module}" "" "Modules")
    add_library (yup::${module} ALIAS ${module})
endforeach()

# ==== Prepare sources: the commands are the same everywhere, the entry point is the one of the platform
file (GLOB sources "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")

# ==== Prepare target
if (YUP_PLATFORM_EMSCRIPTEN)
    # For the web the tool is a module a host calls into, such as an editor extension: it runs the commands
    # on files given in memory, and is the application showing an interface live in the canvas of a page
    list (REMOVE_ITEM sources "${CMAKE_CURRENT_LIST_DIR}/source/main.cpp" "${CMAKE_CURRENT_LIST_DIR}/source/yup_YuiMcpCommand.cpp")
    list (APPEND sources "${CMAKE_CURRENT_LIST_DIR}/web/main.cpp")

    # A material is compiled around the template of yup_renderer, which reads it from its folder of shaders. The
    # module has no disk, so the template and the library it includes are files of its own, at the path it is built from
    set (material_shader_sources "")
    set (material_shader_options "")

    if (YUP_YUI_TOOL_WEB_SHADER_COMPILER)
        set (renderer_shaders "${yup_root}/modules/yup_renderer/shaders")
        file (GLOB material_shader_sources "${renderer_shaders}/yup_MaterialShader.*" "${renderer_shaders}/lib/*.glsl")

        foreach (source IN LISTS material_shader_sources)
            list (APPEND material_shader_options "--embed-file=${source}@${source}")
        endforeach()
    endif()

    yup_standalone_app (
        TARGET_NAME ${target_name}
        TARGET_VERSION ${target_version}
        TARGET_IDE_GROUP "Tools"
        TARGET_APP_ID "audio.yup.${target_name}"
        TARGET_APP_NAMESPACE "audio.yup"
        ENABLE_EMSCRIPTEN_WEBGPU OFF
        LINK_OPTIONS
            -sMODULARIZE=1
            -sEXPORT_NAME=createYuiTool
            -sNODEJS_CATCH_EXIT=0
            -sNODEJS_CATCH_REJECTION=0
            ${material_shader_options}
        MODULES
            yup::yup_core
            yup::yup_events
            yup::yup_graphics
            yup::yup_gui
            yup::yup_3d
            yup::yup_renderer
            tinygltf
            libpng
            libjpeg
            libwebp
            ${shader_compiler_modules})

    # The host loads the script by itself, with no page around it
    set_target_properties (${target_name} PROPERTIES SUFFIX ".js")
    set_property (TARGET ${target_name} APPEND PROPERTY LINK_DEPENDS ${material_shader_sources})
else()
    yup_standalone_app (
        TARGET_NAME ${target_name}
        TARGET_VERSION ${target_version}
        TARGET_IDE_GROUP "Tools"
        TARGET_APP_ID "audio.yup.${target_name}"
        TARGET_APP_NAMESPACE "audio.yup"
        TARGET_CONSOLE ON
        MODULES
            yup::yup_core
            yup::yup_events
            yup::yup_graphics
            yup::yup_gui
            yup::yup_ai
            yup::yup_3d
            yup::yup_renderer
            tinygltf
            libpng
            libjpeg
            libwebp
            glslang
            spirv_cross)

    # yup_gui always compiles its SDL windowing, which a console application doesn't link by itself
    _yup_fetch_sdl()
    target_link_libraries (${target_name} PRIVATE sdl::sdl)
endif()

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
target_compile_definitions (${target_name} PRIVATE YUP_YUI_TOOL_VERSION="${target_version}")
