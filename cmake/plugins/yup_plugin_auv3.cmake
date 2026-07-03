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

#==============================================================================

function (yup_plugin_auv3)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Set defaults for optional args
    _yup_set_default (YUP_ARG_PLUGIN_AU_SANDBOX_SAFE ON)
    _yup_set_default (YUP_ARG_PLUGIN_COPY_AFTER_BUILD ON)
    _yup_set_default (YUP_ARG_PLUGIN_CODESIGN_IDENTITY "-")
    _yup_set_default (YUP_ARG_PLUGIN_HARDENED_RUNTIME OFF)

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
        PLUGIN_ID ${YUP_ARG_PLUGIN_ID}
        PLUGIN_NAME ${YUP_ARG_PLUGIN_NAME}
        PLUGIN_VENDOR ${YUP_ARG_PLUGIN_VENDOR}
        PLUGIN_EMAIL ${YUP_ARG_PLUGIN_EMAIL}
        PLUGIN_VERSION ${YUP_ARG_PLUGIN_VERSION}
        PLUGIN_DESCRIPTION ${YUP_ARG_PLUGIN_DESCRIPTION}
        PLUGIN_URL ${YUP_ARG_PLUGIN_URL}
        PLUGIN_IS_SYNTH ${YUP_ARG_PLUGIN_IS_SYNTH}
        PLUGIN_IS_MONO ${YUP_ARG_PLUGIN_IS_MONO}
        PLUGIN_AU_SUBTYPE ${YUP_ARG_PLUGIN_AU_SUBTYPE}
        PLUGIN_AU_MANUFACTURER ${YUP_ARG_PLUGIN_AU_MANUFACTURER}
        PLUGIN_AU_SANDBOX_SAFE ${YUP_ARG_PLUGIN_AU_SANDBOX_SAFE})

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

    # Determine sandboxSafe plist value
    if (YUP_ARG_PLUGIN_AU_SANDBOX_SAFE)
        set (PLUGIN_AU_SANDBOX_SAFE "<true/>")
    else()
        set (PLUGIN_AU_SANDBOX_SAFE "<false/>")
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
    if (YUP_ARG_PLUGIN_IS_SYNTH)
        set (PLUGIN_AU_TAG "Instrument")
    else()
        set (PLUGIN_AU_TAG "Effects")
    endif()

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

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_auv3_plugin)

    # Always create a minimal dedicated container app to host the .appex
    set (container_target "${target_name}_auv3_container")
    set (auv3_container_main "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../../modules/yup_audio_plugin_client/auv3/yup_audio_plugin_client_AUv3_main.mm")
    set (auv3_container_plist_template "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platforms/mac/AudioUnitV3ContainerInfo.plist.in")
    set (auv3_container_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${container_target}.plist")
    set (auv3_container_bundle_identifier "${target_bundle_id}")

    configure_file ("${auv3_container_plist_template}" "${auv3_container_plist_output}" @ONLY)

    add_executable (${container_target} MACOSX_BUNDLE "${auv3_container_main}")
    add_dependencies (${container_target} ${target_name}_auv3_plugin)

    set_target_properties (${container_target} PROPERTIES
        BUNDLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${auv3_container_plist_output}"
        MACOSX_BUNDLE_BUNDLE_NAME "${YUP_ARG_PLUGIN_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${auv3_container_bundle_identifier}"
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

    target_link_libraries (${container_target} PRIVATE "-framework Cocoa")

    if (XCODE)
        set_target_properties (${container_target} PROPERTIES
            XCODE_EMBED_APP_EXTENSIONS ${target_name}_auv3_plugin)
    else()
        add_custom_command (TARGET ${container_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_DIR:${container_target}>/Contents/PlugIns"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_BUNDLE_DIR:${target_name}_auv3_plugin>"
                "$<TARGET_BUNDLE_DIR:${container_target}>/Contents/PlugIns/$<TARGET_FILE_BASE_NAME:${target_name}_auv3_plugin>.appex"
            COMMENT "Embedding .appex in container app"
            VERBATIM)
    endif()

    add_custom_command (TARGET ${container_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$ENV{HOME}/Applications"
        COMMAND ${CMAKE_COMMAND} -E rm -rf
            "$ENV{HOME}/Applications/$<TARGET_FILE_BASE_NAME:${container_target}>.app"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_BUNDLE_DIR:${container_target}>"
            "$ENV{HOME}/Applications/$<TARGET_FILE_BASE_NAME:${container_target}>.app"
        COMMENT "Installing container app to ~/Applications"
        VERBATIM)

    # Determine entitlements file to use
    if (YUP_ARG_PLUGIN_APPLE_ENTITLEMENTS)
        set (auv3_entitlements "${YUP_ARG_PLUGIN_APPLE_ENTITLEMENTS}")
    else()
        set (auv3_entitlements "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platforms/mac/AudioUnitV3_Entitlements.plist")
    endif()

    yup_codesign_target (${target_name}_auv3_plugin "$<TARGET_BUNDLE_DIR:${target_name}_auv3_plugin>"
        "${YUP_ARG_PLUGIN_CODESIGN_IDENTITY}"
        "${YUP_ARG_PLUGIN_HARDENED_RUNTIME}"
        "${auv3_entitlements}")

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} auv3)
    endif()
endfunction()
