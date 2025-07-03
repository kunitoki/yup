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

cmake_minimum_required (VERSION 3.28)

_yup_get_project_version_string (${CMAKE_CURRENT_LIST_DIR}/../modules yup_version)
_yup_message (STATUS "Building project version ${yup_version}")

set (target_name yup)
set (target_version ${yup_version})
project (${target_name} VERSION ${target_version})

yup_add_default_modules ("${CMAKE_CURRENT_LIST_DIR}/.." DEFINITIONS
    YUP_STANDALONE_APPLICATION=1
    YUP_DISABLE_JUCE_VERSION_PRINTING=1
    YUP_MODAL_LOOPS_PERMITTED=1
    YUP_CATCH_UNHANDLED_EXCEPTIONS=1
    YUP_LOG_ASSERTIONS=1
    YUP_ALLOW_STATIC_NULL_VARIABLES=0
    YUP_STRICT_REFCOUNTEDPOINTER=1
    YUP_LOAD_CURL_SYMBOLS_LAZILY=1
    YUP_PYTHON_EMBEDDED_INTERPRETER=0
    YUP_PYTHON_SCRIPT_CATCH_EXCEPTION=0
    PYBIND11_DETAILED_ERROR_MESSAGES=1)

yup_standalone_app (
    TARGET_NAME ${target_name}
    TARGET_VERSION ${target_version}
    TARGET_IDE_GROUP "Wheel"
    TARGET_APP_ID "org.yup.${target_name}"
    TARGET_APP_NAMESPACE "org.yup"
    TARGET_WHEEL ON
    MODULES
        yup::yup_audio_basics
        yup::yup_audio_devices
        yup::yup_audio_processors
        yup::yup_core
        yup::yup_data_model
        yup::yup_events
        yup::yup_graphics
        yup::yup_gui
        yup::yup_python)

set_target_properties (${target_name} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
set_target_properties (${target_name} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
set_target_properties (${target_name} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties (${target_name} PROPERTIES OUTPUT_NAME "${target_name}")
set_target_properties (${target_name} PROPERTIES PREFIX "")

if (APPLE)
    set_target_properties (${target_name} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    target_link_options (${target_name} PRIVATE "-Wl,-weak_reference_mismatches,weak")
elseif (WIN32)
    target_compile_definitions (${target_name} PUBLIC $<$<CONFIG:Debug>:Py_DEBUG>)
    set_target_properties (${target_name} PROPERTIES SUFFIX ".pyd")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties (${target_name} PROPERTIES OUTPUT_NAME "${target_name}_d")
    endif()
endif()

if (YUP_ENABLE_WHEEL_DISTRIBUTION)
    if (APPLE)
        add_custom_command(
            TARGET "${target_name}" POST_BUILD DEPENDS "${target_name}"
            COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
            ARGS -x $<TARGET_FILE:${target_name}>)
    elseif (UNIX)
        add_custom_command(
            TARGET "${target_name}" POST_BUILD DEPENDS "${target_name}"
            COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
            ARGS --strip-all $<TARGET_FILE:${target_name}>)
    endif()
endif()

target_include_directories (${target_name} PRIVATE "${Python_INCLUDE_DIRS}")
if (NOT "${Python_LIBRARY_DIRS}" STREQUAL "")
    target_link_directories (${target_name} PRIVATE "${Python_LIBRARY_DIRS}")
endif()

target_sources (${target_name} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../modules/yup_core/system/yup_StandardHeader.h")
