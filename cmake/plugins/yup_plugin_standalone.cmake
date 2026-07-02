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

function (_yup_audio_plugin_create_standalone
    target_name
    target_version
    target_ide_group
    target_bundle_id
    target_app_namespace
    target_cxx_standard
    additional_libraries
    target_modules
    unparsed_args)

    _yup_message (STATUS "Setting up standalone plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        standalone
        ${unparsed_args})

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
            ${target_modules})
endfunction()
