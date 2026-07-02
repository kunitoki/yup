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

function (_yup_audio_plugin_create_clap
    target_name
    target_version
    target_ide_group
    target_bundle_id
    target_app_namespace
    target_cxx_standard
    additional_libraries
    target_modules
    unparsed_args)

    _yup_fetch_clap()

    _yup_message (STATUS "Setting up CLAP plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        clap
        ${unparsed_args})

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

    yup_codesign_target (${target_name}_clap_plugin "${clap_plugin_path}")

    yup_validate_clap_plugin (${target_name}_clap_plugin "${clap_plugin_path}")

    yup_audio_plugin_copy_bundle (${target_name} clap)
endfunction()
