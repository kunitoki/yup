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
set (target_name example_docsimg)
set (target_version "2.0.1")

project (${target_name} VERSION ${target_version})

# ==== Prepare target
# Renders the figures of the documentation into docs/_static/images, see `just docs_images`.
# The modules are compiled into the target: changing this list rebuilds all of it.
# glslang and spirv_cross compile the GLSL of the shader figures when they are drawn.
yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Examples"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    DEFINITIONS
        "YUP_DOCSIMG_PATH=\"${CMAKE_CURRENT_LIST_DIR}\""
    MODULES
        yup::yup_core
        yup::yup_events
        yup::yup_graphics
        yup::yup_gui
        yup::yup_gui_markup
        yup::yup_audio_gui
        yup::yup_renderer
        tinygltf
        pffft_library
        libjpeg
        libpng
        glslang
        spirv_cross)

# ==== Prepare sources
# Listed rather than globbed: `just docs_images` regenerates the project when this file changes.
set (sources
    "${CMAKE_CURRENT_LIST_DIR}/source/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/Figures.h"
    "${CMAKE_CURRENT_LIST_DIR}/source/GraphicsFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/ImagingFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/LayoutFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/WidgetFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/DspFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/AudioFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/RhiFigures.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/source/Scene3DFigures.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
