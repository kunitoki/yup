# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
#
#   YUP is an open source library subject to open-source licensing.
#
#   The code included in this file is provided under the terms of the ISC license
#   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#   to use, copy, modify, and/or distribute this software for any purpose with or
#   without fee is hereby granted provided that the above copyright notice and
#   this permission notice appear in all copies.
#
#   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#   DISCLAIMED.
#
# ==============================================================================

include_guard (GLOBAL)

#==============================================================================

function (_yup_fetch_vst3sdk)
    if (NOT TARGET sdk)
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
    endif()

    if (NOT TARGET yup_audio_plugin_host_vst3sdk)
        add_library (yup_audio_plugin_host_vst3sdk INTERFACE)
        target_link_libraries (yup_audio_plugin_host_vst3sdk INTERFACE sdk)

        set (vst3sdk_source_dir "")
        if (DEFINED vst3sdk_SOURCE_DIR)
            set (vst3sdk_source_dir "${vst3sdk_SOURCE_DIR}")
        elseif (TARGET sdk)
            get_target_property (vst3sdk_source_dir sdk SOURCE_DIR)
        endif()

        set (vst3sdk_memorystream_source "${vst3sdk_source_dir}/public.sdk/source/common/memorystream.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_memorystream_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_memorystream_source}")
        endif()

        set (vst3sdk_parameterchanges_source "${vst3sdk_source_dir}/public.sdk/source/vst/hosting/parameterchanges.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_parameterchanges_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_parameterchanges_source}")
        endif()

        set (vst3sdk_eventlist_source "${vst3sdk_source_dir}/public.sdk/source/vst/hosting/eventlist.cpp")
        if (vst3sdk_source_dir AND EXISTS "${vst3sdk_eventlist_source}")
            target_sources (yup_audio_plugin_host_vst3sdk INTERFACE "${vst3sdk_eventlist_source}")
        endif()
    endif()
endfunction()

#==============================================================================

function (_yup_audio_plugin_create_vst3)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # ==== Set defaults for optional args
    _yup_set_default (YUP_ARG_PLUGIN_VST3_AUTO_MANIFEST ON)
    _yup_set_default (YUP_ARG_PLUGIN_COPY_AFTER_BUILD ON)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_bundle_id "${YUP_ARG_TARGET_BUNDLE_ID}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (additional_libraries "${YUP_ARG_ADDITIONAL_LIBRARIES}")
    set (target_modules "${YUP_ARG_MODULES}")

    _yup_fetch_vst3sdk()

    _yup_message (STATUS "Setting up VST3 plugin client")
    get_directory_property (_yup_vst3_saved_compile_options COMPILE_OPTIONS)
    smtg_enable_vst3_sdk()
    set_directory_properties (PROPERTIES COMPILE_OPTIONS "${_yup_vst3_saved_compile_options}")

    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        vst3
        PLUGIN_ID ${YUP_ARG_PLUGIN_ID}
        PLUGIN_NAME ${YUP_ARG_PLUGIN_NAME}
        PLUGIN_VENDOR ${YUP_ARG_PLUGIN_VENDOR}
        PLUGIN_EMAIL ${YUP_ARG_PLUGIN_EMAIL}
        PLUGIN_VERSION ${YUP_ARG_PLUGIN_VERSION}
        PLUGIN_DESCRIPTION ${YUP_ARG_PLUGIN_DESCRIPTION}
        PLUGIN_URL ${YUP_ARG_PLUGIN_URL}
        PLUGIN_IS_SYNTH ${YUP_ARG_PLUGIN_IS_SYNTH}
        PLUGIN_IS_MONO ${YUP_ARG_PLUGIN_IS_MONO}
        PLUGIN_VST3_CATEGORIES ${YUP_ARG_PLUGIN_VST3_CATEGORIES})

    # Create VST3 plugin target
    _yup_message (STATUS "Creating VST3 plugin target")

    smtg_add_vst3plugin(${target_name}_vst3_plugin)

    target_compile_features (${target_name}_vst3_plugin PRIVATE cxx_std_${target_cxx_standard})

    target_compile_definitions (${target_name}_vst3_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_VST3=1
        YUP_STANDALONE_APPLICATION=0)

    _yup_sdl_configure_symbols_patch ("${target_name}_vst3_plugin" vst3_sdl_symbols_patch_target vst3_sdl_symbols_sdl_target)
    set (vst3_plugin_bundle_libraries
        ${vst3_sdl_symbols_sdl_target}
        ${vst3_sdl_symbols_patch_target})

    target_link_libraries (${target_name}_vst3_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        sdk
        ${target_name}_vst3
        ${additional_libraries}
        ${vst3_plugin_bundle_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_vst3_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        sdk
        ${target_name}_vst3
        ${additional_libraries}
        ${vst3_plugin_bundle_libraries}
        ${target_modules})

    set_target_properties (${target_name}_vst3_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        SUFFIX ".vst3"
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_vst3_plugin)

    set (vst3_plugin_binary_path "$<TARGET_FILE:${target_name}_vst3_plugin>")
    set (vst3_pluginval_path "${vst3_plugin_binary_path}")
    get_target_property (vst3_plugin_package_path ${target_name}_vst3_plugin SMTG_PLUGIN_PACKAGE_PATH)
    if (vst3_plugin_package_path)
        set (vst3_pluginval_path "${vst3_plugin_package_path}")
    else()
        set (vst3_plugin_package_path "${vst3_plugin_binary_path}")
    endif()
    if (YUP_PLATFORM_MAC)
        smtg_target_set_bundle (${target_name}_vst3_plugin
            BUNDLE_IDENTIFIER "${target_bundle_id}"
            COMPANY_NAME "kunitoki")

        set (vst3_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_vst3_plugin.plist")
        _yup_configure_audio_plugin_bundle_info_plist ("${vst3_plist_output}" "BNDL")

        set_target_properties (${target_name}_vst3_plugin PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${vst3_plist_output}"
            MACOSX_BUNDLE_BUNDLE_NAME "${target_name}_vst3_plugin"
            MACOSX_BUNDLE_BUNDLE_VERSION "${target_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${target_version}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${target_bundle_id}"
            XCODE_ATTRIBUTE_INFOPLIST_FILE "${vst3_plist_output}"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE BNDL)

        if (XCODE)
            get_target_property (vst3_plugin_package_path ${target_name}_vst3_plugin SMTG_PLUGIN_PACKAGE_PATH)
        else()
            set (vst3_plugin_package_path "$<TARGET_BUNDLE_DIR:${target_name}_vst3_plugin>")
        endif()

        set (vst3_pluginval_path "${vst3_plugin_package_path}")
    endif()
    yup_validate_smtg_vst3_plugin (${target_name}_vst3_plugin "${vst3_plugin_package_path}")

    yup_validate_pluginval (${target_name}_vst3_plugin "${vst3_pluginval_path}")

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} vst3)
    endif()
endfunction()
