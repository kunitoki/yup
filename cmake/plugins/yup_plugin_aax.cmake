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

function (_yup_find_aax_sdk)
    if (TARGET yup_audio_plugin_client_aaxsdk)
        return()
    endif()

    if (DEFINED YUP_AAX_SDK_ROOT)
        set (AAX_SDK_ROOT "${YUP_AAX_SDK_ROOT}")
    elseif (DEFINED ENV{YUP_AAX_SDK_ROOT})
        set (AAX_SDK_ROOT "$ENV{YUP_AAX_SDK_ROOT}")
    else()
        _yup_message (FATAL_ERROR "YUP_AAX_SDK_ROOT not set — AAX plugin support disabled")
        return()
    endif()

    if (NOT EXISTS "${AAX_SDK_ROOT}/Interfaces/AAX.h")
        _yup_message (FATAL_ERROR "AAX SDK not found at ${AAX_SDK_ROOT} (expected ${AAX_SDK_ROOT}/Interfaces/AAX.h)")
    endif()

    # --- Build AAX SDK headers interface ---
    add_library (yup_audio_plugin_client_aaxsdk_headers INTERFACE)
    target_include_directories (yup_audio_plugin_client_aaxsdk_headers INTERFACE
        "${AAX_SDK_ROOT}/Interfaces"
        "${AAX_SDK_ROOT}/Interfaces/ACF")
    target_compile_options (yup_audio_plugin_client_aaxsdk_headers INTERFACE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-multichar>
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-undef-prefix>)

    # --- Build AAXLibrary (static library from SDK sources) ---
    set (AAX_LIBRARY_SOURCE_DIR "${AAX_SDK_ROOT}/Libs/AAXLibrary/source")

    if (EXISTS "${AAX_LIBRARY_SOURCE_DIR}")
        # Collect source files, excluding platform-specific files for the wrong platform
        file (GLOB_RECURSE AAX_LIBRARY_SOURCES
            "${AAX_LIBRARY_SOURCE_DIR}/*.cpp")

        # AAX_CAutoreleasePool.Win.cpp and .OSX.mm define the same symbols unguarded
        if (YUP_PLATFORM_MAC)
            file (GLOB_RECURSE AAX_LIBRARY_WIN_SOURCES
                "${AAX_LIBRARY_SOURCE_DIR}/*.Win.cpp")
            if (AAX_LIBRARY_WIN_SOURCES)
                list (REMOVE_ITEM AAX_LIBRARY_SOURCES ${AAX_LIBRARY_WIN_SOURCES})
            endif()
            file (GLOB_RECURSE AAX_LIBRARY_OBJC_SOURCES
                "${AAX_LIBRARY_SOURCE_DIR}/*.mm")
            list (APPEND AAX_LIBRARY_SOURCES ${AAX_LIBRARY_OBJC_SOURCES})
        endif()

        # Class factory implementation required by AAX_Init.cpp
        list (APPEND AAX_LIBRARY_SOURCES "${AAX_SDK_ROOT}/Interfaces/ACF/CACFClassFactory.cpp")

        if (AAX_LIBRARY_SOURCES)
            add_library (yup_audio_plugin_client_aaxlib STATIC ${AAX_LIBRARY_SOURCES})
            target_compile_features (yup_audio_plugin_client_aaxlib PRIVATE cxx_std_17)
            target_include_directories (yup_audio_plugin_client_aaxlib PRIVATE
                "${AAX_SDK_ROOT}/Interfaces"
                "${AAX_SDK_ROOT}/Interfaces/ACF"
                "${AAX_SDK_ROOT}/Libs/AAXLibrary/include")
            target_compile_options (yup_audio_plugin_client_aaxlib PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-multichar>
                $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-undef-prefix>)
            target_compile_definitions (yup_audio_plugin_client_aaxlib PRIVATE
                AAX_LIBRARY_BINARY=1)

            if (WIN32)
                target_compile_definitions (yup_audio_plugin_client_aaxlib PRIVATE
                    NOMINMAX=1 WIN32_LEAN_AND_MEAN=1)
            endif()

            set_target_properties (yup_audio_plugin_client_aaxlib PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                FOLDER "Thirdparty")

            # Interface target that combines headers + library
            add_library (yup_audio_plugin_client_aaxsdk INTERFACE)
            target_link_libraries (yup_audio_plugin_client_aaxsdk INTERFACE
                yup_audio_plugin_client_aaxsdk_headers
                yup_audio_plugin_client_aaxlib)
            target_compile_definitions (yup_audio_plugin_client_aaxsdk INTERFACE
                AAX_LIBRARY_BINARY=1)

            set_target_properties (yup_audio_plugin_client_aaxsdk PROPERTIES
                YUP_AAX_SDK_ROOT "${AAX_SDK_ROOT}")

            _yup_message (STATUS "AAX SDK found and library built from ${AAX_SDK_ROOT}")
            return()
        endif()
    endif()

    # Fallback: headers-only if no source files found
    add_library (yup_audio_plugin_client_aaxsdk INTERFACE)
    target_link_libraries (yup_audio_plugin_client_aaxsdk INTERFACE
        yup_audio_plugin_client_aaxsdk_headers)

    set_target_properties (yup_audio_plugin_client_aaxsdk PROPERTIES
        YUP_AAX_SDK_ROOT "${AAX_SDK_ROOT}")

    _yup_message (WARNING "AAX SDK headers found but no library sources — plugin may not link properly")
