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

#==============================================================================

function (yup_plugin_auv3)
    # ==== Parse arguments
    set (one_value_args
        TARGET_NAME TARGET_CXX_STANDARD TARGET_IDE_GROUP TARGET_BUNDLE_ID
        PLUGIN_IS_SYNTH PLUGIN_NAME PLUGIN_VERSION PLUGIN_AU_SUBTYPE PLUGIN_AU_MANUFACTURER
        STANDALONE_TARGET HAS_STANDALONE)

    set (multi_value_args
        SHARED_LIBS
        ADDITIONAL_LIBRARIES
        MODULES)

    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT YUP_PLATFORM_MAC)
        _yup_message (WARNING "AUv3 plugins are only supported on macOS. Skipping AUv3 target.")
        return()
    endif()

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_bundle_id "${YUP_ARG_TARGET_BUNDLE_ID}")

    _yup_message (STATUS "Setting up AUv3 plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        auv3
        ${YUP_ARG_UNPARSED_ARGUMENTS})

    # Determine AU type (aumu for instruments, aufx for effects)
    if (YUP_ARG_PLUGIN_IS_SYNTH)
        set (au_bundle_type "aumu")
    else()
        set (au_bundle_type "aufx")
    endif()

    if (NOT YUP_ARG_PLUGIN_AU_SUBTYPE)
        set (YUP_ARG_PLUGIN_AU_SUBTYPE "Dflt")
    endif()
    if (NOT YUP_ARG_PLUGIN_AU_MANUFACTURER)
        set (YUP_ARG_PLUGIN_AU_MANUFACTURER "Yup!")
    endif()
    if (NOT YUP_ARG_PLUGIN_NAME)
        set (YUP_ARG_PLUGIN_NAME "${target_name}")
    endif()
    if (NOT YUP_ARG_PLUGIN_VERSION)
        set (YUP_ARG_PLUGIN_VERSION "1")
    endif()

    _yup_message (STATUS "Creating AUv3 plugin target")

    # Create the .appex App Extension bundle target
    add_library (${target_name}_auv3_plugin MODULE)

    target_compile_features (${target_name}_auv3_plugin PRIVATE cxx_std_${target_cxx_standard})

    target_compile_definitions (${target_name}_auv3_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_AUv3=1
        YUP_STANDALONE_APPLICATION=0)

    _yup_sdl_configure_symbols_patch ("${target_name}_auv3_plugin" auv3_sdl_symbols_patch_target auv3_sdl_symbols_sdl_target)
    set (auv3_plugin_bundle_libraries
        ${auv3_sdl_symbols_sdl_target}
        ${auv3_sdl_symbols_patch_target})

    target_link_libraries (${target_name}_auv3_plugin PRIVATE
        ${YUP_ARG_SHARED_LIBS}
        yup_audio_plugin_client
        ${target_name}_auv3
        ${YUP_ARG_ADDITIONAL_LIBRARIES}
        ${auv3_plugin_bundle_libraries}
        ${YUP_ARG_MODULES}
        "-framework AVFoundation"
        "-framework AudioToolbox"
        "-framework CoreAudioKit"
        "-framework CoreAudio"
        "-framework CoreFoundation"
        "-framework AppKit")

    _yup_module_apply_arc_to_target_sources (${target_name}_auv3_plugin
        ${YUP_ARG_SHARED_LIBS}
        yup_audio_plugin_client
        ${target_name}_auv3
        ${YUP_ARG_ADDITIONAL_LIBRARIES}
        ${auv3_plugin_bundle_libraries}
        ${YUP_ARG_MODULES})

    # Generate the AUv3 Info.plist from our template
    set (auv3_plist_template "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platforms/mac/AudioUnitV3Info.plist.in")
    set (auv3_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_auv3_plugin.plist")

    set (PLUGIN_AU_TYPE "${au_bundle_type}")
    set (PLUGIN_AU_SUBTYPE "${YUP_ARG_PLUGIN_AU_SUBTYPE}")
    set (PLUGIN_AU_MANUFACTURER "${YUP_ARG_PLUGIN_AU_MANUFACTURER}")
    set (PLUGIN_AU_NAME "${YUP_ARG_PLUGIN_NAME}")
    set (PLUGIN_AU_VERSION "${YUP_ARG_PLUGIN_VERSION}")

    set (auv3_bundle_identifier "${target_bundle_id}.auv3")
    string (REGEX REPLACE "[^A-Za-z0-9.-]" "-" auv3_bundle_identifier "${auv3_bundle_identifier}")

    configure_file ("${auv3_plist_template}" "${auv3_plist_output}" @ONLY)

    set_target_properties (${target_name}_auv3_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        BUNDLE TRUE
        BUNDLE_EXTENSION "appex"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${auv3_plist_output}"
        MACOSX_BUNDLE_BUNDLE_NAME "${YUP_ARG_PLUGIN_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${YUP_ARG_PLUGIN_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${YUP_ARG_PLUGIN_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${auv3_bundle_identifier}"
        FOLDER "${target_ide_group}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE XPC!
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${auv3_bundle_identifier}"
        XCODE_GENERATE_SCHEME ON)

    # If standalone is also enabled, embed the .appex in the standalone app
    if (YUP_ARG_HAS_STANDALONE)
        add_dependencies (${YUP_ARG_STANDALONE_TARGET} ${target_name}_auv3_plugin)

        if (XCODE)
            set_target_properties (${target_name}_auv3_plugin PROPERTIES
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE XPC!
                XCODE_EMBED_APP_EXTENSIONS "${target_name}_auv3_plugin.appex")
        endif()
    endif()

    yup_codesign_target (${target_name}_auv3_plugin "$<TARGET_BUNDLE_DIR:${target_name}_auv3_plugin>")

    yup_audio_plugin_copy_bundle (${target_name} auv3)

    # Validation
    set (auv3_pluginval_path "$ENV{HOME}/Library/Audio/Plug-Ins/AppExtensions/${target_name}_auv3_plugin.appex")

    yup_validate_au_plugin (
        ${target_name}_auv3_plugin
        "${YUP_ARG_PLUGIN_NAME}"
        "${au_bundle_type}"
        "${YUP_ARG_PLUGIN_AU_SUBTYPE}"
        "${YUP_ARG_PLUGIN_AU_MANUFACTURER}")

    yup_validate_pluginval (
        ${target_name}_auv3_plugin
        "${auv3_pluginval_path}")

endfunction()
