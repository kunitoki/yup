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

set (target_name example_app)

file (GLOB_RECURSE sources
    "${CMAKE_CURRENT_LIST_DIR}/source/*.cpp")

source_group (TREE ${CMAKE_CURRENT_LIST_DIR}/ FILES ${sources})

add_executable (${target_name})

target_compile_features (${target_name} PRIVATE cxx_std_17)

target_sources (${target_name} PRIVATE ${sources})

if (APPLE)
    set_target_properties (${target_name} PROPERTIES
        BUNDLE                                         ON
        CXX_STANDARD                                   17
        CXX_EXTENSIONS                                 OFF
        MACOSX_BUNDLE_GUI_IDENTIFIER                   "org.kunitoki.yup.${target_name}"
        MACOSX_BUNDLE_NAME                             "${target_name}"
        MACOSX_BUNDLE_ERSION                           "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING             "1.0"
        #MACOSX_BUNDLE_ICON_FILE                        "Icon.icns"
        MACOSX_BUNDLE_INFO_PLIST                       "${CMAKE_CURRENT_LIST_DIR}/../../cmake/platforms/macos/Info.plist"
        #RESOURCE                                       "${RESOURCE_FILES}"
        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY             ""
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED          OFF
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       dwarf
        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME        OFF)
endif()

if ("${yup_platform}" STREQUAL "emscripten")
    target_link_options (${target_name} PRIVATE
        -sWASM=1 -sASSERTIONS=1 -sUSE_GLFW=3 -sERROR_ON_UNDEFINED_SYMBOLS=1 -sDEMANGLE_SUPPORT=1 -sSTACK_OVERFLOW_CHECK=2 -sFORCE_FILESYSTEM=1 -sNODERAWFS=0)
endif()

target_compile_definitions (${target_name} PRIVATE
    $<$<CONFIG:DEBUG>:DEBUG=1>
    $<$<CONFIG:RELEASE>:NDEBUG=1>
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1)

target_link_libraries (${target_name} PRIVATE
    juce_core
    juce_events
    juce_audio_basics
    juce_audio_devices)