endfunction()

#==============================================================================

function (_yup_audio_plugin_create_aax)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # ==== Validate required AAX IDs at cmake time
    if (NOT YUP_ARG_PLUGIN_AAX_MANUFACTURER_ID)
        _yup_message (FATAL_ERROR "PLUGIN_AAX_MANUFACTURER_ID is required when PLUGIN_CREATE_AAX is ON.")
    endif()
    if (NOT YUP_ARG_PLUGIN_AAX_PRODUCT_ID)
        _yup_message (FATAL_ERROR "PLUGIN_AAX_PRODUCT_ID is required when PLUGIN_CREATE_AAX is ON.")
    endif()
    if (NOT YUP_ARG_PLUGIN_AAX_PLUGIN_ID_NATIVE)
        _yup_message (FATAL_ERROR "PLUGIN_AAX_PLUGIN_ID_NATIVE is required when PLUGIN_CREATE_AAX is ON.")
    endif()
    if (NOT YUP_ARG_PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE)
        _yup_message (FATAL_ERROR "PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE is required when PLUGIN_CREATE_AAX is ON.")
    endif()

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

    _yup_find_aax_sdk()

    if (NOT TARGET yup_audio_plugin_client_aaxsdk)
        _yup_message (STATUS "Skipping AAX plugin target for ${target_name} (AAX SDK not available)")
        return()
    endif()

    _yup_message (STATUS "Setting up AAX plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        aax
        PLUGIN_ID ${YUP_ARG_PLUGIN_ID}
        PLUGIN_NAME ${YUP_ARG_PLUGIN_NAME}
        PLUGIN_VENDOR ${YUP_ARG_PLUGIN_VENDOR}
        PLUGIN_EMAIL ${YUP_ARG_PLUGIN_EMAIL}
        PLUGIN_VERSION ${YUP_ARG_PLUGIN_VERSION}
        PLUGIN_DESCRIPTION ${YUP_ARG_PLUGIN_DESCRIPTION}
        PLUGIN_URL ${YUP_ARG_PLUGIN_URL}
        PLUGIN_IS_SYNTH ${YUP_ARG_PLUGIN_IS_SYNTH}
        PLUGIN_IS_MONO ${YUP_ARG_PLUGIN_IS_MONO}
        PLUGIN_AAX_MANUFACTURER_ID ${YUP_ARG_PLUGIN_AAX_MANUFACTURER_ID}
        PLUGIN_AAX_PRODUCT_ID ${YUP_ARG_PLUGIN_AAX_PRODUCT_ID}
        PLUGIN_AAX_PLUGIN_ID_NATIVE ${YUP_ARG_PLUGIN_AAX_PLUGIN_ID_NATIVE}
        PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE ${YUP_ARG_PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE}
        PLUGIN_AAX_CATEGORY ${YUP_ARG_PLUGIN_AAX_CATEGORY}
        PLUGIN_AAX_PAGE_TABLE_FILE ${YUP_ARG_PLUGIN_AAX_PAGE_TABLE_FILE})

    # Create AAX plugin target
    _yup_message (STATUS "Creating AAX plugin target")
    if (YUP_PLATFORM_MAC)
        add_library (${target_name}_aax_plugin MODULE)
    else()
        add_library (${target_name}_aax_plugin SHARED)
    endif()

    target_compile_features (${target_name}_aax_plugin PRIVATE cxx_std_20)

    target_compile_definitions (${target_name}_aax_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_AAX=1
        YUP_STANDALONE_APPLICATION=0)

    _yup_sdl_configure_symbols_patch ("${target_name}_aax_plugin" aax_sdl_symbols_patch_target aax_sdl_symbols_sdl_target)
    set (aax_plugin_bundle_libraries
        ${aax_sdl_symbols_sdl_target}
        ${aax_sdl_symbols_patch_target})

    # The ACF entry points (ACFRegisterPlugin, ACFStartup, ...) are defined in the SDK's AAX_Exports.cpp
    get_target_property (aax_sdk_root yup_audio_plugin_client_aaxsdk YUP_AAX_SDK_ROOT)
    target_sources (${target_name}_aax_plugin PRIVATE
        "${aax_sdk_root}/Interfaces/AAX_Exports.cpp")

    target_link_libraries (${target_name}_aax_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        yup_audio_plugin_client_aaxsdk
        ${target_name}_aax
        ${additional_libraries}
        ${aax_plugin_bundle_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_aax_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        yup_audio_plugin_client_aaxsdk
        ${target_name}_aax
        ${additional_libraries}
        ${aax_plugin_bundle_libraries}
        ${target_modules})

    set_target_properties (${target_name}_aax_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_aax_plugin)

    if (YUP_PLATFORM_MAC)
        set (aax_plist_output "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_aax_plugin.plist")
        _yup_configure_audio_plugin_bundle_info_plist ("${aax_plist_output}" "TDMw")

        set_target_properties (${target_name}_aax_plugin PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION "aaxplugin"
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${aax_plist_output}"
            MACOSX_BUNDLE_BUNDLE_NAME "${target_name}_aax_plugin"
            MACOSX_BUNDLE_BUNDLE_VERSION "${target_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${target_version}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${target_bundle_id}.aax"
            XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_PACKAGE_TYPE TDMw
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${target_bundle_id}.aax"
            PREFIX "")

        set (aax_plugin_path "$<TARGET_BUNDLE_DIR:${target_name}_aax_plugin>")
    else()
        # Windows AAX plugins are directory bundles laid out as
        # Name.aaxplugin/Contents/x64/Name.aaxplugin
        set (aax_bundle_dir "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_aax_plugin.aaxplugin")

        set_target_properties (${target_name}_aax_plugin PROPERTIES
            PREFIX ""
            SUFFIX ".aaxplugin"
            OUTPUT_NAME "${target_name}_aax_plugin"
            RUNTIME_OUTPUT_DIRECTORY "$<1:${aax_bundle_dir}/Contents/x64>"
            LIBRARY_OUTPUT_DIRECTORY "$<1:${aax_bundle_dir}/Contents/x64>")

        set (aax_plugin_path "${aax_bundle_dir}")
    endif()

    if (YUP_PLATFORM_MAC)
        yup_codesign_target (${target_name}_aax_plugin "${aax_plugin_path}"
            "${YUP_ARG_PLUGIN_CODESIGN_IDENTITY}"
            "${YUP_ARG_PLUGIN_HARDENED_RUNTIME}")
    endif()

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} aax)
    endif()
endfunction()
