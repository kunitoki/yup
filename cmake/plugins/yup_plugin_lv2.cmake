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

function (_yup_fetch_lv2_sdk)
    if (NOT TARGET lv2-headers)
        _yup_message (STATUS "Fetching LV2 SDK headers")
        _yup_fetchcontent_declare (lv2-sdk
            GIT_REPOSITORY https://github.com/lv2/lv2.git
            GIT_TAG v1.18.10)

        FetchContent_MakeAvailable (lv2-sdk)

        add_library (lv2-headers INTERFACE)
        target_include_directories (lv2-headers INTERFACE
            "$<BUILD_INTERFACE:${lv2-sdk_SOURCE_DIR}>"
            "$<BUILD_INTERFACE:${lv2-sdk_SOURCE_DIR}/include>")

        set_target_properties (lv2-headers PROPERTIES
            FOLDER "Thirdparty")
    endif()
endfunction()

#==============================================================================
# Minimal CMake wrapper for serd (RDF syntax library)
# Source: https://github.com/drobilla/serd

function (_yup_fetch_serd)
    if (NOT TARGET serd-static)
        _yup_message (STATUS "Fetching serd")
        _yup_fetchcontent_declare (serd
            GIT_REPOSITORY https://github.com/drobilla/serd.git
            GIT_TAG v0.32.4)

        FetchContent_MakeAvailable (serd)

        add_library (serd-static STATIC)

        file (GLOB_RECURSE serd_sources "${serd_SOURCE_DIR}/src/*.c")
        list (FILTER serd_sources EXCLUDE REGEX ".*/test/.*|.*_test\\.c$")
        target_sources (serd-static PRIVATE ${serd_sources})

        target_include_directories (serd-static PUBLIC
            "$<BUILD_INTERFACE:${serd_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${serd_BINARY_DIR}>")

        target_compile_definitions (serd-static PRIVATE
            SERD_INTERNAL
            SERD_STATIC)

        configure_file ("${serd_SOURCE_DIR}/include/serd/serd_config.h.in"
                        "${serd_BINARY_DIR}/serd/serd_config.h" @ONLY)

        set_target_properties (serd-static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            FOLDER "Thirdparty"
            C_VISIBILITY_PRESET hidden)

        install (TARGETS serd-static EXPORT serd-targets)
    endif()
endfunction()

#==============================================================================
# Minimal CMake wrapper for sord (in-memory RDF store)
# Source: https://github.com/drobilla/sord

function (_yup_fetch_sord)
    if (NOT TARGET sord-static)
        _yup_message (STATUS "Fetching sord")

        _yup_fetch_serd()

        _yup_fetchcontent_declare (sord
            GIT_REPOSITORY https://github.com/drobilla/sord.git
            GIT_TAG v0.16.18)

        FetchContent_MakeAvailable (sord)

        add_library (sord-static STATIC)

        file (GLOB_RECURSE sord_sources "${sord_SOURCE_DIR}/src/*.c")
        list (FILTER sord_sources EXCLUDE REGEX ".*/test/.*|.*_test\\.c$")
        target_sources (sord-static PRIVATE ${sord_sources})

        target_include_directories (sord-static PUBLIC
            "$<BUILD_INTERFACE:${sord_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${sord_BINARY_DIR}>")

        target_link_libraries (sord-static PUBLIC serd-static)

        target_compile_definitions (sord-static PRIVATE
            SORD_INTERNAL
            SORD_STATIC)

        configure_file ("${sord_SOURCE_DIR}/include/sord/sord_config.h.in"
                        "${sord_BINARY_DIR}/sord/sord_config.h" @ONLY)

        set_target_properties (sord-static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            FOLDER "Thirdparty"
            C_VISIBILITY_PRESET hidden)

        install (TARGETS sord-static EXPORT sord-targets)
    endif()
endfunction()

#==============================================================================
# Minimal CMake wrapper for zix (data structures library)
# Source: https://github.com/drobilla/zix

