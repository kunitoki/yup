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

cmake_minimum_required(VERSION 3.31)

enable_testing()

option (YUP_TEST_ALLOCATION_HOOKS "Enable allocation-count assertions in realtime tests" OFF)

# ==== Setup googletests
if (YUP_PLATFORM_EMSCRIPTEN)
    set (gtest_disable_pthreads ON CACHE BOOL "" FORCE)
    set (gtest_disable_warnings ON CACHE BOOL "" FORCE)
    set (gtest_hide_internal_symbols ON CACHE BOOL "" FORCE)
    add_definitions (-DTEST_HAS_PTHREAD=0)
endif()

set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

_yup_fetchcontent_declare (googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)

FetchContent_MakeAvailable (googletest)

if (YUP_PLATFORM_EMSCRIPTEN)
    target_compile_options (gmock PRIVATE -pthread)
    target_compile_options (gtest PRIVATE -pthread)
endif()

set_target_properties (gtest PROPERTIES FOLDER "Tests")
set_target_properties (gtest_main PROPERTIES FOLDER "Tests")
set_target_properties (gmock PROPERTIES FOLDER "Tests")
set_target_properties (gmock_main PROPERTIES FOLDER "Tests")

# ==== Create executable
set (target_name yup_tests)
set (target_version "2.0.0")
set (target_console OFF)
set (target_gtest_modules "")
set (target_modules
    yup_core
    yup_simd
    yup_audio_basics
    yup_audio_devices
    yup_audio_formats
    yup_audio_processors
    yup_audio_graph
    yup_ai
    yup_dsp
    yup_events
    yup_data_model
    yup_graphics
    yup_animation
    yup_rhi
    yup_shading
    dr_libs
    pffft_library
    opus_library
    flac_library
    hmp3_library
    bungee_library
    sqlite3_library
    libpng
    libwebp
    libjpeg
    libgif
    libtiff
    luau
    glslang
    spirv_cross)

set (target_gtest_modules
    GTest::gtest
    GTest::gmock)

if (YUP_PLATFORM_DESKTOP OR YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND target_modules
        yup_dsp_jit)

    if (YUP_PLATFORM_DESKTOP)
        list (APPEND target_modules
            yup_audio_plugin_host
            yup_audio_plugin_client)
    endif()
endif()

if (NOT YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND target_modules yup_gui yup_audio_gui)
    if (YUP_PLATFORM_DESKTOP AND NOT YUP_PLATFORM_WINDOWS)
        list (APPEND target_modules yup_python)
    endif()
else()
    set (target_console ON)
endif()

set (target_bundle_resources "")
if (YUP_PLATFORM_MOBILE OR YUP_PLATFORM_EMSCRIPTEN)
    list (APPEND target_bundle_resources
        "${CMAKE_CURRENT_LIST_DIR}/data@data"
        "${CMAKE_CURRENT_LIST_DIR}/../examples/graphics/data/synths@data/synths")
endif()

# ==== Restrict the modules under test (used by the smart CI workflow).
#      Empty builds every module above; otherwise only the listed modules have
#      their tests compiled, linking just what they and the test runner need.
set (YUP_TEST_MODULES "" CACHE STRING "Semicolon-separated list of modules to test; empty builds all")

set (target_test_modules ${target_modules})
if (YUP_TEST_MODULES)
    set (platform_modules ${target_modules})

    set (target_test_modules "")
    foreach (test_module IN LISTS YUP_TEST_MODULES)
        if (test_module IN_LIST platform_modules)
            list (APPEND target_test_modules ${test_module})
        elseif (test_module)
            _yup_message (WARNING "Skipping YUP_TEST_MODULES entry '${test_module}' (not available on this platform)")
        endif()
    endforeach()

    # The test runner needs the application layer; optional and test deps are
    # linked so guarded features and cross-module tests compile in.
    if (YUP_PLATFORM_EMSCRIPTEN)
        set (target_modules yup_events)
    else()
        set (target_modules yup_gui)
    endif()

    foreach (test_module IN LISTS target_test_modules)
        list (APPEND target_modules ${test_module})

        get_target_property (optional_modules ${test_module} YUP_MODULE_OPTIONAL_DEPENDENCIES)
        get_target_property (test_dependency_modules ${test_module} YUP_MODULE_TEST_DEPENDENCIES)
        foreach (extra_module IN LISTS optional_modules test_dependency_modules)
            if (TARGET ${extra_module} AND (NOT extra_module MATCHES "^yup_" OR extra_module IN_LIST platform_modules))
                list (APPEND target_modules ${extra_module})
            endif()
        endforeach()
    endforeach()

    list (REMOVE_DUPLICATES target_modules)
endif()

