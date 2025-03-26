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

#==============================================================================

function (yup_audio_plugin)
    # ==== Fetch options
    set (options CONSOLE)
    set (one_value_args TARGET_NAME TARGET_IDE_GROUP PLUGIN_CREATE_CLAP PLUGIN_CREATE_VST3 PLUGIN_CREATE_STANDALONE)
    set (multi_value_args DEFINITIONS MODULES LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    # ==== Validation stage
    if ("${yup_platform}" MATCHES "^(emscripten|android)$")
        _yup_message (FATAL_ERROR "Audio plugins are not supported on emscripten or android.")
    endif()

    if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT YUP_ARG_PLUGIN_CREATE_VST3 AND NOT YUP_ARG_PLUGIN_CREATE_STANDALONE)
        _yup_message (FATAL_ERROR "At least one plugin type must be enabled (CLAP, VST3, or Standalone).")
    endif()

    # ==== Create static library for user's plugin code
    _yup_message (STATUS "Creating static library for user's plugin code")
    add_library (${target_name}_shared INTERFACE)

    target_compile_features (${target_name}_shared INTERFACE cxx_std_17)

    target_compile_definitions (${target_name}_shared INTERFACE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        ${additional_definitions}
        ${YUP_ARG_DEFINITIONS})

    target_compile_options (${target_name}_shared INTERFACE
        ${additional_options}
        ${YUP_ARG_OPTIONS})

    target_link_libraries (${target_name}_shared INTERFACE
        ${additional_libraries}
        ${YUP_ARG_MODULES})

    set_target_properties (${target_name}_shared PROPERTIES
        FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC OFF
        XCODE_GENERATE_SCHEME ON)

    # ==== Find dependencies
    include (FetchContent)
    _yup_fetch_sdl2()
    list (APPEND additional_libraries sdl2::sdl2)

    # ==== Fetch clap SDK and build clap target
    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        _yup_message (STATUS "Fetching CLAP SDK")
        FetchContent_Declare (
            clap
            GIT_REPOSITORY https://github.com/free-audio/clap.git
            GIT_TAG main)
        FetchContent_MakeAvailable (clap)
        set_target_properties (clap-tests PROPERTIES FOLDER "Tests")

        _yup_message (STATUS "Setting up CLAP plugin client")
        _yup_module_setup_plugin_client (
            ${target_name}
            yup_audio_plugin_client
            ${YUP_ARG_TARGET_IDE_GROUP}
            clap
            ${YUP_ARG_UNPARSED_ARGUMENTS})

        # Create CLAP plugin target
        _yup_message (STATUS "Creating CLAP plugin target")
        add_library (${target_name}_clap_plugin SHARED)

        target_compile_features (${target_name}_clap_plugin PRIVATE cxx_std_17)

        target_compile_definitions (${target_name}_clap_plugin PRIVATE
            YUP_AUDIO_PLUGIN_ENABLE_CLAP=1
            JUCE_STANDALONE_APPLICATION=0)

        target_link_libraries (${target_name}_clap_plugin PRIVATE
            ${target_name}_shared
            yup_audio_plugin_client
            clap
            ${target_name}_clap
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        set_target_properties (${target_name}_clap_plugin PROPERTIES
            SUFFIX ".clap"
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)
    endif()

    # ==== Fetch vst3 SDK and build vst3 target
    if (YUP_ARG_PLUGIN_CREATE_VST3)
        _yup_message (STATUS "Fetching VST3 SDK")
        set (SMTG_ADD_VST3_UTILITIES OFF)
        set (SMTG_CREATE_MODULE_INFO ON)
        set (SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
        set (SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
        set (SMTG_ENABLE_VSTGUI_SUPPORT OFF)
        FetchContent_Declare (
            vst3sdk
            GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
            GIT_TAG master
            GIT_SUBMODULES_RECURSE ON)
        FetchContent_MakeAvailable (vst3sdk)

        _yup_message (STATUS "Setting up VST3 plugin client")
        smtg_enable_vst3_sdk()

        _yup_module_setup_plugin_client (
            ${target_name}
            yup_audio_plugin_client
            ${YUP_ARG_TARGET_IDE_GROUP}
            vst3
            ${YUP_ARG_UNPARSED_ARGUMENTS})

        # Create VST3 plugin target
        _yup_message (STATUS "Creating VST3 plugin target")
        add_library (${target_name}_vst3_plugin SHARED)

        target_compile_features (${target_name}_vst3_plugin PRIVATE cxx_std_17)

        target_compile_definitions (${target_name}_vst3_plugin PRIVATE
            YUP_AUDIO_PLUGIN_ENABLE_VST3=1
            JUCE_STANDALONE_APPLICATION=0)

        target_link_libraries (${target_name}_vst3_plugin PRIVATE
            ${target_name}_shared
            yup_audio_plugin_client
            base
            sdk
            pluginterfaces
            ${target_name}_vst3
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        set_target_properties (${target_name}_vst3_plugin PROPERTIES
            SUFFIX ".vst3"
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)

        smtg_target_configure_version_file (${target_name}_vst3_plugin)
    endif()

    # ==== Build standalone plugin target
    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        _yup_message (STATUS "Setting up standalone plugin client")
        _yup_module_setup_plugin_client (
            ${target_name}
            yup_audio_plugin_client
            ${YUP_ARG_TARGET_IDE_GROUP}
            standalone
            ${YUP_ARG_UNPARSED_ARGUMENTS})

        _yup_message (STATUS "Creating standalone plugin target")
        add_executable (${target_name}_app)

        target_compile_features (${target_name}_app PRIVATE cxx_std_17)

        target_compile_definitions (${target_name}_app PRIVATE
            YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1
            JUCE_STANDALONE_APPLICATION=1)

        target_link_libraries (${target_name}_app PRIVATE
            ${target_name}_shared
            ${target_name}_standalone
            yup_audio_plugin_client
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        set_target_properties (${target_name}_app PROPERTIES
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)

        if ("${yup_platform}" MATCHES "^(osx)$")
            set_target_properties (${target_name}_app PROPERTIES
                BUNDLE ON
                MACOSX_BUNDLE_GUI_IDENTIFIER "org.kunitoki.yup.${target_name}"
                MACOSX_BUNDLE_NAME "${target_name}"
                MACOSX_BUNDLE_VERSION "1.0.0"
                MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms/${yup_platform}/Info.plist"
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED OFF
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf
                XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
                XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME OFF)
        endif()
    endif()

endfunction()
