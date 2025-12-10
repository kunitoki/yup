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

function (yup_standalone_app)
    # ==== Fetch options
    set (options "")
    set (one_value_args
        # Globals
        TARGET_NAME TARGET_VERSION TARGET_CONSOLE TARGET_IDE_GROUP TARGET_APP_NAMESPACE TARGET_ICON TARGET_WHEEL TARGET_CXX_STANDARD
        # Emscripten
        INITIAL_MEMORY PTHREAD_POOL_SIZE CUSTOM_PLIST CUSTOM_SHELL)
    set (multi_value_args
        # Globals
        DEFINITIONS COMPILE_OPTIONS MODULES LINK_OPTIONS
        # Emscripten
        PRELOAD_FILES)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_set_default (YUP_ARG_TARGET_CXX_STANDARD 20)
    _yup_set_default (YUP_ARG_TARGET_ICON "${CMAKE_SOURCE_DIR}/cmake/resources/app-icon.png")

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_console "${YUP_ARG_TARGET_CONSOLE}")
    set (target_icon "${YUP_ARG_TARGET_ICON}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_app_identifier "${target_app_namespace}.${target_name}")
    set (target_wheel "${YUP_ARG_TARGET_WHEEL}")
    set (target_resources "")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    _yup_set_default (target_console OFF)
    _yup_set_default (target_wheel OFF)
    _yup_make_short_version ("${target_version}" target_version_short)

    # ==== Output status
    _yup_message (STATUS "${target_name} - Configuring standalone application")

    # ==== Setup Android platform, build gradle stage
    if (YUP_TARGET_ANDROID)
        _yup_message (STATUS "${target_name} - Creating java gradle project")
        _yup_prepare_gradle_android(
            TARGET_NAME ${target_name}
            TARGET_ICON ${target_icon}
            APPLICATION_ID ${target_app_identifier}
            APPLICATION_NAMESPACE ${target_app_namespace}
            APPLICATION_VERSION ${target_version})

        _yup_message (STATUS "${target_name} - Copying SDL2 java activity to application")
        _yup_fetch_sdl2()
        _yup_copy_sdl2_activity_android()

        return()
    endif()

    # ==== Find dependencies
    if (NOT "${target_console}" AND NOT YUP_PLATFORM_EMSCRIPTEN)
        _yup_message (STATUS "${target_name} - Fetching SDL2 library")
        _yup_fetch_sdl2()
        list (APPEND additional_libraries sdl2::sdl2)
    endif()

    # ==== Enable profiling
    if (YUP_ENABLE_PROFILING AND NOT "${target_name}" STREQUAL "yup_tests")
        list (APPEND additional_definitions YUP_ENABLE_PROFILING=1)
        list (APPEND additional_libraries perfetto::perfetto)
    endif()

    # ==== Prepare executable
    set (executable_options "")
    if (NOT "${target_console}")
        if (YUP_PLATFORM_WINDOWS)
            set (executable_options "WIN32")
        elseif (YUP_PLATFORM_MAC)
            set (executable_options "MACOSX_BUNDLE")
        endif()
    endif()

    if (YUP_PLATFORM_ANDROID)
        add_library (${target_name} SHARED)
    elseif (target_wheel)
        add_library (${target_name} MODULE)
    else()
        add_executable (${target_name} ${executable_options})
    endif()

    target_compile_features (${target_name} PRIVATE cxx_std_${target_cxx_standard})

    # ==== Per platform configuration
    if (YUP_PLATFORM_APPLE)
        if (NOT "${target_console}" AND NOT "${target_wheel}")
            _yup_set_default (YUP_ARG_CUSTOM_PLIST "${CMAKE_SOURCE_DIR}/cmake/platforms/${YUP_PLATFORM}/Info.plist")
            _yup_valid_identifier_string ("${target_app_identifier}" target_app_identifier)

            _yup_message (STATUS "${target_name} - Converting application input icon to apple .icns format")
            _yup_convert_png_to_icns ("${target_icon}" "${CMAKE_CURRENT_BINARY_DIR}/icons" target_iconset)
            get_filename_component (target_iconset_name "${target_iconset}" NAME)
            target_sources (${target_name} PRIVATE ${target_iconset})
            list (APPEND target_resources "${target_iconset}")

            set_target_properties (${target_name} PROPERTIES
                BUNDLE                                         ON
                CXX_EXTENSIONS                                 OFF
                MACOSX_BUNDLE_EXECUTABLE_NAME                  "${target_name}"
                MACOSX_BUNDLE_GUI_IDENTIFIER                   "${target_app_identifier}"
                MACOSX_BUNDLE_BUNDLE_NAME                      "${target_name}"
                MACOSX_BUNDLE_BUNDLE_VERSION                   "${target_version}"
                MACOSX_BUNDLE_LONG_VERSION_STRING              "${target_version}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING             "${target_version_short}"
                MACOSX_BUNDLE_INFO_PLIST                       "${YUP_ARG_CUSTOM_PLIST}"
                MACOSX_BUNDLE_ICON_FILE                        "${target_iconset_name}"
                RESOURCE                                       "${target_resources}"
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER      "${target_app_identifier}")

        endif()

        set_target_properties (${target_name} PROPERTIES
            #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY             ""
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED          OFF
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       dwarf
            XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC          OFF
            XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME        OFF
            XCODE_GENERATE_SCHEME                          ON)

    elseif (YUP_PLATFORM_EMSCRIPTEN)
        if (NOT "${target_console}")
            set_target_properties (${target_name} PROPERTIES SUFFIX ".html")
        endif()

        _yup_set_default (YUP_ARG_CUSTOM_SHELL "${CMAKE_SOURCE_DIR}/cmake/platforms/${YUP_PLATFORM}/shell.html")
        _yup_set_default (YUP_ARG_INITIAL_MEMORY 33554432) # 32mb
        _yup_set_default (YUP_ARG_PTHREAD_POOL_SIZE 8)

        list (APPEND additional_options
            $<$<CONFIG:DEBUG>:-O0 -g>
            $<$<CONFIG:RELEASE>:-O3>
            -fexceptions
            -pthread
            -Wno-nontrivial-memcall
            -sDISABLE_EXCEPTION_CATCHING=0)

        list (APPEND additional_link_options
            $<$<CONFIG:DEBUG>:-gsource-map -g>
            -fexceptions
            -pthread
            -Wno-pthreads-mem-growth
            -sWASM=1
            -sWASM_WORKERS=1
            -sAUDIO_WORKLET=1
            -sPTHREAD_POOL_SIZE=${YUP_ARG_PTHREAD_POOL_SIZE}
            -sSHARED_MEMORY=1
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=${YUP_ARG_INITIAL_MEMORY}
            -sASSERTIONS=1
            -sDISABLE_EXCEPTION_CATCHING=0
            -sERROR_ON_UNDEFINED_SYMBOLS=1
            -sSTACK_OVERFLOW_CHECK=2
            -sFORCE_FILESYSTEM=1
            -sNODERAWFS=0
            -sWASMFS=1
            -sFETCH=1
            #-sASYNCIFY=1
            -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
            -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$dynCall'
            --shell-file=${YUP_ARG_CUSTOM_SHELL})

        foreach (preload_file IN ITEMS ${YUP_ARG_PRELOAD_FILES})
            list (APPEND additional_link_options "--preload-file=${preload_file}")
        endforeach()

        set (target_copy_dest "$<TARGET_FILE_DIR:${target_name}>")
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/cmake/platforms/${YUP_PLATFORM}/mini-coi.js"
                "${target_copy_dest}/mini-coi.js")

    elseif (YUP_PLATFORM_LINUX)
        set_target_properties (${target_name} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

    endif()

    if (YUP_ARG_TARGET_IDE_GROUP)
        set_target_properties (${target_name} PROPERTIES FOLDER "${YUP_ARG_TARGET_IDE_GROUP}")
    endif()

    # ==== Definitions and link libraries
    target_compile_options (${target_name} PRIVATE
        ${additional_options}
        ${YUP_ARG_COMPILE_OPTIONS})

    target_compile_definitions (${target_name} PRIVATE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        YUP_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        YUP_STANDALONE_APPLICATION=1
        ${additional_definitions}
        ${YUP_ARG_DEFINITIONS})

    target_link_options (${target_name} PRIVATE
        ${additional_link_options}
        ${YUP_ARG_LINK_OPTIONS})

    target_link_libraries (${target_name} PRIVATE
        ${additional_libraries}
        ${YUP_ARG_MODULES})

endfunction()
