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

# Shared argument lists for all plugin functions (yup_audio_plugin and each
# _yup_audio_plugin_create_* / yup_plugin_auv3).
#
# Every function that receives user-facing keyword arguments must recognise
# the full set so that cmake_parse_arguments doesn't dump everything after the
# first unknown keyword into UNPARSED_ARGUMENTS.
#
# Call _yup_plugin_shared_args(one_value_args multi_value_args) to populate the base arg lists, then optionally
# append extra format-specific keywords before calling cmake_parse_arguments.

function (_yup_plugin_shared_args one_value_output multi_value_output)
    set (${one_value_output}
        TARGET_NAME TARGET_VERSION TARGET_APP_ID TARGET_APP_NAMESPACE TARGET_CXX_STANDARD TARGET_IDE_GROUP TARGET_BUNDLE_ID
        PLUGIN_CREATE_CLAP PLUGIN_CREATE_VST3 PLUGIN_CREATE_STANDALONE PLUGIN_CREATE_AU PLUGIN_CREATE_AUv3 PLUGIN_CREATE_AAX PLUGIN_CREATE_LV2
        PLUGIN_ID PLUGIN_NAME PLUGIN_VENDOR PLUGIN_EMAIL PLUGIN_VERSION PLUGIN_DESCRIPTION PLUGIN_URL
        PLUGIN_IS_SYNTH PLUGIN_IS_MONO
        PLUGIN_AU_SUBTYPE PLUGIN_AU_MANUFACTURER PLUGIN_AU_SANDBOX_SAFE
        PLUGIN_AAX_MANUFACTURER_ID PLUGIN_AAX_PRODUCT_ID PLUGIN_AAX_PLUGIN_ID_NATIVE PLUGIN_AAX_PLUGIN_ID_AUDIOSUITE
        PLUGIN_AAX_CATEGORY PLUGIN_AAX_PAGE_TABLE_FILE
        PLUGIN_VST3_AUTO_MANIFEST
        PLUGIN_CODESIGN_IDENTITY PLUGIN_CODESIGN_TEAM PLUGIN_APPLE_ENTITLEMENTS PLUGIN_HARDENED_RUNTIME
        PLUGIN_COPYRIGHT
        PLUGIN_COPY_AFTER_BUILD
        PLUGIN_ICON_BIG PLUGIN_ICON_SMALL
        HAS_STANDALONE STANDALONE_TARGET
        PARENT_SCOPE)

    set (${multi_value_output}
        DEFINITIONS MODULES LINK_OPTIONS
        SHARED_LIBS
        ADDITIONAL_LIBRARIES
        PLUGIN_CLAP_FEATURES PLUGIN_VST3_CATEGORIES PLUGIN_HARDENED_RUNTIME_OPTIONS PLUGIN_CUSTOM_PLIST
        PARENT_SCOPE)
endfunction()
