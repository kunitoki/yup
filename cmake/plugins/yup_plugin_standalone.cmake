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

function (_yup_audio_plugin_create_standalone)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_ide_group "${YUP_ARG_TARGET_IDE_GROUP}")
    set (target_bundle_id "${YUP_ARG_TARGET_BUNDLE_ID}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")

    _yup_message (STATUS "Setting up standalone plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        standalone)

    # ==== The application is named after the plugin, the target keeps its own name
    set (product_args "")
    if (YUP_ARG_PLUGIN_NAME)
        list (APPEND product_args TARGET_PRODUCT_NAME "${YUP_ARG_PLUGIN_NAME}")
    endif()

    # ==== Forward only the emscripten options that were given, so yup_standalone_app keeps its defaults
    set (emscripten_args "")
    foreach (emscripten_arg CUSTOM_SHELL INITIAL_MEMORY PTHREAD_POOL_SIZE STACK_SIZE PRELOAD_FILES)
        if (YUP_ARG_${emscripten_arg})
            list (APPEND emscripten_args ${emscripten_arg} ${YUP_ARG_${emscripten_arg}})
        endif()
    endforeach()

    _yup_message (STATUS "Creating standalone plugin target")
    yup_standalone_app (
        TARGET_NAME ${target_name}_standalone_plugin
        ${product_args}
        TARGET_VERSION ${target_version}
        TARGET_IDE_GROUP ${target_ide_group}
        TARGET_APP_ID ${target_bundle_id}
        TARGET_APP_NAMESPACE ${target_app_namespace}
        TARGET_CXX_STANDARD ${target_cxx_standard}
        ${emscripten_args}
        DEFINITIONS
            YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1
        MODULES
            ${target_name}_shared
            ${target_name}_standalone)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_standalone_plugin)
endfunction()
