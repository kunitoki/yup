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
        TARGET_NAME TARGET_PRODUCT_NAME TARGET_VERSION TARGET_CONSOLE TARGET_IDE_GROUP TARGET_APP_NAMESPACE TARGET_ICON TARGET_ICON_COMPOSER TARGET_WHEEL TARGET_STATIC_LIBRARY TARGET_CXX_STANDARD
        # Emscripten
        INITIAL_MEMORY MAXIMUM_MEMORY PTHREAD_POOL_SIZE STACK_SIZE EMSCRIPTEN_DEBUG_INFO CUSTOM_PLIST CUSTOM_SHELL ENABLE_EMSCRIPTEN_WEBGPU ENABLE_EMSCRIPTEN_GL_DEBUGGING ENABLE_EMSCRIPTEN_NODERAWFS)
    set (multi_value_args
        # Globals
        DEFINITIONS COMPILE_OPTIONS MODULES SOURCES LINK_OPTIONS BUNDLE_RESOURCES
        # Emscripten
        PRELOAD_FILES EMSCRIPTEN_LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    _yup_set_default (YUP_ARG_TARGET_CXX_STANDARD 20)
    if (NOT DEFINED YUP_ARG_TARGET_ICON)
        _yup_set_default (YUP_ARG_TARGET_ICON_COMPOSER "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/resources/AppIcon.icon")
    endif()
    _yup_set_default (YUP_ARG_TARGET_ICON "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/resources/app-icon.png")
    _yup_set_default (YUP_ARG_ENABLE_EMSCRIPTEN_WEBGPU ON)
    _yup_set_default (YUP_ARG_ENABLE_EMSCRIPTEN_GL_DEBUGGING OFF)
    _yup_set_default (YUP_ARG_ENABLE_EMSCRIPTEN_NODERAWFS OFF)

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_product_name "${YUP_ARG_TARGET_PRODUCT_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (target_console "${YUP_ARG_TARGET_CONSOLE}")
    set (target_icon "${YUP_ARG_TARGET_ICON}")
    set (target_app_namespace "${YUP_ARG_TARGET_APP_NAMESPACE}")
    set (target_app_identifier "${target_app_namespace}.${target_name}")
    set (target_wheel "${YUP_ARG_TARGET_WHEEL}")
    set (target_static_library "${YUP_ARG_TARGET_STATIC_LIBRARY}")
    set (target_resources "")
    set (target_cxx_standard "${YUP_ARG_TARGET_CXX_STANDARD}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    _yup_set_default (target_console OFF)
    if ("${target_product_name}" STREQUAL "")
        set (target_product_name "${target_name}")
    endif()
    _yup_set_default (target_wheel OFF)
    _yup_set_default (target_static_library OFF)
    _yup_make_short_version ("${target_version}" target_version_short)

    # ==== Output status
    _yup_message (STATUS "${target_name} - Configuring standalone application")

    # ==== Setup Android platform, build gradle stage
    if (YUP_TARGET_ANDROID)
        _yup_message (STATUS "${target_name} - Creating java gradle project")
        _yup_android_prepare_gradle(
            BASE_PATH "${CMAKE_CURRENT_FUNCTION_LIST_DIR}"
            TARGET_NAME "${target_name}"
            TARGET_ICON "${target_icon}"
            APPLICATION_ID "${target_app_identifier}"
            APPLICATION_NAMESPACE "${target_app_namespace}"
            APPLICATION_VERSION "${target_version}")

        _yup_fetch_sdl()

        _yup_message (STATUS "${target_name} - Copying SDL java activity to application")
        _yup_android_copy_sdl_activity()

        return()
    endif()

    # ==== Check for modules circular dependencies
    _yup_module_check_circular_dependencies ("${target_name} standalone application" "${YUP_ARG_MODULES}")

    # ==== Find modules includes
    set (module_include_dirs "")
    foreach (module IN ITEMS ${YUP_ARG_MODULES})
        _yup_message (STATUS "${target_name} - Including module ${module}")
        get_target_property (module_path ${module} YUP_MODULE_PATH)
        if (module_path AND EXISTS "${module_path}")
            get_filename_component (module_path "${module_path}" DIRECTORY)
            list (APPEND module_include_dirs "${module_path}")
        endif()
    endforeach()
    list (REMOVE_DUPLICATES module_include_dirs)

    # ==== Find dependencies
    if (NOT "${target_console}")
        _yup_fetch_sdl()
        list (APPEND additional_libraries sdl::sdl)
    endif()

    # ==== Enable profiling
    if (YUP_ENABLE_PROFILING AND NOT "${target_name}" MATCHES "^yup_tests|^yup_.+_tests$")
        list (APPEND additional_definitions YUP_ENABLE_PROFILING=1)
        list (APPEND additional_libraries perfetto::perfetto)
    endif()

    if (YUP_PLATFORM_DESKTOP)
        _yup_target_list_contains ("${YUP_ARG_MODULES}" yup_audio_plugin_host has_audio_plugin_host)
        if (has_audio_plugin_host)
            _yup_collect_audio_plugin_host_dependencies ("${YUP_ARG_DEFINITIONS}" audio_plugin_host_libraries)
            list (APPEND additional_libraries ${audio_plugin_host_libraries})
        endif()
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
    elseif (target_static_library)
        add_library (${target_name} STATIC)
    else()
        add_executable (${target_name} ${executable_options})
    endif()

    # Android loads the library and Python imports the wheel by the target name, so they keep it.
    if (NOT YUP_PLATFORM_ANDROID AND NOT target_wheel AND NOT target_static_library)
        set_target_properties (${target_name} PROPERTIES OUTPUT_NAME "${target_product_name}")
    endif()

    set_target_properties (${target_name} PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        OBJC_VISIBILITY_PRESET hidden
        OBJCXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

    _yup_apply_binary_optimizations (${target_name})

    target_compile_features (${target_name} PRIVATE cxx_std_${target_cxx_standard})
    target_include_directories (${target_name} PRIVATE ${module_include_dirs})

    # ==== Per platform configuration, defined by the current platform file in platforms/
    _yup_platform_setup_standalone_app (${target_name})

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

    _yup_module_apply_arc_to_target_sources (${target_name}
        ${additional_libraries}
        ${YUP_ARG_MODULES})

    # Consumers get the module sources compiled in here, so they must not see the
    # module INTERFACE_SOURCES again: export the resolved compile settings instead.
    if (target_static_library)
        foreach (compile_property IN ITEMS COMPILE_DEFINITIONS COMPILE_OPTIONS COMPILE_FEATURES INCLUDE_DIRECTORIES)
            set_property (TARGET ${target_name} APPEND PROPERTY
                INTERFACE_${compile_property} "$<TARGET_PROPERTY:${target_name},${compile_property}>")
        endforeach()

        # Keep third party headers (e.g. googletest) as system includes, so their warnings stay hidden
        set (system_include_dirs "")
        set (visited_targets "")
        _yup_collect_system_include_directories (system_include_dirs visited_targets
            ${additional_libraries}
            ${YUP_ARG_MODULES})

        if (system_include_dirs)
            list (REMOVE_DUPLICATES system_include_dirs)
            set_property (TARGET ${target_name} APPEND PROPERTY
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${system_include_dirs})
        endif()
    endif()

    if (YUP_ARG_SOURCES AND NOT YUP_TARGET_ANDROID)
        target_sources (${target_name} PRIVATE ${YUP_ARG_SOURCES})
    endif()

    # ==== Post build steps, workaround for python*.dll
    if ("yup::yup_python" IN_LIST YUP_ARG_MODULES AND YUP_PLATFORM_WINDOWS AND NOT target_static_library AND NOT YUP_ENABLE_STATIC_PYTHON_LIBS AND Python_RUNTIME_LIBRARY_RELEASE)
        add_custom_command (TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Python_RUNTIME_LIBRARY_RELEASE}" "$<TARGET_FILE_DIR:${target_name}>"
            COMMENT "Copying Python DLL next to executable")
    endif()

endfunction()