function (_yup_fetch_zix)
    if (NOT TARGET zix-static)
        _yup_message (STATUS "Fetching zix")

        _yup_fetchcontent_declare (zix
            GIT_REPOSITORY https://github.com/drobilla/zix.git
            GIT_TAG v0.6.2)

        FetchContent_MakeAvailable (zix)

        add_library (zix-static STATIC)

        file (GLOB_RECURSE zix_sources "${zix_SOURCE_DIR}/src/*.c")
        list (FILTER zix_sources EXCLUDE REGEX ".*/test/.*|.*_test\\.c$")
        target_sources (zix-static PRIVATE ${zix_sources})

        target_include_directories (zix-static PUBLIC
            "$<BUILD_INTERFACE:${zix_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${zix_BINARY_DIR}>")

        target_compile_definitions (zix-static PRIVATE ZIX_STATIC)

        configure_file ("${zix_SOURCE_DIR}/include/zix/config.h.in"
                        "${zix_BINARY_DIR}/zix/config.h" @ONLY)

        set_target_properties (zix-static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            FOLDER "Thirdparty"
            C_VISIBILITY_PRESET hidden)

        install (TARGETS zix-static EXPORT zix-targets)
    endif()
endfunction()

#==============================================================================
# Minimal CMake wrapper for lilv (LV2 host library)
# Source: https://github.com/drobilla/lilv

function (_yup_fetch_lilv)
    if (NOT TARGET lilv-static)
        _yup_message (STATUS "Fetching lilv")

        _yup_fetch_lv2_sdk()
        _yup_fetch_zix()
        _yup_fetch_serd()
        _yup_fetch_sord()

        _yup_fetchcontent_declare (lilv
            GIT_REPOSITORY https://github.com/drobilla/lilv.git
            GIT_TAG v0.24.26)

        FetchContent_MakeAvailable (lilv)

        add_library (lilv-static STATIC)

        file (GLOB_RECURSE lilv_sources "${lilv_SOURCE_DIR}/src/*.c")
        list (FILTER lilv_sources EXCLUDE REGEX ".*/test/.*|.*_test\\.c$")
        target_sources (lilv-static PRIVATE ${lilv_sources})

        target_include_directories (lilv-static PUBLIC
            "$<BUILD_INTERFACE:${lilv_SOURCE_DIR}/include>"
            "$<BUILD_INTERFACE:${lilv_BINARY_DIR}>")

        target_link_libraries (lilv-static PUBLIC
            lv2-headers
            zix-static
            serd-static
            sord-static)

        if (YUP_PLATFORM_POSIX)
            target_link_libraries (lilv-static PUBLIC ${CMAKE_DL_LIBS})
        endif()

        target_compile_definitions (lilv-static PRIVATE
            LILV_INTERNAL
            LILV_STATIC)

        configure_file ("${lilv_SOURCE_DIR}/include/lilv/config.h.in"
                        "${lilv_BINARY_DIR}/lilv/config.h" @ONLY)

        set_target_properties (lilv-static PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            FOLDER "Thirdparty"
            C_VISIBILITY_PRESET hidden)

        install (TARGETS lilv-static EXPORT lilv-targets)
    endif()
endfunction()

#==============================================================================
# Fetch LV2 SDK + lilv for host-side support

function (_yup_fetch_lv2)
    _yup_fetch_lv2_sdk()
    _yup_fetch_lilv()
endfunction()

#==============================================================================
# Create an LV2 plugin target from a YUP audio processor

