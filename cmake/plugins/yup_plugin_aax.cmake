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
        _yup_message (STATUS "YUP_AAX_SDK_ROOT not set — AAX plugin support disabled")
        return()
    endif()

    if (NOT EXISTS "${AAX_SDK_ROOT}/Interfaces/AAX.h")
        _yup_message (FATAL_ERROR "AAX SDK not found at ${AAX_SDK_ROOT} "
            "(expected ${AAX_SDK_ROOT}/Interfaces/AAX.h)")
    endif()

    # --- Build AAX SDK headers interface ---
    add_library (yup_audio_plugin_client_aaxsdk_headers INTERFACE)
    target_include_directories (yup_audio_plugin_client_aaxsdk_headers INTERFACE
        "${AAX_SDK_ROOT}/Interfaces"
        "${AAX_SDK_ROOT}/Interfaces/ACF")

    # --- Build AAXLibrary (static library from SDK sources) ---
    set (AAX_LIBRARY_SOURCE_DIR "${AAX_SDK_ROOT}/Libs/AAXLibrary/source")

    if (EXISTS "${AAX_LIBRARY_SOURCE_DIR}")
        # Collect source files, excluding platform-specific files for the wrong platform
        file (GLOB_RECURSE AAX_LIBRARY_SOURCES
            "${AAX_LIBRARY_SOURCE_DIR}/*.cpp")

        if (YUP_PLATFORM_MAC)
            file (GLOB_RECURSE AAX_LIBRARY_OBJC_SOURCES
                "${AAX_LIBRARY_SOURCE_DIR}/*.mm"
                "${AAX_LIBRARY_SOURCE_DIR}/*.OSX.*")
            list (APPEND AAX_LIBRARY_SOURCES ${AAX_LIBRARY_OBJC_SOURCES})
        else()
            # Exclude macOS-specific files on Windows/Linux
            file (GLOB_RECURSE AAX_LIBRARY_OSX_SOURCES
                "${AAX_LIBRARY_SOURCE_DIR}/*_OSX.*"
                "${AAX_LIBRARY_SOURCE_DIR}/*.mm")
            if (AAX_LIBRARY_OSX_SOURCES)
                list (REMOVE_ITEM AAX_LIBRARY_SOURCES ${AAX_LIBRARY_OSX_SOURCES})
            endif()
        endif()

        if (AAX_LIBRARY_SOURCES)
            add_library (yup_audio_plugin_client_aaxlib STATIC ${AAX_LIBRARY_SOURCES})
            target_compile_features (yup_audio_plugin_client_aaxlib PRIVATE cxx_std_17)
            target_include_directories (yup_audio_plugin_client_aaxlib PRIVATE
                "${AAX_SDK_ROOT}/Interfaces"
                "${AAX_SDK_ROOT}/Interfaces/ACF")
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

function (_yup_audio_plugin_create_aax
    target_name
    target_version
    target_ide_group
    target_bundle_id
    target_app_namespace
    target_cxx_standard
    additional_libraries
    target_modules
    unparsed_args)

    _yup_find_aax_sdk()

    _yup_message (STATUS "Setting up AAX plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        aax
        ${unparsed_args})

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

    target_link_libraries (${target_name}_aax_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        yup_audio_plugin_client_aaxsdk
        ${target_name}_aax
        sdl2::sdl2
        ${additional_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_aax_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        yup_audio_plugin_client_aaxsdk
        ${target_name}_aax
        ${additional_libraries}
        ${target_modules})

    set_target_properties (${target_name}_aax_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

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
        # Windows AAX: directory bundle with .aaxplugin extension
        set_target_properties (${target_name}_aax_plugin PROPERTIES
            PREFIX ""
            SUFFIX ".aaxplugin")

        set (aax_plugin_path "$<TARGET_FILE_DIR:${target_name}_aax_plugin>")
    endif()

    yup_codesign_target (${target_name}_aax_plugin "${aax_plugin_path}")
endfunction()
