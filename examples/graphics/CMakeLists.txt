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

cmake_minimum_required (VERSION 3.31)

# ==== Set target name and version
set (target_name example_graphics)
set (target_version "2.0.0")

project (${target_name} VERSION ${target_version})

# ==== Declare the demos and what each one brings on top of the base modules:
#   MODULES       extra modules to link (their dependencies are resolved automatically)
#   RESOURCES     folders of data/ to bundle on mobile and WebAssembly
#   SHADERS       bundles precompiled from data/shaders: <name>.comp, or <name>.vert + <name>.frag;
#                 <name>:<vertex> takes the vertex stage from <vertex>.vert instead, and
#                 data/shaders/*.glsl can be #included
#   LIVE_SHADERS  links the GLSL transpiler, for demos compiling shaders at runtime
set (all_demo_ids "")
macro (graphics_demo demo_id)
    cmake_parse_arguments (DEMO "LIVE_SHADERS" "" "MODULES;RESOURCES;SHADERS" ${ARGN})
    list (APPEND all_demo_ids ${demo_id})
    set (demo_${demo_id}_modules ${DEMO_MODULES})
    set (demo_${demo_id}_resources ${DEMO_RESOURCES})
    set (demo_${demo_id}_shaders ${DEMO_SHADERS})
    set (demo_${demo_id}_live_shaders ${DEMO_LIVE_SHADERS})
endmacro()

set (base_modules yup::yup_core yup::yup_events yup::yup_graphics yup::yup_gui libpng)
set (audio_modules yup::yup_audio_gui pffft_library)

graphics_demo (AI MODULES yup::yup_ai)
graphics_demo (Artboard RESOURCES rive)
graphics_demo (ArtboardLayout RESOURCES rive)
graphics_demo (Audio MODULES ${audio_modules} SHADERS synth_waveform:fullscreen)
graphics_demo (AudioFile MODULES ${audio_modules} bungee_library dr_libs flac_library hmp3_library libvorbis opus_library)
graphics_demo (Clipboard)
graphics_demo (ColorLab)
graphics_demo (Component3D SHADERS component3d)
graphics_demo (ComponentEffects SHADERS
    effect_blur:fullscreen effect_crt:fullscreen effect_edge:fullscreen
    effect_pixelate:fullscreen effect_sharpen:fullscreen effect_wave:fullscreen)
graphics_demo (ComputeParticles SHADERS particles_update particles_draw)
graphics_demo (Convolution MODULES ${audio_modules} dr_libs flac_library RESOURCES audio)
graphics_demo (Crossover MODULES ${audio_modules} dr_libs RESOURCES audio)
graphics_demo (CodeEditor)
graphics_demo (DragAndDrop)
graphics_demo (FileChooser)
graphics_demo (Filter MODULES ${audio_modules})
graphics_demo (FluidSimulation SHADERS
    fluid_advect_dye:fluid_fullscreen fluid_advect_velocity:fluid_fullscreen fluid_bloom_prefilter:fluid_fullscreen
    fluid_blur4:fluid_fullscreen fluid_clear:fluid_fullscreen fluid_curl:fluid_fullscreen
    fluid_display:fluid_fullscreen fluid_gradient_subtract:fluid_fullscreen fluid_pressure:fluid_fullscreen
    fluid_splat_dye:fluid_fullscreen fluid_splat_velocity:fluid_fullscreen fluid_vorticity:fluid_fullscreen)
graphics_demo (GpuAudio MODULES ${audio_modules} dr_libs RESOURCES audio LIVE_SHADERS)
graphics_demo (Images MODULES libgif libjpeg libtiff libwebp)
graphics_demo (Layout)
graphics_demo (LayoutFonts)
graphics_demo (Lottie MODULES yup::yup_animation RESOURCES lottie)
graphics_demo (OffscreenRender)
graphics_demo (Opaque)
graphics_demo (PaintProfiler)
graphics_demo (Paths)
graphics_demo (Pbr SHADERS
    pbr_background:pbr_scene pbr_brdf:pbr_fullscreen pbr_irradiance:pbr_fullscreen
    pbr_prefilter:pbr_fullscreen pbr_scene pbr_sky:pbr_fullscreen)
graphics_demo (PopupMenu)
graphics_demo (ScrollBar)
graphics_demo (Sliders)
graphics_demo (SpectrumAnalyzer MODULES ${audio_modules} dr_libs RESOURCES audio)
graphics_demo (SpinningCube MODULES yup::yup_animation RESOURCES lottie LIVE_SHADERS)
graphics_demo (Svg RESOURCES svg)
graphics_demo (TextEditor)
graphics_demo (ToastNotification)
graphics_demo (TouchTrails)
graphics_demo (VariableFonts)
graphics_demo (Widgets)
if (YUP_PLATFORM_DESKTOP OR YUP_PLATFORM_EMSCRIPTEN)
    graphics_demo (YdspSynths MODULES yup::yup_dsp_jit ${audio_modules} dr_libs RESOURCES audio synths)
endif()
if (YUP_PLATFORM_DESKTOP AND NOT YUP_PLATFORM_WINDOWS AND NOT YUP_PLATFORM_EMSCRIPTEN)
    graphics_demo (Python MODULES yup::yup_python)
endif()

