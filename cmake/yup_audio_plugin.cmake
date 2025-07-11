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

    set (one_value_args
        # Globals
        TARGET_NAME TARGET_VERSION TARGET_IDE_GROUP TARGET_APP_ID TARGET_APP_NAMESPACE TARGET_CXX_STANDARD
        # Plugin types
        PLUGIN_CREATE_CLAP PLUGIN_CREATE_VST3 PLUGIN_CREATE_STANDALONE)

    set (multi_value_args
        DEFINITIONS
        MODULES
        LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_set_default (YUP_ARG_TARGET_CXX_STANDARD 17)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_app_id "${YUP_ARG_TARGET_APP_ID}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    # ==== Validation stage
    if (NOT YUP_PLATFORM_DESKTOP)
        _yup_message (FATAL_ERROR "Audio plugins are not supported on emscripten or android.")
        return()
    endif()

    if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT YUP_ARG_PLUGIN_CREATE_VST3 AND NOT YUP_ARG_PLUGIN_CREATE_STANDALONE)
        _yup_message (FATAL_ERROR "At least one plugin type must be enabled (CLAP, VST3, or Standalone).")
        return()
    endif()

    # ==== Create static library for user's plugin code
    _yup_message (STATUS "Creating static library for user's plugin code")
    add_library (${target_name}_shared INTERFACE)

    target_compile_features (${target_name}_shared INTERFACE cxx_std_${target_cxx_standard})

    target_compile_definitions (${target_name}_shared INTERFACE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        YUP_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        YUP_MODAL_LOOPS_PERMITTED=1
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
        _yup_fetchcontent_declare (clap
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

        target_compile_features (${target_name}_clap_plugin PRIVATE cxx_std_20)

        target_compile_definitions (${target_name}_clap_plugin PRIVATE
            YUP_AUDIO_PLUGIN_ENABLE_CLAP=1
            YUP_STANDALONE_APPLICATION=0)

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

        #yup_audio_plugin_copy_bundle (${target_name} clap)
    endif()

    # ==== Fetch vst3 SDK and build vst3 target
    if (YUP_ARG_PLUGIN_CREATE_VST3)
        _yup_message (STATUS "Fetching VST3 SDK")
        set (SMTG_CREATE_MODULE_INFO OFF)
        set (SMTG_ADD_VST3_UTILITIES OFF)
        set (SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
        set (SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
        set (SMTG_ENABLE_VSTGUI_SUPPORT OFF)
        set (SMTG_CREATE_PLUGIN_LINK OFF)
        if (NOT YUP_PLATFORM_MAC OR XCODE)
            set (SMTG_RUN_VST_VALIDATOR ON)
        else()
            set (SMTG_RUN_VST_VALIDATOR OFF)
        endif()
        _yup_fetchcontent_declare (vst3sdk
            GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
            GIT_TAG master)
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

        smtg_add_vst3plugin(${target_name}_vst3_plugin)
        #smtg_target_configure_version_file (${target_name}_vst3_plugin)

        target_compile_features (${target_name}_vst3_plugin PRIVATE cxx_std_${target_cxx_standard})

        target_compile_definitions (${target_name}_vst3_plugin PRIVATE
            YUP_AUDIO_PLUGIN_ENABLE_VST3=1
            YUP_STANDALONE_APPLICATION=0)

        target_link_libraries (${target_name}_vst3_plugin PRIVATE
            ${target_name}_shared
            yup_audio_plugin_client
            sdk
            ${target_name}_vst3
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        if (YUP_PLATFORM_MAC)
            smtg_target_set_bundle (${target_name}_vst3_plugin
                BUNDLE_IDENTIFIER org.kunitoki.yup.${target_name}
                COMPANY_NAME "kunitoki")

            #smtg_target_set_debug_executable(MyPlugin
            #    "/Applications/VST3PluginTestHost.app"
            #    "--pluginfolder;$(BUILT_PRODUCTS_DIR)")

            if (NOT XCODE)
                add_custom_command(
                    TARGET ${target_name}_vst3_plugin POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E echo [SMTG] Validator started...
                    COMMAND
                        $<TARGET_FILE:validator>
                        "${CMAKE_BINARY_DIR}/VST3/${CMAKE_BUILD_TYPE}/${CMAKE_BUILD_TYPE}/${target_name}_vst3_plugin.vst3"
                        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                    COMMAND ${CMAKE_COMMAND} -E echo [SMTG] Validator finished.)
            endif()
        endif()

        set_target_properties (${target_name}_vst3_plugin PROPERTIES
            SUFFIX ".vst3"
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)

        # Add pluginval validation if enabled
        if (YUP_ENABLE_PLUGINVAL)
            yup_validate_plugin (${target_name}_vst3_plugin "$<TARGET_FILE:${target_name}_vst3_plugin>")
        endif()

        yup_audio_plugin_copy_bundle (${target_name} vst3)
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
        yup_standalone_app (
            TARGET_NAME ${target_name}_standalone_plugin
            TARGET_VERSION ${target_version}
            TARGET_IDE_GROUP ${target_ide_group}
            TARGET_APP_ID ${target_app_id}
            TARGET_APP_NAMESPACE ${target_app_namespace}
            TARGET_CXX_STANDARD ${target_cxx_standard}
            DEFINITIONS
                YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1
            MODULES
                ${target_name}_shared
                ${target_name}_standalone
                yup_audio_plugin_client
                yup_audio_devices
                ${additional_libraries}
                ${YUP_ARG_MODULES})
    endif()

endfunction()

#==============================================================================

function (yup_audio_plugin_copy_bundle target_name plugin_type)
    if (NOT YUP_PLATFORM_MAC)
        return()
    endif()

    string (TOUPPER "${plugin_type}" plugin_type_upper)
    set (dependency_target ${target_name}_${plugin_type}_plugin)
    set (target_file_name "${target_name}_${plugin_type}_plugin.${plugin_type}")
    set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/${plugin_type_upper}")
    set (plugin_path "${plugin_target_path}/${target_file_name}")

    if (NOT EXISTS ${plugin_target_path})
        _yup_message (STATUS "Plugin path ${plugin_target_path} does not exist, skipping copy")
        return()
    endif()

    _yup_message (STATUS "Generating rule to copy ${plugin_type} plugin ${target_name}")

    if ("${plugin_type}" STREQUAL "clap")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f ${plugin_path}
            COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE:${dependency_target}>" ${plugin_path}
            COMMENT "Copying ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "vst3")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f ${plugin_path}
            COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_FILE_DIR:${dependency_target}>/../../../${target_file_name}" ${plugin_path}
            COMMENT "Copying ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    else()
        _yup_message (FATAL_ERROR "Unsupported plugin type ${plugin_type} for copying bundle")
    endif()
endfunction()
