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

function (_yup_configure_audio_plugin_bundle_info_plist output_file package_type)
    set (YUP_AUDIO_PLUGIN_BUNDLE_PACKAGE_TYPE "${package_type}")
    if ("${package_type}" STREQUAL "TDMw")
        configure_file ("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/mac/AudioPluginInfo_AAX.plist.in" "${output_file}" @ONLY)
    else()
        configure_file ("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/platforms/mac/AudioPluginInfo.plist.in" "${output_file}" @ONLY)
    endif()
endfunction()

#==============================================================================

function (yup_audio_plugin)
    # ==== Parse all known keywords so nothing leaks into unparsed.
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_set_default (YUP_ARG_TARGET_CXX_STANDARD 20)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (target_app_id "${YUP_ARG_TARGET_APP_ID}")
    set (target_bundle_id "${target_app_id}")

    set (additional_definitions "${YUP_ARG_DEFINITIONS}")
    set (additional_options "${YUP_ARG_OPTIONS}")
    set (additional_libraries "${YUP_ARG_ADDITIONAL_LIBRARIES}")

    string (REGEX REPLACE "[^A-Za-z0-9.-]" "-" target_bundle_id "${target_bundle_id}")

    # ==== Validation stage
    if (NOT YUP_PLATFORM_DESKTOP)
        _yup_message (FATAL_ERROR "Audio plugins are not supported on emscripten or mobile (yet).")
        return()
    endif()

    if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT YUP_ARG_PLUGIN_CREATE_VST3 AND NOT YUP_ARG_PLUGIN_CREATE_STANDALONE AND NOT YUP_ARG_PLUGIN_CREATE_AU AND NOT YUP_ARG_PLUGIN_CREATE_AUv3 AND NOT YUP_ARG_PLUGIN_CREATE_AAX AND NOT YUP_ARG_PLUGIN_CREATE_LV2)
        _yup_message (FATAL_ERROR "At least one plugin type must be enabled (CLAP, VST3, AU, AUv3, AAX, or Standalone).")
        return()
    endif()

    set (target_modules "${YUP_ARG_MODULES}")
    list (APPEND target_modules yup_audio_plugin_client)
    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        list (APPEND target_modules yup_audio_devices)
    endif()
    _yup_module_check_circular_dependencies ("${target_name} audio plugin" "${target_modules}")

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
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ON
        XCODE_GENERATE_SCHEME ON)

    # ==== Find dependencies
    include (FetchContent)
    _yup_fetch_sdl()

    _yup_target_list_contains ("${YUP_ARG_MODULES}" yup_audio_plugin_host has_audio_plugin_host)
    if (has_audio_plugin_host)
        _yup_collect_audio_plugin_host_dependencies ("${YUP_ARG_DEFINITIONS}" audio_plugin_host_libraries)
        list (APPEND additional_libraries ${audio_plugin_host_libraries})
        if (audio_plugin_host_libraries)
            target_link_libraries (${target_name}_shared INTERFACE
                ${audio_plugin_host_libraries})
        endif()
    endif()

    # ==== Build CLAP plugin target
    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        _yup_audio_plugin_create_clap(
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build VST3 plugin target
    if (YUP_ARG_PLUGIN_CREATE_VST3)
        _yup_audio_plugin_create_vst3(
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build standalone plugin target
    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        _yup_audio_plugin_create_standalone(
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build AUv2 plugin target (macOS only)
    if (YUP_ARG_PLUGIN_CREATE_AU AND YUP_PLATFORM_MAC)
        _yup_audio_plugin_create_au(
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build AUv3 plugin target (macOS only for now, iOS support is planned)
    if (YUP_ARG_PLUGIN_CREATE_AUv3 AND YUP_PLATFORM_MAC)
        set (auv3_has_standalone OFF)
        set (auv3_standalone_target "")
        if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
            set (auv3_has_standalone ON)
            set (auv3_standalone_target "${target_name}_standalone_plugin")
        endif()

        yup_plugin_auv3(
            TARGET_BUNDLE_ID ${target_bundle_id}
            HAS_STANDALONE ${auv3_has_standalone}
            STANDALONE_TARGET ${auv3_standalone_target}
            SHARED_LIBS ${target_name}_shared
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build AAX plugin target
    if (YUP_ARG_PLUGIN_CREATE_AAX)
        _yup_audio_plugin_create_aax(
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
    endif()

    # ==== Build LV2 plugin target
    if (YUP_ARG_PLUGIN_CREATE_LV2)
        _yup_audio_plugin_create_lv2 (
            TARGET_BUNDLE_ID ${target_bundle_id}
            ADDITIONAL_LIBRARIES ${additional_libraries}
            ${ARGN})
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
    if (YUP_ARG_PLUGIN_CREATE_AUv3 AND YUP_PLATFORM_MAC)
        list (APPEND _all_plugin_targets ${target_name}_auv3_plugin)
    endif()
    if (YUP_ARG_PLUGIN_CREATE_AAX AND TARGET ${target_name}_aax_plugin)
        list (APPEND _all_plugin_targets ${target_name}_aax_plugin)
    endif()
    if (YUP_ARG_PLUGIN_CREATE_LV2)
        list (APPEND _all_plugin_targets ${target_name}_lv2_plugin)
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

    # Parse optional LV2 arguments
    cmake_parse_arguments (ARG "" "LV2_BUNDLE_DIR;LV2_BUNDLE_NAME" "" ${ARGN})

    string (TOUPPER "${plugin_type}" plugin_type_upper)
    set (dependency_target ${target_name}_${plugin_type}_plugin)

    if ("${plugin_type}" STREQUAL "au")
        set (target_file_name "${target_name}_${plugin_type}_plugin.component")
        set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/Components")
    elseif ("${plugin_type}" STREQUAL "auv3")
        set (target_file_name "${target_name}_${plugin_type}_plugin.appex")
        set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/AppExtensions")
    elseif ("${plugin_type}" STREQUAL "aax")
        set (target_file_name "${target_name}_${plugin_type}_plugin.aaxplugin")
        set (plugin_target_path "$ENV{HOME}/Library/Application Support/Avid/Audio/Plug-Ins")
    else()
        set (target_file_name "${target_name}_${plugin_type}_plugin.${plugin_type}")
        set (plugin_target_path "$ENV{HOME}/Library/Audio/Plug-Ins/${plugin_type_upper}")
    endif()

    set (plugin_path "${plugin_target_path}/${target_file_name}")

    _yup_message (STATUS "Generating rule to copy ${plugin_type} plugin ${target_name}")

    if ("${plugin_type}" STREQUAL "clap")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMENT "Symlinking CLAP plugin ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "vst3")
        if (YUP_PLATFORM_MAC AND NOT XCODE)
            set (source_plugin_path "$<TARGET_BUNDLE_DIR:${dependency_target}>")
        else()
            get_target_property (source_plugin_path ${dependency_target} SMTG_PLUGIN_PACKAGE_PATH)
            if (NOT source_plugin_path)
                set (source_plugin_path "$<TARGET_BUNDLE_DIR:${dependency_target}>")
            endif()
        endif()

        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${source_plugin_path}" "${plugin_path}"
            COMMENT "Symlinking VST3 plugin ${plugin_type_upper} plugin to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "au")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMENT "Copying AU plugin ${plugin_type_upper} to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "auv3")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMAND /bin/sh -c "killall -9 AudioComponentRegistrar 2>/dev/null; sleep 2; true"
            COMMENT "Copying AUv3 plugin ${plugin_type_upper} to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "aax")
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "$<TARGET_BUNDLE_DIR:${dependency_target}>" "${plugin_path}"
            COMMENT "Copying AAX plugin ${plugin_type_upper} to ${plugin_path}"
            VERBATIM)
    elseif ("${plugin_type}" STREQUAL "lv2")
        # The .lv2 bundle is already assembled in the build dir; symlink it into the system location
        add_custom_command(TARGET ${dependency_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${plugin_target_path}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${ARG_LV2_BUNDLE_DIR}" "${plugin_path}"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${plugin_target_path}/.DS_Store"
            COMMENT "Copying LV2 bundle to ${plugin_path}"
            VERBATIM)
    else()
        _yup_message (FATAL_ERROR "Unsupported plugin type ${plugin_type} for copying bundle")
    endif()
endfunction()
