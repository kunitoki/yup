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
set (target_name yup_shader_bundler)
set (target_version "2.0.0")

project (${target_name} VERSION ${target_version})

# ==== Bootstrap YUP modules (this is always built as a standalone host tool)
get_filename_component (yup_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include (${yup_root}/cmake/yup.cmake)

# Only add the modules actually needed by the shader bundler tool.
# Adding all default modules would pull in audio modules that require host
# system packages (e.g. alsa, pulseaudio) which may not be available.
yup_add_module ("${yup_root}/thirdparty/zlib" "" "Thirdparty")
yup_add_module ("${yup_root}/thirdparty/glslang" "" "Thirdparty")
yup_add_module ("${yup_root}/thirdparty/spirv_cross" "" "Thirdparty")
yup_add_module ("${yup_root}/thirdparty/spirv_tools" "" "Thirdparty")
yup_add_module ("${yup_root}/modules/yup_core" "" "Modules")
yup_add_module ("${yup_root}/modules/yup_shading" "" "Modules")
add_library (yup::yup_core ALIAS yup_core)
add_library (yup::yup_shading ALIAS yup_shading)

# ==== Prepare target
yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Tools"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    TARGET_CONSOLE ON
    MODULES
        yup::yup_core
        yup::yup_shading
        glslang
        spirv_cross
        spirv_tools)

# ==== Prepare sources
file (GLOB_RECURSE sources "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