function (_yup_audio_plugin_create_lv2)
    # ==== Parse arguments — recognise all possible keywords from parent call
    _yup_plugin_shared_args (one_value_args multi_value_args)
    cmake_parse_arguments (YUP_ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_fetch_lv2_sdk()

    _yup_message (STATUS "Setting up LV2 plugin client")
    _yup_module_setup_plugin_client (
        ${target_name}
        yup_audio_plugin_client
        ${target_ide_group}
        lv2
        ${unparsed_args})

    # Extract PLUGIN_ID for LV2 URI
    if (YUP_ARG_PLUGIN_ID)
        set (lv2_uri "${YUP_ARG_PLUGIN_ID}")
    else()
        set (lv2_uri "${target_app_namespace}/${target_name}")
    endif()

    # LV2 URIs must be absolute IRIs. A bare reverse-domain ID (e.g. "org.yup.Foo")
    # is a relative IRI in Turtle — lilv would resolve it against the bundle base URI,
    # producing "file:///path/to/bundle/org.yup.Foo", which never matches the
    # lv2_descriptor URI string in the binary. Prefix with "urn:" when no scheme is
    # present to keep the manifest IRI and the C++ string identical.
    if (NOT lv2_uri MATCHES "^[a-zA-Z][a-zA-Z0-9+.-]*:")
        set (lv2_uri "urn:${lv2_uri}")
    endif()

    set (lv2_binary_suffix "${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set (lv2_binary_name "${target_name}_lv2_plugin${lv2_binary_suffix}")

    # Bundle lives in the build output dir alongside the other plugin formats
    set (lv2_bundle_name "${target_name}_lv2_plugin.lv2")
    set (lv2_bundle_dir "$<TARGET_FILE_DIR:${target_name}_lv2_plugin>/${lv2_bundle_name}")
    set (lv2_plugin_ttl_name "${target_name}_plugin.ttl")

    # is-synth flag for the generator command
    if (YUP_ARG_PLUGIN_IS_SYNTH)
        set (lv2_is_synth_arg "ON")
    else()
        set (lv2_is_synth_arg "OFF")
    endif()

    # ==== Build the TTL generator executable
    get_target_property (lv2_plugin_client_path yup_audio_plugin_client YUP_MODULE_PATH)
    get_target_property (lv2_plugin_client_includes yup_audio_plugin_client YUP_MODULE_INCLUDE_PATHS)

    add_executable (${target_name}_lv2_ttl_generator
        "${lv2_plugin_client_path}/tools/yup_lv2_ttl_generator.cpp")

    target_compile_features (${target_name}_lv2_ttl_generator PRIVATE cxx_std_20)

    target_include_directories (${target_name}_lv2_ttl_generator PRIVATE
        ${lv2_plugin_client_includes})

    if (YUP_ARG_PLUGIN_IS_SYNTH)
        set (lv2_ttl_gen_synth_define YupPlugin_IsSynth=1)
    else()
        set (lv2_ttl_gen_synth_define YupPlugin_IsSynth=0)
    endif()

    if (YUP_ARG_PLUGIN_IS_MONO)
        set (lv2_ttl_gen_mono_define YupPlugin_IsMono=1)
    else()
        set (lv2_ttl_gen_mono_define YupPlugin_IsMono=0)
    endif()

    target_compile_definitions (${target_name}_lv2_ttl_generator PRIVATE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        YUP_STANDALONE_APPLICATION=0
        YUP_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        YupPlugin_Id="${YUP_ARG_PLUGIN_ID}"
        YupPlugin_Name="${YUP_ARG_PLUGIN_NAME}"
        YupPlugin_Version="${YUP_ARG_PLUGIN_VERSION}"
        YupPlugin_Vendor="${YUP_ARG_PLUGIN_VENDOR}"
        YupPlugin_Description="${YUP_ARG_PLUGIN_DESCRIPTION}"
        YupPlugin_URL="${YUP_ARG_PLUGIN_URL}"
        YupPlugin_Email="${YUP_ARG_PLUGIN_EMAIL}"
        ${lv2_ttl_gen_synth_define}
        ${lv2_ttl_gen_mono_define})

    target_link_libraries (${target_name}_lv2_ttl_generator PRIVATE
        ${target_name}_shared
        sdl::sdl
        ${additional_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_lv2_ttl_generator
        ${target_name}_shared
        ${additional_libraries}
        ${target_modules})

    set_target_properties (${target_name}_lv2_ttl_generator PROPERTIES
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME OFF)

    # Create LV2 plugin target
    _yup_message (STATUS "Creating LV2 plugin target")

    add_library (${target_name}_lv2_plugin SHARED)

    target_compile_features (${target_name}_lv2_plugin PRIVATE cxx_std_20)

    target_compile_definitions (${target_name}_lv2_plugin PRIVATE
        YUP_AUDIO_PLUGIN_ENABLE_LV2=1
        YUP_STANDALONE_APPLICATION=0
        YupPlugin_LV2URI="${lv2_uri}")

    _yup_sdl_configure_symbols_patch ("${target_name}_lv2_plugin" lv2_sdl_symbols_patch_target lv2_sdl_symbols_sdl_target)
    set (lv2_plugin_bundle_libraries
        ${lv2_sdl_symbols_sdl_target}
        ${lv2_sdl_symbols_patch_target})

    target_link_libraries (${target_name}_lv2_plugin PRIVATE
        ${target_name}_shared
        yup_audio_plugin_client
        lv2-headers
        ${target_name}_lv2
        ${additional_libraries}
        ${lv2_plugin_bundle_libraries}
        ${target_modules})

    _yup_module_apply_arc_to_target_sources (${target_name}_lv2_plugin
        ${target_name}_shared
        yup_audio_plugin_client
        lv2-headers
        ${target_name}_lv2
        ${additional_libraries}
        ${lv2_plugin_bundle_libraries}
        ${target_modules})

    set_target_properties (${target_name}_lv2_plugin PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        PREFIX ""
        SUFFIX "${lv2_binary_suffix}"
        FOLDER "${target_ide_group}"
        XCODE_GENERATE_SCHEME ON)

    _yup_audio_plugin_apply_binary_optimizations (${target_name}_lv2_plugin
        EXPORTED_SYMBOLS lv2_descriptor)

    yup_codesign_target (${target_name}_lv2_plugin "$<TARGET_FILE:${target_name}_lv2_plugin>")

    # Ensure the generator is built before the plugin so POST_BUILD can run it
    add_dependencies (${target_name}_lv2_plugin ${target_name}_lv2_ttl_generator)

    # Assemble the .lv2 bundle in the build output directory
    add_custom_command (TARGET ${target_name}_lv2_plugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${lv2_bundle_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${lv2_bundle_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:${target_name}_lv2_plugin>"
            "${lv2_bundle_dir}/${lv2_binary_name}"
        COMMAND "$<TARGET_FILE:${target_name}_lv2_ttl_generator>"
            "--uri"          "${lv2_uri}"
            "--name"         "${YUP_ARG_PLUGIN_NAME}"
            "--vendor"       "${YUP_ARG_PLUGIN_VENDOR}"
            "--version"      "${YUP_ARG_PLUGIN_VERSION}"
            "--description"  "${YUP_ARG_PLUGIN_DESCRIPTION}"
            "--output-dir"   "${lv2_bundle_dir}"
            "--binary-name"  "${lv2_binary_name}"
            "--ttl-name"     "${lv2_plugin_ttl_name}"
            "--is-synth"     "${lv2_is_synth_arg}"
        COMMENT "Assembling LV2 bundle for ${target_name}"
        VERBATIM)

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD)
        yup_audio_plugin_copy_bundle (${target_name} lv2
            LV2_BUNDLE_DIR  "${lv2_bundle_dir}"
            LV2_BUNDLE_NAME "${lv2_bundle_name}")
    endif()

    if (YUP_ARG_PLUGIN_COPY_AFTER_BUILD AND YUP_PLATFORM_MAC)
        # The bundle is installed into a system LV2 directory that the LV2 world
        # scans. Validating the build-dir copy would be discarded by lilv as a
        # duplicate URI (the installed copy wins), so validate the installed
        # bundle directly - this is also what hosts actually load.
        yup_validate_pluginval (${target_name}_lv2_plugin
            "$ENV{HOME}/Library/Audio/Plug-Ins/LV2/${target_name}_lv2_plugin.lv2")
    else()
        yup_validate_pluginval (${target_name}_lv2_plugin "${lv2_bundle_dir}")
    endif()
endfunction()
