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
    set (additional_libraries "yup_audio_plugin_client")
    set (additional_link_options "")

    # ==== Find dependencies
    include (FetchContent)

    if (NOT "${yup_platform}" MATCHES "^(emscripten)$")
        _yup_fetch_sdl2()
        list (APPEND additional_libraries sdl2::sdl2)

        # ==== Fetch plugins SDKS
        if (YUP_ARG_PLUGIN_CREATE_CLAP)
            FetchContent_Declare (clap GIT_REPOSITORY https://github.com/free-audio/clap.git GIT_TAG main)
            FetchContent_MakeAvailable (clap)
            set_target_properties (clap-tests PROPERTIES FOLDER "Tests")

            _yup_module_setup_plugin_client_clap (${target_name} yup_audio_plugin_client ${YUP_ARG_TARGET_IDE_GROUP} ${YUP_ARG_UNPARSED_ARGUMENTS})

            list (APPEND additional_libraries clap ${target_name}_clap)
        endif()

        if (YUP_ARG_PLUGIN_CREATE_VST3)
            set (SMTG_ADD_VST3_UTILITIES OFF)
            set (SMTG_CREATE_MODULE_INFO ON)
            set (SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
            set (SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
            set (SMTG_ENABLE_VSTGUI_SUPPORT OFF)
            FetchContent_Declare (vst3sdk GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git GIT_TAG master GIT_SUBMODULES_RECURSE ON)
            FetchContent_MakeAvailable (vst3sdk)

            smtg_enable_vst3_sdk()

            _yup_module_setup_plugin_client_vst3 (${target_name} yup_audio_plugin_client ${YUP_ARG_TARGET_IDE_GROUP} ${YUP_ARG_UNPARSED_ARGUMENTS})

            list (APPEND additional_libraries base sdk pluginterfaces ${target_name}_vst3)
        endif()

        if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT YUP_ARG_PLUGIN_CREATE_VST3 AND NOT YUP_ARG_PLUGIN_CREATE_STANDALONE) #  AND NOT YUP_ARG_PLUGIN_CREATE_VST3 ...
            _yup_message (FATAL_ERROR "No valid plugin target defined")
        endif()
    endif()

    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        list (APPEND additional_definitions YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1)
    endif()

    # ==== Prepare shared binary
    add_library (${target_name} SHARED)
    target_compile_features (${target_name} PRIVATE cxx_std_17)

    # ==== Per platform configuration
    if ("${yup_platform}" MATCHES "^(osx)$")
        #    set_target_properties (${target_name} PROPERTIES
        #        BUNDLE ON
        #        CXX_EXTENSIONS OFF
        #        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kunitoki.yup.${target_name}"
        #        MACOSX_BUNDLE_NAME "${target_name}"
        #        MACOSX_BUNDLE_VERSION "1.0.0"
        #        #MACOSX_BUNDLE_ICON_FILE "Icon.icns"
        #        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms/${yup_platform}/Info.plist"
        #        #RESOURCE "${RESOURCE_FILES}"
        #        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
        #        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED OFF
        #        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf
        #        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
        #        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME OFF)

        set_target_properties (${target_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC OFF
            XCODE_GENERATE_SCHEME ON)

    endif()

    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        set_target_properties (${target_name} PROPERTIES SUFFIX ".clap")
    endif()

    if (YUP_ARG_PLUGIN_CREATE_VST3)
        set_target_properties (${target_name} PROPERTIES SUFFIX ".vst3")
        smtg_target_configure_version_file (${target_name})
    endif()

    if (YUP_ARG_TARGET_IDE_GROUP)
        set_target_properties (${target_name} PROPERTIES FOLDER "${YUP_ARG_TARGET_IDE_GROUP}")
    endif()

    # ==== Definitions and link libraries
    target_compile_options (${target_name} PRIVATE
        ${additional_options}
        ${YUP_ARG_OPTIONS})

    target_compile_definitions (${target_name} PRIVATE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        ${additional_definitions}
        ${YUP_ARG_DEFINITIONS})

    target_link_options (${target_name} PRIVATE
        ${additional_link_options}
        ${YUP_ARG_LINK_OPTIONS})

    target_link_libraries (${target_name} PRIVATE
        ${additional_libraries}
        ${YUP_ARG_MODULES})

endfunction()
