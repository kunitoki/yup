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
set (target_name yup_dsp_compiler)
set (target_version "2.0.0")

project (${target_name} VERSION 2.0.0)

# ==== Bootstrap YUP modules (this is always built as a standalone host tool)
get_filename_component (yup_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set (YUP_FETCH_UPSTREAM_MODULES ON CACHE BOOL "Fetch missing YUP module upstream sources" FORCE)
include ("${yup_root}/cmake/yup.cmake")

# Only add the modules actually needed by the dsp compiler.
yup_add_module ("${yup_root}/modules/yup_core" "" "Modules")
yup_add_module ("${yup_root}/modules/yup_dsp" "" "Modules")
yup_add_module ("${yup_root}/modules/yup_audio_basics" "" "Modules")
yup_add_module ("${yup_root}/modules/yup_dsp_jit" "" "Modules")
yup_add_module ("${yup_root}/modules/yup_simd" "" "Modules")
yup_add_module ("${yup_root}/thirdparty/asmjit_library" "" "Thirdparty")
yup_add_module ("${yup_root}/thirdparty/xsimd" "" "Thirdparty")
yup_add_module ("${yup_root}/thirdparty/zlib" "" "Thirdparty")

add_library (yup::yup_core ALIAS yup_core)
add_library (yup::yup_dsp ALIAS yup_dsp)
add_library (yup::yup_audio_basics ALIAS yup_audio_basics)
add_library (yup::yup_dsp_jit ALIAS yup_dsp_jit)
add_library (yup::yup_simd ALIAS yup_simd)

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
        yup::yup_dsp
        yup::yup_audio_basics
        yup::yup_simd
        yup::yup_dsp_jit
        asmjit_library
        xsimd)

# ==== Prepare sources
file (GLOB_RECURSE sources "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
target_sources (${target_name} PRIVATE ${sources})