# ==== Select which demo(s) to build
# Set to "ALL" (default) to build the full demo browser with every example. Set to a single demo:
#   cmake -DYUP_EXAMPLE_GRAPHICS_DEMO=SpinningCube ...
set (YUP_EXAMPLE_GRAPHICS_DEMO "ALL" CACHE STRING "Graphics demo to build: ALL, or one of ${all_demo_ids}")

if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL")
    set (selected_demo_ids ${all_demo_ids})
elseif (YUP_EXAMPLE_GRAPHICS_DEMO IN_LIST all_demo_ids)
    set (selected_demo_ids ${YUP_EXAMPLE_GRAPHICS_DEMO})
else()
    message (FATAL_ERROR "YUP_EXAMPLE_GRAPHICS_DEMO must be ALL or one of the demos available on this platform: ${all_demo_ids}")
endif()

set (demo_definitions "")
set (demo_modules "")
set (demo_resources "")
set (demo_shaders "")
set (demo_live_shaders OFF)
foreach (demo_id IN LISTS all_demo_ids)
    if (NOT demo_id IN_LIST selected_demo_ids)
        list (APPEND demo_definitions "YUP_EXAMPLE_GRAPHICS_DEMO_${demo_id}=0")
        continue()
    endif()

    list (APPEND demo_definitions "YUP_EXAMPLE_GRAPHICS_DEMO_${demo_id}=1")
    list (APPEND demo_modules ${demo_${demo_id}_modules})
    list (APPEND demo_resources ${demo_${demo_id}_resources})
    list (APPEND demo_shaders ${demo_${demo_id}_shaders})
    if (demo_${demo_id}_live_shaders)
        set (demo_live_shaders ON)
    endif()
endforeach()

list (REMOVE_DUPLICATES demo_modules)
list (REMOVE_DUPLICATES demo_resources)
list (REMOVE_DUPLICATES demo_shaders)

if (demo_live_shaders)
    list (APPEND demo_modules glslang spirv_cross spirv_tools)
endif()

# ==== Prepare Android build
set (link_libraries "")
if (ANDROID)
    include (../../cmake/yup.cmake)
    yup_add_default_modules ("${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

# ==== Bundle data files on platforms without access to the source tree at runtime, read back
#      at runtime through File::getSpecialLocation (File::bundleDirectory) - see getAssetPath()
set (bundle_resources "")
if (YUP_PLATFORM_MOBILE OR YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND bundle_resources "${CMAKE_CURRENT_LIST_DIR}/data/logo.png@data/logo.png")
    foreach (resource IN LISTS demo_resources)
        list (APPEND bundle_resources "${CMAKE_CURRENT_LIST_DIR}/data/${resource}@data/${resource}")
    endforeach()
endif()

# ==== Precompile shaders, bundled like the data files and read from the build tree on desktop
#      - see loadShaderBundle()
set (shaders_dir "${CMAKE_CURRENT_LIST_DIR}/data/shaders")
file (GLOB shader_includes "${shaders_dir}/*.glsl")
foreach (shader IN LISTS demo_shaders)
    string (REPLACE ":" ";" shader_parts "${shader}")
    list (GET shader_parts 0 shader_name)
    list (GET shader_parts -1 vertex_name)

    if (EXISTS "${shaders_dir}/${shader_name}.comp")
        set (shader_stages COMPUTE "${shaders_dir}/${shader_name}.comp")
    else()
        set (shader_stages VERT "${shaders_dir}/${vertex_name}.vert" FRAG "${shaders_dir}/${shader_name}.frag")
    endif()

    yup_add_shader_bundle (${shader_name}
        ${shader_stages}
        BUNDLE_RESOURCE shader_resource
        BUNDLE_DESTINATION "data/shaders/${shader_name}.ysl"
        DEPENDS ${shader_includes}
        OPTIONS "-I${shaders_dir}")

    if (YUP_PLATFORM_MOBILE OR YUP_PLATFORM_EMSCRIPTEN)
        list (APPEND bundle_resources "${shader_resource}")
    endif()
endforeach()

# ==== Embed shaders
if ("SpinningCube" IN_LIST selected_demo_ids)
    yup_add_shader_bundle (
        "${target_name}_binary_shaders"
        VERT ${CMAKE_CURRENT_LIST_DIR}/data/shaders/cube.vert
        FRAG ${CMAKE_CURRENT_LIST_DIR}/data/shaders/cube.frag
        NAMESPACE yup
        OUTPUT_NAME
            ShaderBundle
        RESOURCE_NAME
            ShaderBundleFile)
    list (APPEND link_libraries "${target_name}_binary_shaders")
endif()

# ==== Prepare target
set (additional_definitions
    ${demo_definitions}
    "YUP_EXAMPLE_GRAPHICS_SHADERS_PATH=\"${CMAKE_CURRENT_BINARY_DIR}\"")

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    PTHREAD_POOL_SIZE 16
    STACK_SIZE 4194304
    DEFINITIONS
        ${additional_definitions}
    BUNDLE_RESOURCES
        ${bundle_resources}
    MODULES
        ${base_modules}
        ${demo_modules}
        ${link_libraries})

# ==== Prepare sources
if (NOT YUP_TARGET_ANDROID)
    file (GLOB_RECURSE sources
        "${CMAKE_CURRENT_LIST_DIR}/source/examples/*.h"
        "${CMAKE_CURRENT_LIST_DIR}/source/main.cpp")
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
    target_sources (${target_name} PRIVATE ${sources})
endif()
