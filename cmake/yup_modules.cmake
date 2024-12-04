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
        if (line MATCHES "^.*BEGIN_JUCE_MODULE_DECLARATION.*")
            set (begin_decl ON)
        elseif (line MATCHES "^.*END_JUCE_MODULE_DECLARATION.*")
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

        if (NOT "${yup_platform}" MATCHES "^(win32|uwp)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_windows${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(win32)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_win32${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(uwp)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_uwp${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(ios|osx)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_apple${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(ios)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_ios${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(osx)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_osx${extension}")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_mac${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(linux)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_linux${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(ios|android)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_mobile${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(android)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_android${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(emscripten)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_emscripten${extension}")
        endif()

        if (NOT "${yup_platform}" MATCHES "^(ios|osx|android|linux|emscripten)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_posix${extension}")
        endif()

        foreach (source ${found_source_files})
            list (APPEND all_module_sources ${source})
        endforeach()
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
                                   module_cpp_standard
                                   module_include_paths
                                   module_options
                                   module_defines
                                   module_sources
                                   module_libs
                                   module_frameworks
                                   module_dependencies
                                   module_arc_enabled)
    if ("${yup_platform}" MATCHES "^(win32|uwp)$")
        list (APPEND module_defines NOMINMAX=1 WIN32_LEAN_AND_MEAN=1)
    endif()

    if ("${yup_platform}" MATCHES "^(android)$")
        list (APPEND module_include_paths
            "${ANDROID_NDK}/sources/android/cpufeatures")
        list (APPEND module_sources
            "${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c")
    endif()

    target_sources (${module_name} INTERFACE ${module_sources})

    if (module_cpp_standard)
        target_compile_features (${module_name} INTERFACE cxx_std_${module_cpp_standard})
    else()
        target_compile_features (${module_name} INTERFACE cxx_std_17)
    endif()

    set_target_properties (${module_name} PROPERTIES
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       "hidden"
        VISIBILITY_INLINES_HIDDEN   ON)

    if ("${yup_platform}" MATCHES "^(osx|ios)$")
        set_target_properties (${module_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ${module_arc_enabled})
    endif()

    target_compile_options (${module_name} INTERFACE
        ${module_options})

    target_compile_definitions (${module_name} INTERFACE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        JUCE_MODULE_AVAILABLE_${module_name}=1
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        ${module_defines})

    target_include_directories (${module_name} INTERFACE
        ${module_include_paths})

    target_link_libraries (${module_name} INTERFACE
        ${module_libs}
        ${module_frameworks})

    target_link_libraries (${module_name} INTERFACE
        ${module_dependencies})

    #add_library("yup::${module_name}" ALIAS ${module_name})

endfunction()

#==============================================================================

function (_yup_module_setup_plugin_client_clap target_name plugin_client_target folder_name)
    if (NOT "${yup_platform}" MATCHES "^(osx|linux|win32)$")
        return()
    endif()

    set (options "")
    set (one_value_args PLUGIN_ID PLUGIN_NAME PLUGIN_VENDOR PLUGIN_VERSION PLUGIN_DESCRIPTION PLUGIN_URL PLUGING_IS_SYNTH PLUGIN_IS_MONO)
    set (multi_value_args "")

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (custom_target_name "${target_name}_clap")

    add_library (${custom_target_name} INTERFACE)
    set_target_properties (${custom_target_name} PROPERTIES FOLDER "${folder_name}")

    get_target_property (module_path ${plugin_client_target} YUP_MODULE_PATH)
    get_target_property (module_cpp_standard ${plugin_client_target} YUP_MODULE_CPP_STANDARD)
    get_target_property (module_include_paths ${plugin_client_target} YUP_MODULE_INCLUDE_PATHS)
    get_target_property (module_defines ${plugin_client_target} YUP_MODULE_DEFINES)
    get_target_property (module_options ${plugin_client_target} YUP_MODULE_OPTIONS)
    get_target_property (module_libs ${plugin_client_target} YUP_MODULE_LIBS)
    get_target_property (module_frameworks ${plugin_client_target} YUP_MODULE_FRAMEWORK)
    get_target_property (module_dependencies ${plugin_client_target} YUP_MODULE_DEPENDENCIES)
    get_target_property (module_arc_enabled ${plugin_client_target} YUP_MODULE_ARC_ENABLED)

    list (APPEND module_defines YUP_AUDIO_PLUGIN_ENABLE_CLAP=1)
    list (APPEND module_defines YupPlugin_Id="${YUP_ARG_PLUGIN_ID}")
    list (APPEND module_defines YupPlugin_Name="${YUP_ARG_PLUGIN_NAME}")
    list (APPEND module_defines YupPlugin_Version="${YUP_ARG_PLUGIN_VERSION}")
    list (APPEND module_defines YupPlugin_Vendor="${YUP_ARG_PLUGIN_VENDOR}")
    list (APPEND module_defines YupPlugin_Description="${YUP_ARG_PLUGIN_DESCRIPTION}")
    list (APPEND module_defines YupPlugin_URL="${YUP_ARG_PLUGIN_URL}")
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

    if ("${yup_platform}" MATCHES "^(ios|osx)$")
        _yup_glob_recurse ("${module_path}/clap/*.mm" module_sources)
    else()
        _yup_glob_recurse ("${module_path}/clap/*.cpp" module_sources)
    endif()

    _yup_module_setup_target (${custom_target_name}
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_options}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
                              "${module_frameworks}"
                              "${module_dependencies}"
                              "${module_arc_enabled}")


    _yup_glob_recurse ("${module_path}/clap/*" all_module_files_clap)
    target_sources (${custom_target_name} PRIVATE ${all_module_files_clap})
    source_group (TREE ${module_path}/clap/ FILES ${all_module_files_clap})
    list (REMOVE_ITEM all_module_files_clap ${module_sources})
    set_source_files_properties (${all_module_files_clap} PROPERTIES HEADER_FILE_ONLY TRUE)

endfunction()

#==============================================================================

function (yup_add_module module_path)
    get_filename_component (module_path ${module_path} ABSOLUTE)
    get_filename_component (module_name ${module_path} NAME)

    _yup_message (STATUS "Processing module " ${module_name} " at " ${module_path})
    set (${module_name}_Found OFF PARENT_SCOPE)

    if (NOT EXISTS ${module_path})
        _yup_message (FATAL_ERROR "Module location not found")
    endif()

    set (module_header "${module_path}/${module_name}.h")
    if (NOT EXISTS ${module_header})
        _yup_message (FATAL_ERROR "Module header ${module_header} not found")
    endif()

    # ==== Add module as library
    add_library (${module_name} INTERFACE)
    set_target_properties (${module_name} PROPERTIES FOLDER "Modules")

    # ==== Parse module declaration string
    _yup_module_parse_config ("${module_header}" module_configs module_user_configs)

    # ==== Assign configs to variables from module declaration string
    # Globals --------------------------------------------------------------
    set (module_cpp_standard "")
    set (module_dependencies "")
    set (module_include_paths "")
    set (module_options "")
    set (module_defines "")
    set (module_searchpaths "")
    set (module_searchpaths_private "")
    set (module_arc_enabled OFF)
    # macOS -----------------------------------------------------------------
    set (module_osx_dependencies "")
    set (module_osx_frameworks "")
    set (module_osx_weak_frameworks "")
    set (module_osx_libs "")
    set (module_osx_defines "")
    # iOS -------------------------------------------------------------------
    set (module_ios_dependencies "")
    set (module_ios_frameworks "")
    set (module_ios_weak_frameworks "")
    set (module_ios_libs "")
    set (module_ios_defines "")
    # Linux -----------------------------------------------------------------
    set (module_linux_dependencies "")
    set (module_linux_libs "")
    set (module_linux_packages "")
    set (module_linux_defines "")
    # Windows ---------------------------------------------------------------
    set (module_windows_dependencies "")
    set (module_windows_libs "")
    set (module_windows_defines "")
    set (module_windows_options "")
    set (module_mingw_libs "")
    # Wasm ------------------------------------------------------------------
    set (module_wasm_dependencies "")
    set (module_wasm_libs "")
    set (module_wasm_defines "")
    # Android ---------------------------------------------------------------
    set (module_android_dependencies "")
    set (module_android_libs "")
    set (module_android_defines "")

    set (parsed_dependencies "")
    foreach (module_config ${module_configs})
        string (REGEX REPLACE "^(.+):([ \t\r\n]+.*)$" "\\1" module_config_key ${module_config})
        string (REGEX REPLACE "^.+:[ \t\r\n]+(.+)$" "\\1" module_config_value ${module_config})

        if (${module_config_key} STREQUAL "minimumCppStandard")
            set (module_cpp_standard "${module_config_value}")
        elseif (${module_config_key} STREQUAL "dependencies")
            _yup_comma_or_space_separated_list (${module_config_value} module_dependencies)
        elseif (${module_config_key} STREQUAL "defines")
            _yup_comma_or_space_separated_list (${module_config_value} module_defines)
        elseif (${module_config_key} STREQUAL "searchpaths")
            _yup_comma_or_space_separated_list (${module_config_value} module_searchpaths)

        elseif (${module_config_key} STREQUAL "osxDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_dependencies)
        elseif (${module_config_key} STREQUAL "osxFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_frameworks)
        elseif (${module_config_key} STREQUAL "osxWeakFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_weak_frameworks)
        elseif (${module_config_key} STREQUAL "osxLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_libs)
        elseif (${module_config_key} STREQUAL "osxDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_defines)

        elseif (${module_config_key} STREQUAL "iosDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_dependencies)
        elseif (${module_config_key} STREQUAL "iosFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_frameworks)
        elseif (${module_config_key} STREQUAL "iosWeakFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_weak_frameworks)
        elseif (${module_config_key} STREQUAL "iosLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_libs)
        elseif (${module_config_key} STREQUAL "iosDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_defines)

        elseif (${module_config_key} STREQUAL "linuxDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_dependencies)
        elseif (${module_config_key} STREQUAL "linuxLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_libs)
        elseif (${module_config_key} STREQUAL "linuxPackages")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_packages)
        elseif (${module_config_key} STREQUAL "linuxDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_defines)

        elseif (${module_config_key} STREQUAL "windowsDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_windows_dependencies)
        elseif (${module_config_key} STREQUAL "windowsLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_windows_libs)
        elseif (${module_config_key} STREQUAL "windowsDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_windows_defines)
        elseif (${module_config_key} STREQUAL "windowsOptions")
            _yup_comma_or_space_separated_list (${module_config_value} module_windows_options)
        elseif (${module_config_key} STREQUAL "mingwLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_mingw_libs)

        elseif (${module_config_key} STREQUAL "wasmDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_wasm_dependencies)
        elseif (${module_config_key} STREQUAL "wasmLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_wasm_libs)
        elseif (${module_config_key} STREQUAL "wasmDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_wasm_defines)

        elseif (${module_config_key} STREQUAL "androidDeps")
            _yup_comma_or_space_separated_list (${module_config_value} module_android_dependencies)
        elseif (${module_config_key} STREQUAL "androidLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_android_libs)
        elseif (${module_config_key} STREQUAL "androidDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_android_defines)

        elseif (${module_config_key} STREQUAL "enableARC")
            _yup_boolean_property (${module_config_value} module_arc_enabled)

        endif()
    endforeach()

    # ==== Scan sources to include
    _yup_module_collect_sources ("${module_path}" module_sources)

    # ==== Setup libs and frameworks
    set (module_frameworks "")
    set (module_libs "")
    if ("${yup_platform}" MATCHES "^(ios)$")
        list (APPEND module_dependencies ${module_ios_dependencies})
        list (APPEND module_libs ${module_ios_libs})
        list (APPEND module_defines ${module_ios_defines})
        _yup_module_prepare_frameworks ("${module_ios_frameworks}" "${module_ios_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(osx)$")
        list (APPEND module_dependencies ${module_osx_dependencies})
        list (APPEND module_libs ${module_osx_libs})
        list (APPEND module_defines ${module_osx_defines})
        _yup_module_prepare_frameworks ("${module_osx_frameworks}" "${module_osx_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(linux)$")
        list (APPEND module_dependencies ${module_linux_dependencies})
        list (APPEND module_libs ${module_linux_libs})
        list (APPEND module_defines ${module_linux_defines})
        foreach (package ${module_linux_packages})
            _yup_get_package_config_libs ("${package}" package_libs)
            list (APPEND module_libs ${package_libs})
        endforeach()
    elseif ("${yup_platform}" MATCHES "^(emscripten)$")
        list (APPEND module_dependencies ${module_wasm_dependencies})
        list (APPEND module_libs ${module_wasm_libs})
        list (APPEND module_defines ${module_wasm_defines})
    elseif ("${yup_platform}" MATCHES "^(android)$")
        list (APPEND module_dependencies ${module_android_dependencies})
        list (APPEND module_libs ${module_android_libs})
        list (APPEND module_defines ${module_android_defines})
        list (APPEND module_include_paths "${ANDROID_NDK}/sources/android/native_app_glue")
    elseif ("${yup_platform}" MATCHES "^(win32|uwp)$")
        list (APPEND module_dependencies ${module_windows_dependencies})
        list (APPEND module_defines ${module_windows_defines})
        list (APPEND module_options ${module_windows_options})
        if (MINGW)
            list (APPEND module_libs ${module_mingw_libs})
        else()
            list (APPEND module_libs ${module_windows_libs})
        endif()
    endif()

    #if (("${module_name}" STREQUAL "juce_audio_devices") AND ("${yup_platform}" MATCHES "^(android)$"))
    #    add_subdirectory("${module_path}/native/oboe")
    #    list (APPEND module_libs oboe)
    #endif()

    # ==== Prepare include paths
    get_filename_component (module_include_path ${module_path} DIRECTORY)
    list (APPEND module_include_paths "${module_include_path}")

    foreach (searchpath ${module_searchpaths})
        if (EXISTS "${module_path}/${searchpath}")
            list (APPEND module_include_paths "${module_path}/${searchpath}")
        endif()
    endforeach()

    # ==== Setup module sources and properties
    _yup_module_setup_target (${module_name}
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_options}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
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
        YUP_MODULE_FRAMEWORK "${module_frameworks}"
        YUP_MODULE_DEPENDENCIES "${module_dependencies}"
        YUP_MODULE_ARC_ENABLED "${module_arc_enabled}")

endfunction()

#==============================================================================

function (_yup_add_default_modules modules_path)
    yup_add_module (${modules_path}/thirdparty/zlib)
    yup_add_module (${modules_path}/thirdparty/glad)
    yup_add_module (${modules_path}/thirdparty/harfbuzz)
    yup_add_module (${modules_path}/thirdparty/sheenbidi)
    yup_add_module (${modules_path}/thirdparty/yoga_library)
    yup_add_module (${modules_path}/thirdparty/rive)
    yup_add_module (${modules_path}/thirdparty/rive_renderer)
    yup_add_module (${modules_path}/thirdparty/oboe)

    # Original juce modules
    yup_add_module (${modules_path}/modules/juce_core)
    yup_add_module (${modules_path}/modules/juce_events)
    yup_add_module (${modules_path}/modules/juce_audio_basics)
    yup_add_module (${modules_path}/modules/juce_audio_devices)

    # New yup modules
    yup_add_module (${modules_path}/modules/yup_audio_processors)
    yup_add_module (${modules_path}/modules/yup_audio_plugin_client)
    yup_add_module (${modules_path}/modules/yup_graphics)
    yup_add_module (${modules_path}/modules/yup_gui)
endfunction()
