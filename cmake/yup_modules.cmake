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

function (_yup_module_parse_config module_header output_module_configs output_module_user_configs)
    set (module_configs "")
    set (module_user_configs "")

    set (begin_decl OFF)
    set (end_decl OFF)
    file (STRINGS ${module_header} module_header_lines)
    while (module_header_lines)
        list (POP_FRONT module_header_lines line)
        if (line MATCHES "^.*BEGIN_YUP_MODULE_DECLARATION.*")
            set (begin_decl ON)
        elseif (line MATCHES "^.*END_YUP_MODULE_DECLARATION.*")
            if (NOT begin_decl)
                _yup_message (FATAL_ERROR "Invalid module declaration")
            endif()
            set (begin_decl OFF)
        elseif (begin_decl)
            string (STRIP "${line}" stripped_line)
            if ("${stripped_line}" STREQUAL "")
                continue()
            endif()
            list (APPEND module_configs ${stripped_line})
        elseif (line MATCHES "^.*Config:.*")
            string (STRIP "${line}" stripped_line)
            string (REGEX REPLACE "^.*Config:(.*)$" "\\1" user_config_name ${stripped_line})
            string (STRIP "${user_config_name}" stripped_user_config_name)
            if ("${stripped_user_config_name}" STREQUAL "")
                continue()
            endif()
            list (APPEND module_user_configs ${stripped_user_config_name})
        endif()
    endwhile()

    set (${output_module_configs} "${module_configs}" PARENT_SCOPE)
    set (${output_module_user_configs} "${module_user_configs}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_module_collect_sources folder output_variable)
    set(source_extensions ".c;.cc;.cxx;.cpp;.h;.hh;.hxx;.hpp")
    if (APPLE)
        list (APPEND source_extensions ".m" ".mm")
    endif()

    set (base_path "${folder}/${module_name}")
    set (all_module_sources "")

    foreach (extension ${source_extensions})
        file (GLOB found_source_files "${base_path}*${extension}")

        if (NOT YUP_PLATFORM_MSFT)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_microsoft${extension}")
        endif()

        if (NOT YUP_PLATFORM_WINDOWS)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_windows${extension}")
        endif()

        if (NOT YUP_PLATFORM_UWP)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_uwp${extension}")
        endif()

        if (NOT YUP_PLATFORM_APPLE)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_apple${extension}")
        endif()

        if (NOT YUP_PLATFORM_IOS)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_ios${extension}")
        endif()

        if (NOT YUP_PLATFORM_OSX)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_osx${extension}")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_mac${extension}")
        endif()

        if (NOT YUP_PLATFORM_LINUX)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_linux${extension}")
        endif()

        if (NOT YUP_PLATFORM_MOBILE)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_mobile${extension}")
        endif()

        if (NOT YUP_PLATFORM_ANDROID)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_android${extension}")
        endif()

        if (NOT YUP_PLATFORM_EMSCRIPTEN)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_emscripten${extension}")
        endif()

        if (NOT YUP_PLATFORM_POSIX)
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_posix${extension}")
        endif()

        list (APPEND all_module_sources ${found_source_files})
    endforeach()

    set (module_sources "")
    foreach (module_source ${all_module_sources})
        if (APPLE)
            if (module_source MATCHES "^.*\.(cc|cxx|cpp)$")
                get_filename_component (source_directory ${module_source} DIRECTORY)
                get_filename_component (source_file ${module_source} NAME_WLE)
                set (imported_module_source "${source_directory}/${source_file}.mm")
                if (${imported_module_source} IN_LIST all_module_sources)
                    continue()
                endif()
            elseif (module_source MATCHES "^.*\.c$")
                get_filename_component (source_directory ${module_source} DIRECTORY)
                get_filename_component (source_file ${module_source} NAME_WLE)
                set (imported_module_source "${source_directory}/${source_file}.m")
                if (${imported_module_source} IN_LIST all_module_sources)
                    continue()
                endif()
            endif()
        endif()
        list (APPEND module_sources ${module_source})
    endforeach()

    set (${output_variable} "${module_sources}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_module_prepare_frameworks frameworks weak_frameworks output_variable)
    set (temp_frameworks "")
    foreach (framework ${frameworks})
        list (APPEND temp_frameworks "-framework ${framework}")
    endforeach()

    foreach (framework ${weak_frameworks})
        list (APPEND temp_frameworks "-weak_framework ${framework}")
    endforeach()

    list (JOIN temp_frameworks " " final_frameworks)
    set (${output_variable} "${final_frameworks}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_module_setup_target module_name
                                   module_path
                                   module_cpp_standard
                                   module_include_paths
                                   module_options
                                   module_defines
                                   module_sources
                                   module_libs
                                   module_libs_paths
                                   module_link_options
                                   module_frameworks
                                   module_dependencies
                                   module_arc_enabled)
    if (YUP_PLATFORM_MSFT)
        list (APPEND module_defines NOMINMAX=1 WIN32_LEAN_AND_MEAN=1)
        list (APPEND module_options /bigobj)
    endif()

    target_sources (${module_name} INTERFACE ${module_sources})

    if (module_cpp_standard)
        target_compile_features (${module_name} INTERFACE cxx_std_${module_cpp_standard})
    else()
        target_compile_features (${module_name} INTERFACE cxx_std_17)
    endif()

    set_target_properties (${module_name} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

    if (YUP_PLATFORM_OSX OR YUP_PLATFORM_IOS)
        set_target_properties (${module_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ${module_arc_enabled}
            XCODE_GENERATE_SCHEME OFF)
    endif()

    target_compile_options (${module_name} INTERFACE
        ${module_options})

    target_compile_definitions (${module_name} INTERFACE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        YUP_MODULE_AVAILABLE_${module_name}=1
        YUP_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        ${module_defines})

    target_include_directories (${module_name} INTERFACE
        ${module_include_paths})

    target_link_directories (${module_name} INTERFACE
        ${module_libs_paths})

    target_link_libraries (${module_name} INTERFACE
        ${module_libs}
        ${module_frameworks}
        ${module_dependencies})

    target_link_options (${module_name} INTERFACE
        ${module_link_options})

    # Add coverage support if enabled
    if (YUP_ENABLE_COVERAGE)
        _yup_setup_coverage_flags (${module_name})
    endif()
endfunction()

#==============================================================================

function (_yup_module_setup_plugin_client target_name plugin_client_target folder_name plugin_type)
    if (NOT YUP_PLATFORM_DESKTOP)
        return()
    endif()

    set (options "")
    set (one_value_args PLUGIN_ID PLUGIN_NAME PLUGIN_VENDOR PLUGIN_VERSION PLUGIN_DESCRIPTION PLUGIN_URL PLUGIN_EMAIL PLUGIN_IS_SYNTH PLUGIN_IS_MONO)
    set (multi_value_args "")

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (plugin_type STREQUAL "vst3")
        set (custom_target_name "${target_name}_vst3")
        set (plugin_define "YUP_AUDIO_PLUGIN_ENABLE_VST3=1")
    elseif (plugin_type STREQUAL "clap")
        set (custom_target_name "${target_name}_clap")
        set (plugin_define "YUP_AUDIO_PLUGIN_ENABLE_CLAP=1")
    elseif (plugin_type STREQUAL "standalone")
        set (custom_target_name "${target_name}_standalone")
        set (plugin_define "YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1")
    else()
        _yup_message (FATAL_ERROR "Invalid plugin type: ${plugin_type}. Must be either 'vst3', 'clap' or 'standalone'")
    endif()

    add_library (${custom_target_name} INTERFACE)
    set_target_properties (${custom_target_name} PROPERTIES FOLDER "${folder_name}")

    get_target_property (module_path ${plugin_client_target} YUP_MODULE_PATH)
    get_target_property (module_cpp_standard ${plugin_client_target} YUP_MODULE_CPP_STANDARD)
    get_target_property (module_include_paths ${plugin_client_target} YUP_MODULE_INCLUDE_PATHS)
    get_target_property (module_defines ${plugin_client_target} YUP_MODULE_DEFINES)
    get_target_property (module_options ${plugin_client_target} YUP_MODULE_OPTIONS)
    get_target_property (module_libs ${plugin_client_target} YUP_MODULE_LIBS)
    get_target_property (module_libs_paths ${plugin_client_target} YUP_MODULE_LIBS_PATHS)
    get_target_property (module_link_options ${plugin_client_target} YUP_MODULE_LINK_OPTIONS)
    get_target_property (module_frameworks ${plugin_client_target} YUP_MODULE_FRAMEWORK)
    get_target_property (module_dependencies ${plugin_client_target} YUP_MODULE_DEPENDENCIES)
    get_target_property (module_arc_enabled ${plugin_client_target} YUP_MODULE_ARC_ENABLED)

    list (APPEND module_defines ${plugin_define})
    list (APPEND module_defines YupPlugin_Id="${YUP_ARG_PLUGIN_ID}")
    list (APPEND module_defines YupPlugin_Name="${YUP_ARG_PLUGIN_NAME}")
    list (APPEND module_defines YupPlugin_Version="${YUP_ARG_PLUGIN_VERSION}")
    list (APPEND module_defines YupPlugin_Vendor="${YUP_ARG_PLUGIN_VENDOR}")
    list (APPEND module_defines YupPlugin_Description="${YUP_ARG_PLUGIN_DESCRIPTION}")
    list (APPEND module_defines YupPlugin_URL="${YUP_ARG_PLUGIN_URL}")
    list (APPEND module_defines YupPlugin_Email="${YUP_ARG_PLUGIN_EMAIL}")

    if (YUP_ARG_PLUGIN_IS_SYNTH)
        list (APPEND module_defines YupPlugin_IsSynth=1)
    else()
        list (APPEND module_defines YupPlugin_IsSynth=0)
    endif()

    if (YUP_ARG_PLUGIN_IS_MONO)
        list (APPEND module_defines YupPlugin_IsMono=1)
    else()
        list (APPEND module_defines YupPlugin_IsMono=0)
    endif()

    if (YUP_PLATFORM_APPLE)
        _yup_glob_recurse ("${module_path}/${plugin_type}/*.mm" module_sources)
    else()
        _yup_glob_recurse ("${module_path}/${plugin_type}/*.cpp" module_sources)
    endif()

    _yup_module_setup_target ("${custom_target_name}"
                              "${module_path}"
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_options}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
                              "${module_libs_paths}"
                              "${module_link_options}"
                              "${module_frameworks}"
                              "${module_dependencies}"
                              "${module_arc_enabled}")

    _yup_glob_recurse ("${module_path}/${plugin_type}/*" all_module_files)
    target_sources (${custom_target_name} PRIVATE ${all_module_files})
    source_group (TREE ${module_path}/${plugin_type}/ FILES ${all_module_files})
    list (REMOVE_ITEM all_module_files ${module_sources})
    set_source_files_properties (${all_module_files} PROPERTIES HEADER_FILE_ONLY TRUE)
endfunction()

#==============================================================================

function (yup_add_module module_path modules_definitions module_group)
    get_filename_component (module_path ${module_path} ABSOLUTE)
    get_filename_component (module_name ${module_path} NAME)

    _yup_message (STATUS "Processing module " ${module_name} " at " ${module_path})
    set (${module_name}_Found OFF PARENT_SCOPE)

    if (NOT EXISTS ${module_path})
        _yup_message (FATAL_ERROR "Module location ${module_path} not found")
    endif()

    set (module_header "${module_path}/${module_name}.h")
    if (NOT EXISTS ${module_header})
        _yup_message (FATAL_ERROR "Module header ${module_header} in module ${module_path} not found")
    endif()

    # ==== Add module as library
    add_library (${module_name} INTERFACE)
    set_target_properties (${module_name} PROPERTIES FOLDER "${module_group}")

    # ==== Parse module declaration string
    _yup_module_parse_config ("${module_header}" module_configs module_user_configs)

    # ==== Assign Configurations Dynamically
    set (global_properties "dependencies|defines|libs|options|searchpaths")
    set (platform_properties "^(.*)Deps$|^(.*)Defines$|^(.*)Libs$|^(.*)Frameworks$|^(.*)WeakFrameworks$|^(.*)Options$|^(.*)LinkOptions$|^(.*)Packages$|^(.*)Searchpaths$|^(.*)CppStandard$")

    set (parsed_config "")
    foreach (module_config ${module_configs})
        string (REGEX REPLACE "^(.+):[ \t\r\n]+(.+)$" "\\1;\\2" parsed_config ${module_config})
        list (GET parsed_config 0 key)
        list (LENGTH parsed_config parsed_config_len)
        if (parsed_config_len GREATER 1)
            list (GET parsed_config 1 value)
        else()
            set (value "")
        endif()

        if (${key} MATCHES "${global_properties}|${platform_properties}")
            _yup_comma_or_space_separated_list ("${value}" module_${key})
        elseif (${key} MATCHES "^minimumCppStandard$")
            set (module_cpp_standard "${value}")
        elseif (${key} MATCHES "^enableARC$")
            _yup_boolean_property ("${value}" module_arc_enabled)
        elseif (${key} MATCHES "^needsPython$")
            _yup_boolean_property ("${value}" module_needs_python)
        endif()
    endforeach()

    _yup_set_default (module_cpp_standard "17")
    _yup_set_default (module_arc_enabled OFF)
    _yup_set_default (module_needs_python OFF)
    _yup_resolve_variable_paths ("${module_searchpaths}" module_searchpaths)

    # ==== Setup Platform-Specific Configurations
    if (YUP_PLATFORM_IOS)
        if (module_appleCppStandard)
            set (module_cpp_standard "${module_appleCppStandard}")
        endif()

        if (PLATFORM MATCHES "^(SIMULATOR.*)$")
            list (APPEND module_dependencies ${module_iosSimDeps})
            list (APPEND module_defines ${module_iosSimDefines})
            list (APPEND module_options ${module_iosSimOptions})
            list (APPEND module_libs ${module_iosSimLibs})
            list (APPEND module_link_options ${module_iosSimLinkOptions})
            _yup_resolve_variable_paths ("${module_iosSimSearchpaths}" module_iosSimSearchpaths)
            list (APPEND module_searchpaths ${module_iosSimSearchpaths})
            _yup_module_prepare_frameworks ("${module_iosSimFrameworks}" "${module_iosSimWeakFrameworks}" module_iosSimframeworks)
            list (APPEND module_frameworks ${module_iosSimframeworks})
            if (module_iosSimCppStandard)
                set (module_cpp_standard "${module_iosSimCppStandard}")
            endif()
        else()
            list (APPEND module_dependencies ${module_iosDeps})
            list (APPEND module_defines ${module_iosDefines})
            list (APPEND module_options ${module_iosOptions})
            list (APPEND module_libs ${module_iosLibs})
            list (APPEND module_link_options ${module_iosLinkOptions})
            _yup_resolve_variable_paths ("${module_iosSearchpaths}" module_iosSearchpaths)
            list (APPEND module_searchpaths ${module_iosSearchpaths})
            _yup_module_prepare_frameworks ("${module_iosFrameworks}" "${module_iosWeakFrameworks}" module_iosFrameworks)
            list (APPEND module_frameworks ${module_iosFrameworks})
            if (module_iosCppStandard)
                set (module_cpp_standard "${module_iosCppStandard}")
            endif()
        endif()

        list (APPEND module_dependencies ${module_appleDeps})
        list (APPEND module_defines ${module_appleDefines})
        list (APPEND module_options ${module_appleOptions})
        list (APPEND module_libs ${module_appleLibs})
        list (APPEND module_link_options ${module_appleLinkOptions})
        _yup_resolve_variable_paths ("${module_appleSearchpaths}" module_appleSearchpaths)
        list (APPEND module_searchpaths ${module_appleSearchpaths})
        _yup_module_prepare_frameworks ("${module_appleFrameworks}" "${module_appleWeakFrameworks}" module_appleFrameworks)
        list (APPEND module_frameworks ${module_appleFrameworks})

    elseif (YUP_PLATFORM_OSX)
        if (module_appleCppStandard)
            set (module_cpp_standard "${module_appleCppStandard}")
        endif()
        list (APPEND module_dependencies ${module_osxDeps})
        list (APPEND module_dependencies ${module_appleDeps})
        list (APPEND module_defines ${module_osxDefines})
        list (APPEND module_defines ${module_appleDefines})
        list (APPEND module_options ${module_osxOptions})
        list (APPEND module_options ${module_appleOptions})
        list (APPEND module_libs ${module_osxLibs})
        list (APPEND module_link_options ${module_osxLinkOptions})
        list (APPEND module_libs ${module_appleLibs})
        list (APPEND module_link_options ${module_appleLinkOptions})
        _yup_resolve_variable_paths ("${module_osxSearchpaths}" module_osxSearchpaths)
        list (APPEND module_searchpaths ${module_osxSearchpaths})
        _yup_resolve_variable_paths ("${module_appleSearchpaths}" module_appleSearchpaths)
        list (APPEND module_searchpaths ${module_appleSearchpaths})
        _yup_module_prepare_frameworks ("${module_osxFrameworks}" "${module_osxWeakFrameworks}" module_osxFrameworks)
        list (APPEND module_frameworks ${module_osxFrameworks})
        _yup_module_prepare_frameworks ("${module_appleFrameworks}" "${module_appleWeakFrameworks}" module_appleFrameworks)
        list (APPEND module_frameworks ${module_appleFrameworks})
        if (module_osxCppStandard)
            set (module_cpp_standard "${module_osxCppStandard}")
        endif()

    elseif (YUP_PLATFORM_LINUX)
        if (module_linuxCppStandard)
            set (module_cpp_standard "${module_linuxCppStandard}")
        endif()
        list (APPEND module_dependencies ${module_linuxDeps})
        list (APPEND module_defines ${module_linuxDefines})
        list (APPEND module_options ${module_linuxOptions})
        list (APPEND module_libs ${module_linuxLibs})
        list (APPEND module_link_options ${module_linuxLinkOptions})
        _yup_resolve_variable_paths ("${module_linuxSearchpaths}" module_linuxSearchpaths)
        list (APPEND module_searchpaths ${module_linuxSearchpaths})
        foreach (package ${module_linuxPackages})
            _yup_get_package_config_libs ("${package}" package_libs)
            list (APPEND module_libs ${package_libs})
        endforeach()

    elseif (YUP_PLATFORM_EMSCRIPTEN)
        if (module_wasmCppStandard)
            set (module_cpp_standard "${module_wasmCppStandard}")
        endif()
        list (APPEND module_dependencies ${module_wasmDeps})
        list (APPEND module_defines ${module_wasmDefines})
        list (APPEND module_options ${module_wasmOptions})
        list (APPEND module_libs ${module_wasmLibs})
        list (APPEND module_link_options ${module_wasmLinkOptions})
        _yup_resolve_variable_paths ("${module_wasmSearchpaths}" module_wasmSearchpaths)
        list (APPEND module_searchpaths ${module_wasmSearchpaths})

    elseif (YUP_PLATFORM_ANDROID)
        if (module_androidCppStandard)
            set (module_cpp_standard "${module_androidCppStandard}")
        endif()
        list (APPEND module_dependencies ${module_androidDeps})
        list (APPEND module_defines ${module_androidDefines})
        list (APPEND module_options ${module_androidOptions})
        list (APPEND module_libs ${module_androidLibs})
        list (APPEND module_link_options ${module_androidLinkOptions})
        _yup_resolve_variable_paths ("${module_androidSearchpaths}" module_androidSearchpaths)
        list (APPEND module_searchpaths ${module_androidSearchpaths})

    elseif (YUP_PLATFORM_MSFT)
        if (module_msftCppStandard)
            set (module_cpp_standard "${module_msftCppStandard}")
        endif()
        list (APPEND module_libs ${module_msftLibs})
        list (APPEND module_libs ${module_windowsLibs})
        list (APPEND module_dependencies ${module_msftDeps})
        list (APPEND module_dependencies ${module_windowsDeps})
        list (APPEND module_defines ${module_msftDefines})
        list (APPEND module_defines ${module_windowsDefines})
        list (APPEND module_options ${module_msftOptions})
        list (APPEND module_options ${module_windowsOptions})
        list (APPEND module_link_options ${module_msftLinkOptions})
        list (APPEND module_link_options ${module_windowsLinkOptions})
        _yup_resolve_variable_paths ("${module_msftSearchpaths}" module_msftSearchpaths)
        list (APPEND module_searchpaths ${module_msftSearchpaths})
        _yup_resolve_variable_paths ("${module_windowsSearchpaths}" module_windowsSearchpaths)
        list (APPEND module_searchpaths ${module_windowsSearchpaths})
        if (module_windowsCppStandard)
            set (module_cpp_standard "${module_windowsCppStandard}")
        endif()
    endif()

    # ==== Add module definitions
    foreach (module_definition ${modules_definitions})
        list (APPEND module_defines ${module_definition})
    endforeach()

    # ==== Prepare include paths
    get_filename_component (module_include_path ${module_path} DIRECTORY)
    list (APPEND module_include_paths "${module_include_path}")

    foreach (searchpath ${module_searchpaths})
        if (EXISTS "${searchpath}")
            list (APPEND module_include_paths "${searchpath}")
        elseif (EXISTS "${module_path}/${searchpath}")
            list (APPEND module_include_paths "${module_path}/${searchpath}")
        endif()
    endforeach()

    # ==== Fetch Python if needed
    if (module_needs_python)
        _yup_fetch_python ("${Python_INCLUDE_DIRS}" "${Python_ROOT_DIR}")
        list (APPEND module_libs Python::Python)
        list (APPEND module_include_paths "${Python_INCLUDE_DIRS}")
        if (NOT "${Python_LIBRARY_DIRS}" STREQUAL "")
            list (APPEND module_libs_paths "${Python_LIBRARY_DIRS}")
        endif()
    endif()

    # ==== Scan sources to include
    _yup_module_collect_sources ("${module_path}" module_sources)

    # ==== Setup module sources and properties
    _yup_module_setup_target ("${module_name}"
                              "${module_path}"
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_options}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
                              "${module_libs_paths}"
                              "${module_link_options}"
                              "${module_frameworks}"
                              "${module_dependencies}"
                              "${module_arc_enabled}")

    #set (${module_name}_Configs "${module_user_configs}")
    #set (${module_name}_Configs ${${module_name}_Configs} PARENT_SCOPE)

    _yup_glob_recurse ("${module_path}/*" all_module_files)
    target_sources (${module_name} PRIVATE ${all_module_files})
    source_group (TREE ${module_path}/ FILES ${all_module_files})
    list (REMOVE_ITEM all_module_files ${module_sources})
    set_source_files_properties (${all_module_files} PROPERTIES HEADER_FILE_ONLY TRUE)

    # ==== Setup parent scope variables
    set (${module_name}_Found ON PARENT_SCOPE)
    set_target_properties (${module_name} PROPERTIES
        YUP_MODULE_PATH "${module_path}"
        YUP_MODULE_HEADER "${module_header}"
        YUP_MODULE_CPP_STANDARD "${module_cpp_standard}"
        YUP_MODULE_INCLUDE_PATHS "${module_include_paths}"
        YUP_MODULE_OPTIONS "${module_options}"
        YUP_MODULE_DEFINES "${module_defines}"
        YUP_MODULE_SOURCES "${module_sources}"
        YUP_MODULE_LIBS "${module_libs}"
        YUP_MODULE_LIBS_PATHS "${module_libs_paths}"
        YUP_MODULE_LINK_OPTIONS "${module_link_options}"
        YUP_MODULE_FRAMEWORK "${module_frameworks}"
        YUP_MODULE_DEPENDENCIES "${module_dependencies}"
        YUP_MODULE_ARC_ENABLED "${module_arc_enabled}")

    # ==== Add Java support for Android if available (after target properties are set)
    if (YUP_PLATFORM_ANDROID AND YUP_BUILD_JAVA_SUPPORT)
        _yup_module_add_java_support (${module_name})
    endif()

endfunction()

#==============================================================================

function (yup_add_default_modules modules_path)
    get_filename_component (modules_path "${modules_path}" ABSOLUTE)
    _yup_message (STATUS "Adding default modules from ${modules_path}")

    # ==== Fetch options
    set (options "")
    set (one_value_args "")
    set (multi_value_args DEFINITIONS)
    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    _yup_set_default (YUP_ARG_TARGET_DEFINITIONS "")
    set (modules_definitions "${YUP_ARG_DEFINITIONS}")

    # ==== Thirdparty modules
    set (thirdparty_group "Thirdparty")
    yup_add_module (${modules_path}/thirdparty/zlib "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/glad "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/harfbuzz "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/libpng "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/libwebp "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/sheenbidi "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/yoga_library "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/rive "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/rive_decoders "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/rive_renderer "${modules_definitions}" ${thirdparty_group})
    yup_add_module (${modules_path}/thirdparty/oboe_library "${modules_definitions}" ${thirdparty_group})

    # ==== Yup modules
    set (modules_group "Modules")
    yup_add_module (${modules_path}/modules/yup_core "${modules_definitions}" ${modules_group})
    add_library (yup::yup_core ALIAS yup_core)

    yup_add_module (${modules_path}/modules/yup_events "${modules_definitions}" ${modules_group})
    add_library (yup::yup_events ALIAS yup_events)

    yup_add_module (${modules_path}/modules/yup_data_model "${modules_definitions}" ${modules_group})
    add_library (yup::yup_data_model ALIAS yup_data_model)

    yup_add_module (${modules_path}/modules/yup_audio_basics "${modules_definitions}" ${modules_group})
    add_library (yup::yup_audio_basics ALIAS yup_audio_basics)

    yup_add_module (${modules_path}/modules/yup_audio_devices "${modules_definitions}" ${modules_group})
    add_library (yup::yup_audio_devices ALIAS yup_audio_devices)

    yup_add_module (${modules_path}/modules/yup_audio_processors "${modules_definitions}" ${modules_group})
    add_library (yup::yup_audio_processors ALIAS yup_audio_processors)

    yup_add_module (${modules_path}/modules/yup_audio_plugin_client "${modules_definitions}" ${modules_group})
    add_library (yup::yup_audio_plugin_client ALIAS yup_audio_plugin_client)

    yup_add_module (${modules_path}/modules/yup_graphics "${modules_definitions}" ${modules_group})
    add_library (yup::yup_graphics ALIAS yup_graphics)

    yup_add_module (${modules_path}/modules/yup_gui "${modules_definitions}" ${modules_group})
    add_library (yup::yup_gui ALIAS yup_gui)

    if (YUP_PLATFORM_MAC OR YUP_PLATFORM_WINDOWS OR YUP_PLATFORM_LINUX)
        yup_add_module (${modules_path}/modules/yup_python "${modules_definitions}" ${modules_group})
        add_library (yup::yup_python ALIAS yup_python)
    endif()
endfunction()
