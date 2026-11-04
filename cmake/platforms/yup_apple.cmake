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

include_guard (DIRECTORY)

# ==============================================================================

macro (_yup_apple_setup_deployment_target)
    execute_process (COMMAND xcodebuild -version OUTPUT_VARIABLE xcode_version_output ERROR_QUIET)
    string (REGEX MATCH "Xcode ([0-9]+)" _ "${xcode_version_output}")
    _yup_message (STATUS "Detected Xcode version ${CMAKE_MATCH_1}")
    if (CMAKE_MATCH_1 GREATER_EQUAL 27)
        if (YUP_PLATFORM_MAC)
            set (CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
        else()
            set (CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
        endif()
    else()
        if (YUP_PLATFORM_MAC)
            if (CMAKE_OSX_DEPLOYMENT_TARGET LESS 11.0)
                set (CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
            endif()
        else()
            if (CMAKE_OSX_DEPLOYMENT_TARGET LESS 13.0)
                set (CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
            endif()
        endif()
    endif()

    if (NOT YUP_PLATFORM_MAC AND CMAKE_OSX_DEPLOYMENT_TARGET)
        if (NOT DEFINED DEPLOYMENT_TARGET OR DEPLOYMENT_TARGET VERSION_LESS CMAKE_OSX_DEPLOYMENT_TARGET)
            set (DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
        endif()
    endif()

    _yup_message (STATUS "CMAKE_OSX_DEPLOYMENT_TARGET is set to ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endmacro()

# ==============================================================================

# Applies the platform settings of a yup_standalone_app target. Called from yup_standalone_app,
# so it reads its parsed YUP_ARG_* arguments and target_* variables directly.
function (_yup_platform_setup_standalone_app target_name)
    if (NOT "${target_console}" AND NOT "${target_wheel}" AND NOT "${target_static_library}")
        _yup_set_default (YUP_ARG_CUSTOM_PLIST "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${YUP_PLATFORM}/ApplicationInfo.plist")
        _yup_valid_identifier_string ("${target_app_identifier}" target_app_identifier)

        _yup_convert_png_to_icns ("${target_icon}" "${CMAKE_CURRENT_BINARY_DIR}/icons" target_iconset)
        get_filename_component (target_iconset_name "${target_iconset}" NAME)
        target_sources (${target_name} PRIVATE ${target_iconset})
        list (APPEND target_resources "${target_iconset}")

        if (YUP_ARG_TARGET_ICON_COMPOSER)
            string (SHA256 icon_composer_key "${YUP_ARG_TARGET_ICON_COMPOSER}")
            string (SUBSTRING "${icon_composer_key}" 0 16 icon_composer_key)
            _yup_compile_icon_composer ("${YUP_ARG_TARGET_ICON_COMPOSER}" "${CMAKE_BINARY_DIR}/icon_composer/${icon_composer_key}" target_asset_catalog)
            if (target_asset_catalog)
                target_sources (${target_name} PRIVATE ${target_asset_catalog})
                list (APPEND target_resources "${target_asset_catalog}")
            endif()
        endif()

        set_target_properties (${target_name} PROPERTIES
            BUNDLE                                         ON
            CXX_EXTENSIONS                                 OFF
            MACOSX_BUNDLE_EXECUTABLE_NAME                  "${target_product_name}"
            MACOSX_BUNDLE_GUI_IDENTIFIER                   "${target_app_identifier}"
            MACOSX_BUNDLE_BUNDLE_NAME                      "${target_product_name}"
            MACOSX_BUNDLE_BUNDLE_VERSION                   "${target_version}"
            MACOSX_BUNDLE_LONG_VERSION_STRING              "${target_version}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING             "${target_version_short}"
            MACOSX_BUNDLE_INFO_PLIST                       "${YUP_ARG_CUSTOM_PLIST}"
            MACOSX_BUNDLE_ICON_FILE                        "${target_iconset_name}"
            RESOURCE                                       "${target_resources}"
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER      "${target_app_identifier}")

        if (YUP_ARG_BUNDLE_RESOURCES)
            yup_add_bundled_resources (${target_name} RESOURCES ${YUP_ARG_BUNDLE_RESOURCES})
        endif()

    endif()

    target_compile_options (${target_name} PRIVATE
        -fno-unwind-tables
        -fno-asynchronous-unwind-tables)

    set_target_properties (${target_name} PROPERTIES
        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY             ""
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED          OFF
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       dwarf
        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC          ON
        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME        OFF
        XCODE_GENERATE_SCHEME                          ON)
endfunction()
