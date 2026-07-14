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

function (_yup_fetch_apple_ausdk)
    if (NOT TARGET base-sdk-auv2)
        if (NOT AUDIOUNIT_SDK_ROOT)
            _yup_message (STATUS "Fetching Apple AudioUnitSDK")
            _yup_fetchcontent_declare (AudioUnitSDK
                GIT_REPOSITORY https://github.com/apple/AudioUnitSDK.git
                GIT_TAG        AudioUnitSDK-1.1.0)
            FetchContent_MakeAvailable (AudioUnitSDK)
            set (AUDIOUNIT_SDK_ROOT "${audiounitsdk_SOURCE_DIR}")
        endif()

        set (AUSDK_SRC "${AUDIOUNIT_SDK_ROOT}/src/AudioUnitSDK")

        add_library (base-sdk-auv2 STATIC
            "${AUSDK_SRC}/AUBase.cpp"
            "${AUSDK_SRC}/AUBuffer.cpp"
            "${AUSDK_SRC}/AUBufferAllocator.cpp"
            "${AUSDK_SRC}/AUEffectBase.cpp"
            "${AUSDK_SRC}/AUInputElement.cpp"
            "${AUSDK_SRC}/AUMIDIBase.cpp"
            "${AUSDK_SRC}/AUMIDIEffectBase.cpp"
            "${AUSDK_SRC}/AUOutputElement.cpp"
            "${AUSDK_SRC}/AUPlugInDispatch.cpp"
            "${AUSDK_SRC}/AUScopeElement.cpp"
            "${AUSDK_SRC}/ComponentBase.cpp"
            "${AUSDK_SRC}/MusicDeviceBase.cpp")

        target_include_directories (base-sdk-auv2 PUBLIC "${AUDIOUNIT_SDK_ROOT}/include")
        target_compile_features (base-sdk-auv2 PUBLIC cxx_std_17)
        target_compile_options (base-sdk-auv2 PRIVATE -Wno-deprecated-declarations)

        set_target_properties (base-sdk-auv2 PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            OBJC_VISIBILITY_PRESET hidden
            OBJCXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
            FOLDER "Thirdparty")
    endif()
endfunction()

#==============================================================================

function (_yup_audio_plugin_create_au)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # ==== Set defaults for optional args
    _yup_set_default (YUP_ARG_PLUGIN_AU_SANDBOX_SAFE ON)
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

    _yup_fetch_apple_ausdk()

    _yup_message (STATUS "Setting up AUv2 plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        au
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

    _yup_message (STATUS "Creating AUv2 plugin target")
    add_library (${target_name}_au_plugin MODULE)

    target_compile_features (${target_name}_au_plugin PRIVATE cxx_std_${target_cxx_standard})

    target_compile_definitions (${target_name}_au_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_AU=1
        YUP_STANDALONE_APPLICATION=0)

    _yup_sdl_configure_symbols_patch ("${target_name}_au_plugin" au_sdl_symbols_patch_target au_sdl_symbols_sdl_target)
    set (au_plugin_bundle_libraries
        ${au_sdl_symbols_sdl_target}
        ${au_sdl_symbols_patch_target})

    target_link_libraries (${target_name}_au_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        base-sdk-auv2
        ${target_name}_au
        ${additional_libraries}
        ${au_plugin_bundle_libraries}
        ${target_modules}
        "-framework AudioUnit"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreFoundation"
        "-framework AppKit")

    _yup_module_apply_arc_to_target_sources (${target_name}_au_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        base-sdk-auv2
        ${target_name}_au
        ${additional_libraries}
        ${au_plugin_bundle_libraries}
        ${target_modules})

    # Generate the AU Info.plist from our template
    set (au_plist_template "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../platforms/mac/AudioUnitInfo.plist.in")
    set (au_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_plugin.plist")

    set (PLUGIN_AU_TYPE "${au_bundle_type}")
    set (PLUGIN_AU_SUBTYPE "${YUP_ARG_PLUGIN_AU_SUBTYPE}")
    set (PLUGIN_AU_MANUFACTURER "${YUP_ARG_PLUGIN_AU_MANUFACTURER}")
    set (PLUGIN_AU_NAME "${YUP_ARG_PLUGIN_NAME}")
    set (PLUGIN_AU_VERSION "${YUP_ARG_PLUGIN_VERSION}")

    set (au_bundle_identifier "${target_bundle_id}.au")
    string (REGEX REPLACE "[^A-Za-z0-9.-]" "-" au_bundle_identifier "${au_bundle_identifier}")

    set (au_pkginfo_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_plugin.PkgInfo")
    file (WRITE "${au_pkginfo_file}" "BNDL${YUP_ARG_PLUGIN_AU_MANUFACTURER}")

    configure_file ("${au_plist_template}" "${au_plist_output}" @ONLY)

    set_target_properties (${target_name}_au_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        BUNDLE TRUE
        BUNDLE_EXTENSION "component"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${au_plist_output}"
        MACOSX_BUNDLE_BUNDLE_NAME "${YUP_ARG_PLUGIN_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${YUP_ARG_PLUGIN_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${YUP_ARG_PLUGIN_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${au_bundle_identifier}"
        FOLDER "${target_ide_group}"
        XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE BNDL
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${au_bundle_identifier}"
        XCODE_GENERATE_SCHEME ON)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_au_plugin)

    add_custom_command (TARGET ${target_name}_au_plugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${au_pkginfo_file}" "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}_au_plugin>/PkgInfo"
        COMMENT "Generating AU PkgInfo"
        VERBATIM)

    yup_codesign_target (${target_name}_au_plugin "$<TARGET_BUNDLE_DIR:${target_name}_au_plugin>"
        "${YUP_ARG_PLUGIN_CODESIGN_IDENTITY}"
        "${YUP_ARG_PLUGIN_HARDENED_RUNTIME}")

    yup_validate_au_plugin (
        ${target_name}_au_plugin
        "${YUP_ARG_PLUGIN_NAME}"
        "${au_bundle_type}"
        "${YUP_ARG_PLUGIN_AU_SUBTYPE}"
        "${YUP_ARG_PLUGIN_AU_MANUFACTURER}")

    yup_validate_pluginval (
        ${target_name}_au_plugin
        "$<TARGET_BUNDLE_DIR:${target_name}_au_plugin>")

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} au)
    endif()
endfunction()
