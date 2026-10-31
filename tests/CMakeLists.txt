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
set (YUP_TEST_LINK_JOBS 2 CACHE STRING "Maximum number of test runners linked at once by Ninja generators (0 for no limit)")
set (YUP_TEST_WASM_DEBUG_INFO NAMES CACHE STRING "Debug information of the Emscripten Debug test runners: NAMES (fast link) or FULL (DWARF and source maps)")
set_property (CACHE YUP_TEST_WASM_DEBUG_INFO PROPERTY STRINGS NAMES FULL)

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

# ==== Modules linked by the test runners
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
    list (APPEND target_modules yup_gui yup_audio_gui yup_3d yup_renderer tinygltf)
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

    # Linked modules (and their required deps) also get their third-party optional
    # deps, so guarded features match a full build (e.g. dr_libs for WAV reading).
    set (modules_to_visit ${target_modules})
    set (visited_modules "")
    while (modules_to_visit)
        list (POP_FRONT modules_to_visit current_module)
        if (current_module IN_LIST visited_modules OR NOT TARGET ${current_module})
            continue()
        endif()
        list (APPEND visited_modules ${current_module})

        get_target_property (required_modules ${current_module} YUP_MODULE_DEPENDENCIES)
        get_target_property (optional_modules ${current_module} YUP_MODULE_OPTIONAL_DEPENDENCIES)
        list (APPEND modules_to_visit ${required_modules})
        foreach (extra_module IN LISTS optional_modules)
            if (TARGET ${extra_module} AND NOT extra_module MATCHES "^yup_")
                list (APPEND target_modules ${extra_module})
                list (APPEND modules_to_visit ${extra_module})
            endif()
        endforeach()
    endwhile()

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

# ==== Shared runtime: the linked modules are compiled once into a static library,
#      so every test runner only links the objects its tests actually reference.
set (runtime_target_name ${target_name}_runtime)

