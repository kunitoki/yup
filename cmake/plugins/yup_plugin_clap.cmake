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

function (_yup_fetch_clap)
    if (NOT TARGET clap)
        _yup_message (STATUS "Fetching CLAP SDK")
        _yup_fetchcontent_declare (clap
            GIT_REPOSITORY https://github.com/free-audio/clap.git
            GIT_TAG main)

        FetchContent_MakeAvailable (clap)
    endif()

    if (TARGET clap-tests)
        set_target_properties (clap-tests PROPERTIES FOLDER "Tests")
    endif()
endfunction()

#==============================================================================

function (_yup_audio_plugin_create_clap)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # ==== Set defaults for optional args
    _yup_set_default (YUP_ARG_PLUGIN_COPY_AFTER_BUILD ON)
    _yup_set_default (YUP_ARG_PLUGIN_CODESIGN_IDENTITY "-")
    _yup_set_default (YUP_ARG_PLUGIN_HARDENED_RUNTIME OFF)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_bundle_id "${YUP_ARG_TARGET_BUNDLE_ID}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (additional_libraries "${YUP_ARG_ADDITIONAL_LIBRARIES}")
    set (target_modules "${YUP_ARG_MODULES}")

    _yup_fetch_clap()

    _yup_message (STATUS "Setting up CLAP plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        clap
        PLUGIN_ID ${YUP_ARG_PLUGIN_ID}
        PLUGIN_NAME ${YUP_ARG_PLUGIN_NAME}
        PLUGIN_VENDOR ${YUP_ARG_PLUGIN_VENDOR}
        PLUGIN_EMAIL ${YUP_ARG_PLUGIN_EMAIL}
        PLUGIN_VERSION ${YUP_ARG_PLUGIN_VERSION}
        PLUGIN_DESCRIPTION ${YUP_ARG_PLUGIN_DESCRIPTION}
        PLUGIN_URL ${YUP_ARG_PLUGIN_URL}
        PLUGIN_IS_SYNTH ${YUP_ARG_PLUGIN_IS_SYNTH}
        PLUGIN_IS_MONO ${YUP_ARG_PLUGIN_IS_MONO}
        PLUGIN_CLAP_FEATURES ${YUP_ARG_PLUGIN_CLAP_FEATURES})

    # Create CLAP plugin target
    _yup_message (STATUS "Creating CLAP plugin target")
    if (YUP_PLATFORM_MAC)
        add_library (${target_name}_clap_plugin MODULE)
    else()
        add_library (${target_name}_clap_plugin SHARED)
    endif()

    target_compile_features (${target_name}_clap_plugin PRIVATE cxx_std_20)

    target_compile_definitions (${target_name}_clap_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_CLAP=1
        YUP_STANDALONE_APPLICATION=0)

    _yup_sdl_configure_symbols_patch ("${target_name}_clap_plugin" clap_sdl_symbols_patch_target clap_sdl_symbols_sdl_target)
    set (clap_plugin_bundle_libraries
        ${clap_sdl_symbols_sdl_target}
        ${clap_sdl_symbols_patch_target})

    target_link_libraries (${target_name}_clap_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        clap
        ${target_name}_clap
        ${additional_libraries}
        ${clap_plugin_bundle_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_clap_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        clap
        ${target_name}_clap
        ${additional_libraries}
        ${clap_plugin_bundle_libraries}
        ${target_modules})

    set_target_properties (${target_name}_clap_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_clap_plugin
        EXPORTED_SYMBOLS clap_entry)

    if (YUP_PLATFORM_MAC)
        set (clap_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_clap_plugin.plist")
        _yup_configure_audio_plugin_bundle_info_plist ("${clap_plist_output}" "BNDL")

        set_target_properties (${target_name}_clap_plugin PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "clap"
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${clap_plist_output}"
            MACOSX_BUNDLE_BUNDLE_NAME "${target_name}_clap_plugin"
            MACOSX_BUNDLE_BUNDLE_VERSION "${target_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${target_version}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${target_bundle_id}.clap"
            XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE BNDL
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${target_bundle_id}.clap"
            PREFIX "")

        set (clap_plugin_path "$<TARGET_BUNDLE_DIR:${target_name}_clap_plugin>")
    else()
        set_target_properties (${target_name}_clap_plugin PROPERTIES
            PREFIX ""
            SUFFIX ".clap")

        set (clap_plugin_path "$<TARGET_FILE:${target_name}_clap_plugin>")
    endif()

    if (YUP_PLATFORM_MAC)
        yup_codesign_target (${target_name}_clap_plugin "${clap_plugin_path}"
            "${YUP_ARG_PLUGIN_CODESIGN_IDENTITY}"
            "${YUP_ARG_PLUGIN_HARDENED_RUNTIME}")
    endif()

    yup_validate_clap_plugin (${target_name}_clap_plugin "${clap_plugin_path}")

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} clap)
    endif()
endfunction()
