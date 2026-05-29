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
        PLUGIN_CREATE_CLAP PLUGIN_CREATE_VST3 PLUGIN_CREATE_STANDALONE PLUGIN_CREATE_AU)

    set (multi_value_args
        DEFINITIONS
        MODULES
        LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_set_default (YUP_ARG_TARGET_CXX_STANDARD 20)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_app_id "${YUP_ARG_TARGET_APP_ID}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (target_bundle_id "${target_app_id}")
    if (NOT target_bundle_id)
        set (target_bundle_id "org.kunitoki.yup.${target_name}")
    endif()
    string (REGEX REPLACE "[^A-Za-z0-9.-]" "-" target_bundle_id "${target_bundle_id}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    # ==== Validation stage
    if (NOT YUP_PLATFORM_DESKTOP)
        _yup_message (FATAL_ERROR "Audio plugins are not supported on emscripten or android.")
        return()
    endif()

    if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT YUP_ARG_PLUGIN_CREATE_VST3 AND NOT YUP_ARG_PLUGIN_CREATE_STANDALONE AND NOT YUP_ARG_PLUGIN_CREATE_AU)
        _yup_message (FATAL_ERROR "At least one plugin type must be enabled (CLAP, VST3, AU, or Standalone).")
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
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ON
        XCODE_GENERATE_SCHEME ON)

    # ==== Find dependencies
    include (FetchContent)
    _yup_fetch_sdl2()
    list (APPEND additional_libraries sdl2::sdl2)

    _yup_target_list_contains ("${YUP_ARG_MODULES}" yup_audio_plugin_host has_audio_plugin_host)
    if (has_audio_plugin_host)
        _yup_collect_audio_plugin_host_dependencies ("${YUP_ARG_DEFINITIONS}" audio_plugin_host_libraries)
        list (APPEND additional_libraries ${audio_plugin_host_libraries})
        if (audio_plugin_host_libraries)
            target_link_libraries (${target_name}_shared INTERFACE
                ${audio_plugin_host_libraries})
        endif()
    endif()

    # ==== Fetch clap SDK and build clap target
    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        _yup_fetch_clap()

        _yup_message (STATUS "Setting up CLAP plugin client")
        _yup_module_setup_plugin_client (
            ${target_name}
            yup_audio_plugin_client
            ${YUP_ARG_TARGET_IDE_GROUP}
            clap
            ${YUP_ARG_UNPARSED_ARGUMENTS})

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

        target_link_libraries (${target_name}_clap_plugin PRIVATE
            ${target_name}_shared
            yup_audio_plugin_client
            clap
            ${target_name}_clap
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        _yup_module_apply_arc_to_target_sources (${target_name}_clap_plugin
            ${target_name}_shared
            yup_audio_plugin_client
            clap
            ${target_name}_clap
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        set_target_properties (${target_name}_clap_plugin PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            OBJC_VISIBILITY_PRESET hidden
            OBJCXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)

        if (YUP_PLATFORM_MAC)
            set_target_properties (${target_name}_clap_plugin PROPERTIES
                BUNDLE TRUE
                BUNDLE_EXTENSION "clap"
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_BUNDLE_NAME "${target_name}_clap_plugin"
                MACOSX_BUNDLE_GUI_IDENTIFIER "${target_bundle_id}.clap"
                PREFIX "")

            set (clap_plugin_path "$<TARGET_BUNDLE_DIR:${target_name}_clap_plugin>")
        else()
            set_target_properties (${target_name}_clap_plugin PROPERTIES
                SUFFIX ".clap")

            set (clap_plugin_path "$<TARGET_FILE:${target_name}_clap_plugin>")
        endif()

        yup_codesign_target (${target_name}_clap_plugin "${clap_plugin_path}")

        yup_validate_clap_plugin (${target_name}_clap_plugin "${clap_plugin_path}")

        yup_audio_plugin_copy_bundle (${target_name} clap)
    endif()

    # ==== Fetch vst3 SDK and build vst3 target
    if (YUP_ARG_PLUGIN_CREATE_VST3)
        _yup_fetch_vst3sdk()

        _yup_message (STATUS "Setting up VST3 plugin client")
        get_directory_property (_yup_vst3_saved_compile_options COMPILE_OPTIONS)
        smtg_enable_vst3_sdk()
        set_directory_properties (PROPERTIES COMPILE_OPTIONS "${_yup_vst3_saved_compile_options}")

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

        _yup_module_apply_arc_to_target_sources (${target_name}_vst3_plugin
            ${target_name}_shared
            yup_audio_plugin_client
            sdk
            ${target_name}_vst3
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        set_target_properties (${target_name}_vst3_plugin PROPERTIES
            C_VISIBILITY_PRESET hidden
            CXX_VISIBILITY_PRESET hidden
            OBJC_VISIBILITY_PRESET hidden
            OBJCXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
            SUFFIX ".vst3"
            FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
            XCODE_GENERATE_SCHEME ON)

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
                COMPANY_NAME "kunitoki") # TODO - make company name configurable
            if ("${vst3_plugin_package_path}" STREQUAL "${vst3_plugin_binary_path}")
                set (vst3_plugin_package_path "$<TARGET_BUNDLE_DIR:${target_name}_vst3_plugin>")
            endif()
            set (vst3_pluginval_path "${vst3_plugin_package_path}")
        endif()
        yup_validate_smtg_vst3_plugin (${target_name}_vst3_plugin "${vst3_plugin_package_path}")

        yup_validate_pluginval (${target_name}_vst3_plugin "${vst3_pluginval_path}")

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
            TARGET_APP_ID ${target_bundle_id}
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

    # ==== Build AUv2 plugin target (macOS only)
    if (YUP_ARG_PLUGIN_CREATE_AU)
        if (NOT YUP_PLATFORM_MAC)
            _yup_message (WARNING "AUv2 plugins are only supported on macOS. Skipping AU target.")
        else()
            _yup_fetch_apple_ausdk()

            _yup_message (STATUS "Setting up AUv2 plugin client")
            _yup_module_setup_plugin_client (
                ${target_name}
                yup_audio_plugin_client
                ${YUP_ARG_TARGET_IDE_GROUP}
                au
                ${YUP_ARG_UNPARSED_ARGUMENTS})

            # Determine AU type (aumu for instruments, aufx for effects)
            cmake_parse_arguments (AU_ARGS ""
                "PLUGIN_IS_SYNTH;PLUGIN_AU_SUBTYPE;PLUGIN_AU_MANUFACTURER;PLUGIN_NAME;PLUGIN_VERSION;PLUGIN_ID;PLUGIN_VENDOR;PLUGIN_DESCRIPTION;PLUGIN_URL;PLUGIN_EMAIL;PLUGIN_IS_MONO"
                "" ${YUP_ARG_UNPARSED_ARGUMENTS})
            if (AU_ARGS_PLUGIN_IS_SYNTH)
                set (au_bundle_type "aumu")
            else()
                set (au_bundle_type "aufx")
            endif()

            if (NOT AU_ARGS_PLUGIN_AU_SUBTYPE)
                set (AU_ARGS_PLUGIN_AU_SUBTYPE "Dflt")
            endif()
            if (NOT AU_ARGS_PLUGIN_AU_MANUFACTURER)
                set (AU_ARGS_PLUGIN_AU_MANUFACTURER "Yup!")
            endif()
            if (NOT AU_ARGS_PLUGIN_NAME)
                set (AU_ARGS_PLUGIN_NAME "${target_name}")
            endif()
            if (NOT AU_ARGS_PLUGIN_VERSION)
                set (AU_ARGS_PLUGIN_VERSION "1")
            endif()

            _yup_message (STATUS "Creating AUv2 plugin target")
            add_library (${target_name}_au_plugin MODULE)

            target_compile_features (${target_name}_au_plugin PRIVATE cxx_std_${target_cxx_standard})

            target_compile_definitions (${target_name}_au_plugin PRIVATE
                YUP_AUDIO_PLUGIN_ENABLE_AU=1
                YUP_STANDALONE_APPLICATION=0)

            target_link_libraries (${target_name}_au_plugin PRIVATE
                ${target_name}_shared
                yup_audio_plugin_client
                base-sdk-auv2
                ${target_name}_au
                ${additional_libraries}
                ${YUP_ARG_MODULES}
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
                ${YUP_ARG_MODULES})

            # Generate the AU Info.plist from our template
            set (au_plist_template "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/mac/AUInfo.plist")
            set (au_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_plugin.plist")

            set (PLUGIN_AU_TYPE "${au_bundle_type}")
            set (PLUGIN_AU_SUBTYPE "${AU_ARGS_PLUGIN_AU_SUBTYPE}")
            set (PLUGIN_AU_MANUFACTURER "${AU_ARGS_PLUGIN_AU_MANUFACTURER}")
            set (PLUGIN_AU_NAME "${AU_ARGS_PLUGIN_NAME}")
            set (PLUGIN_AU_VERSION "${AU_ARGS_PLUGIN_VERSION}")

            set (au_bundle_identifier "${target_bundle_id}.au")
            string (REGEX REPLACE "[^A-Za-z0-9.-]" "-" au_bundle_identifier "${au_bundle_identifier}")

            set (au_pkginfo_file "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_au_plugin.PkgInfo")
            file (WRITE "${au_pkginfo_file}" "BNDL${AU_ARGS_PLUGIN_AU_MANUFACTURER}")

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
                MACOSX_BUNDLE_BUNDLE_NAME "${AU_ARGS_PLUGIN_NAME}"
                MACOSX_BUNDLE_BUNDLE_VERSION "${AU_ARGS_PLUGIN_VERSION}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING "${AU_ARGS_PLUGIN_VERSION}"
                MACOSX_BUNDLE_GUI_IDENTIFIER "${au_bundle_identifier}"
                FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
                XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE BNDL
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${au_bundle_identifier}"
                XCODE_GENERATE_SCHEME ON)

            add_custom_command (TARGET ${target_name}_au_plugin POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${au_pkginfo_file}" "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}_au_plugin>/PkgInfo"
                COMMENT "Generating AU PkgInfo"
                VERBATIM)

            yup_codesign_target (${target_name}_au_plugin "$<TARGET_BUNDLE_DIR:${target_name}_au_plugin>")

            yup_audio_plugin_copy_bundle (${target_name} au)

            set (au_pluginval_path "$ENV{HOME}/Library/Audio/Plug-Ins/Components/${target_name}_au_plugin.component")

            yup_validate_au_plugin (
                ${target_name}_au_plugin
                "${AU_ARGS_PLUGIN_NAME}"
                "${au_bundle_type}"
                "${AU_ARGS_PLUGIN_AU_SUBTYPE}"
                "${AU_ARGS_PLUGIN_AU_MANUFACTURER}")

            yup_validate_pluginval (
                ${target_name}_au_plugin
                "${au_pluginval_path}")
        endif()
    endif()

    # ==== Create composite target for all enabled plugin formats
    set (_all_plugin_targets "")
    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        list (APPEND _all_plugin_targets ${target_name}_clap_plugin)
    endif()
    if (YUP_ARG_PLUGIN_CREATE_VST3)
        list (APPEND _all_plugin_targets ${target_name}_vst3_plugin)
    endif()
    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        list (APPEND _all_plugin_targets ${target_name}_standalone_plugin)
    endif()
    if (YUP_ARG_PLUGIN_CREATE_AU AND YUP_PLATFORM_MAC)
        list (APPEND _all_plugin_targets ${target_name}_au_plugin)
    endif()

    add_custom_target (${target_name} DEPENDS ${_all_plugin_targets})
    set_target_properties (${target_name} PROPERTIES
        FOLDER "${YUP_ARG_TARGET_IDE_GROUP}"
        XCODE_GENERATE_SCHEME ON)

endfunction()

#==============================================================================

function (yup_audio_plugin_copy_bundle target_name plugin_type)
    if (NOT YUP_PLATFORM_MAC)
        return()
    endif()

    string (TOUPPER "${plugin_type}" plugin_type_upper)
    set (dependency_target ${target_name}_${plugin_type}_plugin)

    if ("${plugin_type}" STREQUAL "au")
        set (target_file_name "${target_name}_${plugin_type}_plugin.component")
        set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
    else()
        set (target_file_name "${target_name}_${plugin_type}_plugin.${plugin_type}")
        set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/${plugin_type_upper}")
    endif()

    set (plugin_path "${plugin_target_path}/${target_file_name}")

    if (NOT EXISTS ${plugin_target_path} AND NOT "${plugin_type}" STREQUAL "clap")
        _yup_message (STATUS "Plugin path ${plugin_target_path} does not exist, skipping copy")
        return()
    endif()

    _yup_message (STATUS "Generating rule to copy ${plugin_type} plugin ${target_name}")

    if ("${plugin_type}" STREQUAL "clap")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMENT "Symlinking CLAP plugin ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "vst3")
        get_target_property (source_plugin_path ${dependency_target} SMTG_PLUGIN_PACKAGE_PATH)
        if (NOT source_plugin_path)
            set (source_plugin_path "$<TARGET_BUNDLE_DIR:${dependency_target}>")
        endif()

        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${source_plugin_path}" "${plugin_path}"
            COMMENT "Symlinking VST3 plugin ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "au")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMENT "Copying AU plugin ${plugin_type_upper} to ${plugin_path}"
            VERBATIM)
    else()
        _yup_message (FATAL_ERROR "Unsupported plugin type ${plugin_type} for copying bundle")
    endif()
endfunction()