yup_standalone_app (
    TARGET_NAME ${runtime_target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Tests"
    TARGET_CONSOLE ${target_console}
    TARGET_STATIC_LIBRARY ON
    DEFINITIONS
        YUP_ENABLE_ALLOCATION_HOOKS=$<BOOL:${YUP_TEST_ALLOCATION_HOOKS}>
        YUP_USE_CURL=0
        YUP_MODAL_LOOPS_PERMITTED=1
    COMPILE_OPTIONS
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU>>:-Wno-subobject-linkage>
    MODULES
        ${target_modules}
        ${target_gtest_modules})

# ==== One test runner per module, all built by the yup_tests target and run by ctest
if (YUP_TEST_LINK_JOBS GREATER 0)
    set_property (GLOBAL APPEND PROPERTY JOB_POOLS ${target_name}_link=${YUP_TEST_LINK_JOBS})
endif()

add_custom_target (${target_name})
set_target_properties (${target_name} PROPERTIES FOLDER "Tests")

foreach (module ${target_test_modules})
    set (module_main "${CMAKE_CURRENT_LIST_DIR}/${module}/module.cpp")
    if (NOT EXISTS "${module_main}")
        continue()
    endif()

    set (module_target_name ${module}_tests)

    set (sources "${module_main}" "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
    if (YUP_PLATFORM_APPLE AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/${module}/module.mm")
        list (APPEND sources "${CMAKE_CURRENT_LIST_DIR}/${module}/module.mm")
    endif()

    file (GLOB imported_sources "${CMAKE_CURRENT_LIST_DIR}/${module}/*.cpp")
    list (REMOVE_ITEM imported_sources "${module_main}")

    yup_standalone_app (
        TARGET_NAME ${module_target_name}
        TARGET_VERSION ${target_version}
        TARGET_IDE_GROUP "Tests"
        TARGET_CONSOLE ${target_console}
        INITIAL_MEMORY 268435456   # 256MB: enlarged for wasm pthread stress tests (CI stability)
        MAXIMUM_MEMORY 1073741824  # 1GB:  cap for ALLOW_MEMORY_GROWTH under node/wasm
        STACK_SIZE 1048576         # 1MB:  per-thread stack for concurrent stress tests
        EMSCRIPTEN_DEBUG_INFO ${YUP_TEST_WASM_DEBUG_INFO}
        BUNDLE_RESOURCES
            ${target_bundle_resources}
        MODULES
            ${runtime_target_name})

    target_sources (${module_target_name} PRIVATE ${sources} ${imported_sources})
    source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources} ${imported_sources})
    set_source_files_properties (${imported_sources} PROPERTIES HEADER_FILE_ONLY TRUE)

    set_target_properties (${module_target_name} PROPERTIES
        XCODE_SCHEME_ARGUMENTS "--gtest_filter=*"
        VS_DEBUGGER_COMMAND_ARGUMENTS "--gtest_filter=*")

    if (YUP_TEST_LINK_JOBS GREATER 0)
        set_target_properties (${module_target_name} PROPERTIES JOB_POOL_LINK ${target_name}_link)
    endif()

    add_dependencies (${target_name} ${module_target_name})

    # Runs from the folder holding the runners, emscripten runners go through CMAKE_CROSSCOMPILING_EMULATOR (node)
    if (YUP_PLATFORM_MAC)
        set (module_working_directory "$<TARGET_BUNDLE_DIR:${module_target_name}>/..")
    else()
        set (module_working_directory "$<TARGET_FILE_DIR:${module_target_name}>")
    endif()

    add_test (NAME ${module} COMMAND ${module_target_name} WORKING_DIRECTORY "${module_working_directory}")
    set_tests_properties (${module} PROPERTIES LABELS "${module}")

    # Debug wasm frames are large and exceptions add JS trampolines: the default ~1MB V8 stack of node
    # overflows on deep recursion tests, so give the main thread 4MB of its 8MB native stack
    if (YUP_PLATFORM_EMSCRIPTEN AND CMAKE_CROSSCOMPILING_EMULATOR)
        set_target_properties (${module_target_name} PROPERTIES
            CROSSCOMPILING_EMULATOR "${CMAKE_CROSSCOMPILING_EMULATOR};--stack-size=4096")
    endif()
endforeach()

if (YUP_PLATFORM_APPLE)
    set_source_files_properties ("${CMAKE_CURRENT_LIST_DIR}/yup_rhi/module.mm" PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

# ==== Plugin wrapper standalone test TUs (compiled directly, not HEADER_FILE_ONLY)
set (plugin_client_target_name yup_audio_plugin_client_tests)
if (YUP_PLATFORM_DESKTOP AND TARGET ${plugin_client_target_name})
    if (TARGET clap)
        target_link_libraries (${plugin_client_target_name} PRIVATE clap)
        target_sources (${plugin_client_target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_clap.cpp")
    endif()

    target_sources (${plugin_client_target_name} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_standalone.cpp")
    set_source_files_properties (
        "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_standalone.cpp"
        PROPERTIES HEADER_FILE_ONLY FALSE)

    if (TARGET sdk)
        target_link_libraries (${plugin_client_target_name} PRIVATE sdk)
        target_sources (${plugin_client_target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_vst3.cpp")
        if (TARGET sdk_hosting)
            target_link_libraries (${plugin_client_target_name} PRIVATE sdk_hosting)
        endif()
    endif()

    if (YUP_PLATFORM_MAC AND TARGET base-sdk-auv2)
        target_link_libraries (${plugin_client_target_name} PRIVATE base-sdk-auv2)
        target_sources (${plugin_client_target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_au.mm")
        set_source_files_properties (
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_au.mm"
            PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    endif()

    if (YUP_PLATFORM_MAC)
        target_sources (${plugin_client_target_name} PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_auv3.mm")
        set_source_files_properties (
            "${CMAKE_CURRENT_LIST_DIR}/yup_audio_plugin_client/module_auv3.mm"
            PROPERTIES COMPILE_FLAGS "-fobjc-arc")
        target_link_libraries (${plugin_client_target_name} PRIVATE "-framework CoreAudioKit")
    endif()
endif()

# ==== Setup coverage support
if (YUP_ENABLE_COVERAGE)
    _yup_setup_coverage_flags (${runtime_target_name})
    foreach (module ${target_modules})
        _yup_setup_coverage_flags (${module})
    endforeach()

    _yup_setup_coverage_targets (${target_name} "${target_modules}")
endif()
