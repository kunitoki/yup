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

set (rive_file "data/rive/alien.riv")
set (lottie_file "data/lottie/goal.lottie")

# ==== Select which demo(s) to build
# Set to "ALL" (default) to build the full demo browser with every example. Set to a single demo:
#   cmake -DYUP_EXAMPLE_GRAPHICS_DEMO=SpinningCube ...
set (all_demo_ids
     AI Artboard Audio AudioFile Clipboard ColorLab ComponentEffects ComputeParticles
     Convolution Crossover CodeEditor FileChooser FilterDemo GpuAudio Images LayoutFonts
     Lottie OffscreenRender OpaqueDemo PaintProfiler Paths PopupMenu ScrollBar Sliders
     SpectrumAnalyzer SpinningCube Svg TextEditor VariableFonts Widgets YdspSynths Python)

set (YUP_EXAMPLE_GRAPHICS_DEMO "ALL" CACHE STRING "Graphics demo to build: ALL, or one of ${all_demo_ids}")

if (NOT YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" AND NOT YUP_EXAMPLE_GRAPHICS_DEMO IN_LIST all_demo_ids)
    message (FATAL_ERROR "YUP_EXAMPLE_GRAPHICS_DEMO must be ALL or one of: ${all_demo_ids}")
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "YdspSynths" AND NOT (YUP_PLATFORM_DESKTOP OR YUP_PLATFORM_EMSCRIPTEN))
    message (FATAL_ERROR "YUP_EXAMPLE_GRAPHICS_DEMO=YdspSynths requires yup_dsp_jit, not available on this platform")
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "Python" AND (NOT YUP_PLATFORM_DESKTOP OR YUP_PLATFORM_WINDOWS OR YUP_PLATFORM_EMSCRIPTEN))
    message (FATAL_ERROR "YUP_EXAMPLE_GRAPHICS_DEMO=Python requires yup_python, not available on this platform")
endif()

set (demo_definitions "")
foreach (demo_id IN LISTS all_demo_ids)
    if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL demo_id)
        list (APPEND demo_definitions "YUP_EXAMPLE_GRAPHICS_DEMO_${demo_id}=1")
    else()
        list (APPEND demo_definitions "YUP_EXAMPLE_GRAPHICS_DEMO_${demo_id}=0")
    endif()
endforeach()

# Which of the resources/modules below are actually needed by the selected demo(s).
set (need_rive_file OFF)
set (need_lottie_file OFF)
set (need_shader_bundle OFF)
set (need_synths_data OFF)
set (need_shader_transpiler OFF)
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "Artboard")
    set (need_rive_file ON)
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "Lottie" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "SpinningCube")
    set (need_lottie_file ON)
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "SpinningCube")
    set (need_shader_bundle ON)
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "YdspSynths")
    set (need_synths_data ON)
endif()
if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "SpinningCube" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "GpuAudio" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ComputeParticles")
    set (need_shader_transpiler ON)
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
    if (need_rive_file)
        list (APPEND bundle_resources "${CMAKE_CURRENT_LIST_DIR}/${rive_file}@${rive_file}")
    endif()
    if (need_lottie_file)
        list (APPEND bundle_resources "${CMAKE_CURRENT_LIST_DIR}/${lottie_file}@${lottie_file}")
    endif()
    if (need_synths_data)
        list (APPEND bundle_resources "${CMAKE_CURRENT_LIST_DIR}/data/synths@data/synths")
    endif()
endif()

# ==== Embed shaders
if (need_shader_bundle)
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
set (additional_modules "")
set (additional_definitions "${demo_definitions}")
if (YUP_PLATFORM_DESKTOP OR YUP_PLATFORM_EMSCRIPTEN)
    if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "YdspSynths")
        list (APPEND additional_modules yup::yup_dsp_jit)
    endif()
    if (NOT YUP_PLATFORM_WINDOWS AND NOT YUP_PLATFORM_EMSCRIPTEN)
        if (YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "ALL" OR YUP_EXAMPLE_GRAPHICS_DEMO STREQUAL "Python")
            list (APPEND additional_modules yup::yup_python)
        endif()
    endif()
endif()
if (need_shader_transpiler)
    list (APPEND additional_modules glslang spirv_cross spirv_tools)
endif()

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    PTHREAD_POOL_SIZE 16
    STACK_SIZE 4194304
    DEFINITIONS
        YUP_EXAMPLE_GRAPHICS_RIVE_FILE="${rive_file}"
        YUP_EXAMPLE_GRAPHICS_LOTTIE_FILE="${lottie_file}"
        ${additional_definitions}
    BUNDLE_RESOURCES
        ${bundle_resources}
    MODULES
        yup::yup_core
        yup::yup_audio_basics
        yup::yup_audio_devices
        yup::yup_dsp
        yup::yup_events
        yup::yup_graphics
        yup::yup_animation
        yup::yup_gui
        yup::yup_audio_gui
        yup::yup_audio_processors
        yup::yup_audio_formats
        yup::yup_shading
        yup::yup_ai
        bungee_library
        pffft_library
        opus_library
        flac_library
        hmp3_library
        dr_libs
        libvorbis
        libpng
        libwebp
        libjpeg
        libgif
        libtiff
        ${additional_modules}
        ${link_libraries})

# ==== Prepare sources
if (NOT YUP_TARGET_ANDROID)
    file (GLOB_RECURSE sources
        "${CMAKE_CURRENT_LIST_DIR}/source/examples/*.h"
        "${CMAKE_CURRENT_LIST_DIR}/source/main.cpp")
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
    target_sources (${target_name} PRIVATE ${sources})
endif()