if (YUP_PLATFORM_MAC AND "yup_python" IN_LIST target_modules)
    set (python_tools_path "${CMAKE_CURRENT_LIST_DIR}/../python/tools")
    set (python_target_name "${target_name}_python_stdlib")

    yup_prepare_python_stdlib (${python_target_name} ${python_tools_path} python_standard_library)

    yup_add_embedded_binary_resources (
        ${target_name}_python_stdlib_binary
        OUT_DIR PythonStandardLibrary
        HEADER PythonStandardLibrary.h
        NAMESPACE yup
        RESOURCE_NAMES
            PythonStandardLibrary
        RESOURCES
            ${python_standard_library})

    list (APPEND target_modules "${target_name}_python_stdlib_binary")
endif()

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Tests"
    TARGET_CONSOLE ${target_console}
    INITIAL_MEMORY 268435456   # 256MB: enlarged for wasm pthread stress tests (CI stability)
    MAXIMUM_MEMORY 1073741824  # 1GB:  cap for ALLOW_MEMORY_GROWTH under node/wasm
    STACK_SIZE 1048576         # 1MB:  per-thread stack for concurrent stress tests
    DEFINITIONS
        YUP_ENABLE_ALLOCATION_HOOKS=$<BOOL:${YUP_TEST_ALLOCATION_HOOKS}>
        YUP_USE_CURL=0
        YUP_MODAL_LOOPS_PERMITTED=1
    BUNDLE_RESOURCES
        ${target_bundle_resources}
    MODULES
        ${target_modules}
        ${target_gtest_modules})

# ==== (Only For Manual Testing) Setup FFTW3
# _yup_find_fftw3 (${target_name})

# ==== Setup sources
set (sources "")
set (imported_sources "")
foreach (module ${target_test_modules})
    set (module_main "${CMAKE_CURRENT_LIST_DIR}/${module}/module.cpp")
    file (GLOB module_sources "${module_main}")
    list (APPEND sources ${module_sources})

    file (GLOB module_imported_sources "${CMAKE_CURRENT_LIST_DIR}/${module}/*.cpp")
    list (APPEND imported_sources ${module_imported_sources})
    list (REMOVE_ITEM imported_sources "${module_main}")

    if (YUP_PLATFORM_APPLE)
        file (GLOB_RECURSE apple_sources "${CMAKE_CURRENT_LIST_DIR}/${module}/module.mm")
        list (APPEND sources ${apple_sources})

        set (apple_main "${CMAKE_CURRENT_LIST_DIR}/${module}/module.mm")
        file (GLOB_RECURSE apple_imported_sources "${apple_main}")
        list (APPEND imported_sources ${apple_imported_sources})
        list (REMOVE_ITEM imported_sources "${apple_main}")
    endif()
endforeach()

list (APPEND sources "${CMAKE_CURRENT_LIST_DIR}/main.cpp")

target_sources (${target_name} PRIVATE ${sources} ${imported_sources})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})
source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${imported_sources})
set_source_files_properties (${imported_sources} PROPERTIES HEADER_FILE_ONLY TRUE)

# ==== Plugin wrapper standalone test TUs (compiled directly, not HEADER_FILE_ONLY)
if (YUP_PLATFORM_DESKTOP AND "yup_audio_plugin_client" IN_LIST target_test_modules)
    if (TARGET clap)
        target_link_libraries (${target_name} PRIVATE clap)
        target_sources (${target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_clap.cpp")
    endif()

    if (TARGET sdk)
        target_link_libraries (${target_name} PRIVATE sdk)
        target_sources (${target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_vst3.cpp")
        if (TARGET sdk_hosting)
            target_link_libraries (${target_name} PRIVATE sdk_hosting)
        endif()
    endif()

    if (YUP_PLATFORM_MAC AND TARGET base-sdk-auv2)
        target_link_libraries (${target_name} PRIVATE base-sdk-auv2)
        target_sources (${target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_au.mm")
        set_source_files_properties (
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_au.mm"
            PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    endif()

    if (YUP_PLATFORM_MAC)
        target_sources (${target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_auv3.mm")
        set_source_files_properties (
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_auv3.mm"
            PROPERTIES COMPILE_FLAGS "-fobjc-arc")
        target_link_libraries (${target_name} PRIVATE "-framework CoreAudioKit")
    endif()
endif()

target_compile_options (${target_name} PRIVATE
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-subobject-linkage>)

set_target_properties (${target_name} PROPERTIES
    XCODE_SCHEME_ARGUMENTS "--gtest_filter=*"
    VS_DEBUGGER_COMMAND_ARGUMENTS "--gtest_filter=*")

# ==== Setup coverage support
if (YUP_ENABLE_COVERAGE)
    _yup_setup_coverage_flags (${target_name})
    foreach (module ${target_modules})
        _yup_setup_coverage_flags (${module})
    endforeach()

    _yup_setup_coverage_targets (${target_name} "${target_modules}")
endif()
