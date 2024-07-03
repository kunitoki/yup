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

#list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools")

#==============================================================================

function (_yup_strip_list input_list output_variable)
    set (inner_list "" PARENT_SCOPE)
    foreach (item ${input_list})
        string (STRIP ${item} stripped_item)
        if (${stripped_item} STREQUAL "")
            continue()
        endif()
        list (APPEND inner_list ${stripped_item})
    endforeach()
    set (${output_variable} "${inner_list}" PARENT_SCOPE)
endfunction()

function (_yup_make_short_version version output_variable)
    string (REPLACE "." ";" version_list ${version})
    list (LENGTH version_list version_list_length)
    math (EXPR version_list_last_index "${version_list_length} - 1")
    list (REMOVE_AT version_list ${version_list_last_index})
    string (JOIN "." version_short ${version_list})
    set (${output_variable} "${version_short}" PARENT_SCOPE)
endfunction()

function (_yup_comma_or_space_separated_list input_list output_variable)
    string (REPLACE "," " " temp1_list ${input_list})
    string (REPLACE " " ";" temp2_list ${temp1_list})
    _yup_strip_list ("${temp2_list}" final_list)
    set (${output_variable} "${final_list}" PARENT_SCOPE)
endfunction()

function (_yup_boolean_property input_bool output_variable)
    string (STRIP "${input_bool}" ${input_bool})
    string (TOLOWER "${input_bool}" ${input_bool})
    if ("${input_bool}" STREQUAL "on" OR "${input_bool}" STREQUAL "true" OR "${input_bool}" STREQUAL "1")
        set (${output_variable} ON PARENT_SCOPE)
    else()
        set (${output_variable} OFF PARENT_SCOPE)
    endif()
endfunction()

function (_yup_file_to_byte_array file_path output_variable)
    file (READ ${file_path} hex_contents HEX)
    string (REGEX MATCHALL "([A-Fa-f0-9][A-Fa-f0-9])" separated_hex ${hex_contents})

    list (JOIN separated_hex ", 0x" formatted_hex)
    string (PREPEND formatted_hex "0x")
    string (APPEND formatted_hex "")

    set (${output_variable} ${formatted_hex} PARENT_SCOPE)
endfunction()

function (_yup_get_package_config_libs package_name output_variable)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (${package_name} REQUIRED IMPORTED_TARGET ${package_name})
    set (${output_variable} "PkgConfig::${package_name}" PARENT_SCOPE)
endfunction()

#==============================================================================

macro(_yup_setup_platform)
    if (IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR CMAKE_TOOLCHAIN_FILE MATCHES ".*ios\.cmake$")
        set (yup_platform "ios")
    elseif (ANDROID)
        set (yup_platform "android")
    elseif (EMSCRIPTEN OR CMAKE_TOOLCHAIN_FILE MATCHES ".*Emscripten\.cmake$")
        set (yup_platform "emscripten")
    elseif (APPLE)
        set (yup_platform "osx")
    elseif (UNIX)
        set (yup_platform "linux")
    elseif (WIN32)
        if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            set (yup_platform "uwp")
        else()
            set (yup_platform "win32")
        endif()
    else()
        set (yup_platform "unknown")
    endif()

    message (STATUS "YUP -- Setting up for ${yup_platform} platform")
endmacro()

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
                message (FATAL_ERROR "YUP -- Invalid module declaration")
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
    set (${output_module_user_configs} "${module_user_configs}")
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

        if (NOT "${yup_platform}" MATCHES "^(ios|osx|android|linux|emscripten)$")
            list (FILTER found_source_files EXCLUDE REGEX "${base_path}*_posix${extension}")
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
                                   module_defines
                                   module_sources
                                   module_libs
                                   module_frameworks
                                   module_dependencies
                                   module_arc_enabled)
    target_sources (${module_name} INTERFACE ${module_sources})

    if (module_cpp_standard)
        target_compile_features (${module_name} INTERFACE cxx_std_${module_cpp_standard})
    else()
        target_compile_features (${module_name} INTERFACE cxx_std_11)
    endif()

    set_target_properties (${module_name} PROPERTIES
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       "hidden"
        VISIBILITY_INLINES_HIDDEN   ON)

    if ("${yup_platform}" MATCHES "^(osx|ios)$")
        set_target_properties (${module_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC ${module_arc_enabled})
    endif()

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

endfunction()

#==============================================================================

function (_yup_module_setup_plugin_client_clap target_name plugin_client_target)
    if ("${yup_platform}" MATCHES "^(emscripten)$")
        return()
    endif()

    set (options "")
    set (one_value_args PLUGIN_ID PLUGIN_NAME PLUGIN_VENDOR PLUGIN_VERSION PLUGIN_DESCRIPTION PLUGIN_URL PLUGING_IS_SYNTH PLUGIN_IS_MONO)
    set (multi_value_args "")

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (custom_target_name "${target_name}_clap")

    add_library (${custom_target_name} INTERFACE)

    get_target_property (module_path ${plugin_client_target} YUP_MODULE_PATH)
    get_target_property (module_cpp_standard ${plugin_client_target} YUP_MODULE_CPP_STANDARD)
    get_target_property (module_include_paths ${plugin_client_target} YUP_MODULE_INCLUDE_PATHS)
    get_target_property (module_defines ${plugin_client_target} YUP_MODULE_DEFINES)
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
        file (GLOB_RECURSE module_sources "${module_path}/clap/*.mm")
    else()
        file (GLOB_RECURSE module_sources "${module_path}/clap/*.cpp")
    endif()

    _yup_module_setup_target (${custom_target_name}
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
                              "${module_frameworks}"
                              "${module_dependencies}"
                              "${module_arc_enabled}")

    file (GLOB_RECURSE all_module_files_clap "${module_path}/clap/*")
    add_library (${custom_target_name}-module INTERFACE ${all_module_files_clap})
    source_group (TREE ${module_path}/clap/ FILES ${all_module_files_clap})
    set_target_properties (${custom_target_name}-module PROPERTIES FOLDER "Modules")

endfunction()

#==============================================================================

function (yup_add_module module_path)
    get_filename_component (module_path ${module_path} ABSOLUTE)
    get_filename_component (module_name ${module_path} NAME)

    message (STATUS "YUP -- Processing module " ${module_name} " at " ${module_path})
    set (${module_name}_Found OFF PARENT_SCOPE)

    if (NOT EXISTS ${module_path})
        message (FATAL_ERROR "YUP -- Module location not found")
    endif()

    set (module_header "${module_path}/${module_name}.h")
    if (NOT EXISTS ${module_header})
        message (FATAL_ERROR "YUP -- Module header ${module_header} not found")
    endif()

    # ==== Add module as library
    add_library (${module_name} INTERFACE)

    # ==== Parse module declaration string
    _yup_module_parse_config ("${module_header}" module_configs module_user_configs)

    # ==== Assign configs to variables from module declaration string
    set (module_cpp_standard "")
    set (module_dependencies "")
    set (module_include_paths "")
    set (module_defines "")
    set (module_wasm_defines "")
    set (module_searchpaths "")
    set (module_searchpaths_private "")
    set (module_osx_frameworks "")
    set (module_osx_weak_frameworks "")
    set (module_osx_libs "")
    set (module_ios_frameworks "")
    set (module_ios_weak_frameworks "")
    set (module_ios_libs "")
    set (module_linux_libs "")
    set (module_linux_packages "")
    set (module_windows_libs "")
    set (module_mingw_libs "")
    set (module_wasm_libs "")
    set (module_arc_enabled OFF)

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
        elseif (${module_config_key} STREQUAL "WASMDefines")
            _yup_comma_or_space_separated_list (${module_config_value} module_wasm_defines)
        elseif (${module_config_key} STREQUAL "searchpaths")
            _yup_comma_or_space_separated_list (${module_config_value} module_searchpaths)
        elseif (${module_config_key} STREQUAL "OSXFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_frameworks)
        elseif (${module_config_key} STREQUAL "WeakOSXFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_weak_frameworks)
        elseif (${module_config_key} STREQUAL "OSXLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_osx_libs)
        elseif (${module_config_key} STREQUAL "iOSFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_frameworks)
        elseif (${module_config_key} STREQUAL "WeakiOSFrameworks")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_weak_frameworks)
        elseif (${module_config_key} STREQUAL "iOSLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_ios_libs)
        elseif (${module_config_key} STREQUAL "linuxLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_libs)
        elseif (${module_config_key} STREQUAL "linuxPackages")
            _yup_comma_or_space_separated_list (${module_config_value} module_linux_packages)
        elseif (${module_config_key} STREQUAL "windowsLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_windows_libs)
        elseif (${module_config_key} STREQUAL "mingwLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_mingw_libs)
        elseif (${module_config_key} STREQUAL "wasmLibs")
            _yup_comma_or_space_separated_list (${module_config_value} module_wasm_libs)
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
        set (module_libs "${module_ios_libs}")
        _yup_module_prepare_frameworks ("${module_ios_frameworks}" "${module_ios_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(osx)$")
        set (module_libs "${module_osx_libs}")
        _yup_module_prepare_frameworks ("${module_osx_frameworks}" "${module_osx_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(linux)$")
        set (module_libs "${module_linux_libs}")
        foreach (package ${module_linux_packages})
            _yup_get_package_config_libs ("${package}" package_libs)
            list (APPEND module_libs "${package_libs}")
        endforeach()
    elseif ("${yup_platform}" MATCHES "^(emscripten)$")
        set (module_libs "${module_wasm_libs}")
    elseif ("${yup_platform}" MATCHES "^(win32|uwp)$" AND NOT)
        if (MINGW)
            set (module_libs "${module_mingw_libs}")
        else()
            set (module_libs "${module_windows_libs}")
        endif()
    endif()

    if (("${module_name}" STREQUAL "juce_audio_devices") AND ("${yup_platform}" MATCHES "^(android)$"))
        add_subdirectory("${module_path}/native/oboe")
        list (APPEND module_libs oboe)
    endif()

    # ==== Prepare include paths
    get_filename_component (module_include_path ${module_path} DIRECTORY)
    list (APPEND module_include_paths "${module_include_path}")

    foreach (searchpath ${module_searchpaths})
        if (EXISTS "${module_path}/${searchpath}")
            list (APPEND module_include_paths "${module_path}/${searchpath}")
        endif()
    endforeach()

    # ==== Prepare defines
    if ("${yup_platform}" MATCHES "^(emscripten)$")
        list (APPEND module_defines ${module_wasm_defines})
    endif()

    # ==== Setup module sources and properties
    _yup_module_setup_target (${module_name}
                              "${module_cpp_standard}"
                              "${module_include_paths}"
                              "${module_defines}"
                              "${module_sources}"
                              "${module_libs}"
                              "${module_frameworks}"
                              "${module_dependencies}"
                              "${module_arc_enabled}")

    #set (${module_name}_Configs "${module_user_configs}")
    #set (${module_name}_Configs ${${module_name}_Configs} PARENT_SCOPE)

    file (GLOB_RECURSE all_module_files "${module_path}/*")
    add_library (${module_name}-module INTERFACE ${all_module_files})
    source_group (TREE ${module_path}/ FILES ${all_module_files})
    set_target_properties (${module_name}-module PROPERTIES FOLDER "Modules")

    # ==== Setup parent scope variables
    set (${module_name}_Found ON PARENT_SCOPE)
    set_target_properties (${module_name} PROPERTIES
        YUP_MODULE_PATH "${module_path}"
        YUP_MODULE_HEADER "${module_header}"
        YUP_MODULE_CPP_STANDARD "${module_cpp_standard}"
        YUP_MODULE_INCLUDE_PATHS "${module_include_paths}"
        YUP_MODULE_DEFINES "${module_defines}"
        YUP_MODULE_SOURCES "${module_sources}"
        YUP_MODULE_LIBS "${module_libs}"
        YUP_MODULE_FRAMEWORK "${module_frameworks}"
        YUP_MODULE_DEPENDENCIES "${module_dependencies}"
        YUP_MODULE_ARC_ENABLED "${module_arc_enabled}")

endfunction()

#==============================================================================

function (yup_standalone_app)
    # ==== Fetch options
    set (options CONSOLE)
    set (one_value_args TARGET_NAME TARGET_VERSION TARGET_IDE_GROUP)
    set (multi_value_args DEFINITIONS MODULES LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (target_version "${YUP_ARG_TARGET_VERSION}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "")
    set (additional_link_options "")

    _yup_make_short_version ("${target_version}" target_version_short)

    # ==== Find dependencies
    if (NOT "${yup_platform}" MATCHES "^(emscripten|ios)$")
        include (FetchContent)

        FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG master)
        set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable (glfw)

        list (APPEND additional_libraries glfw)
    endif()

    # ==== Prepare executable
    add_executable (${target_name})
    target_compile_features (${target_name} PRIVATE cxx_std_17)

    # ==== Per platform configuration
    if ("${yup_platform}" MATCHES "^(osx|ios)$")
        if (NOT YUP_ARG_CONSOLE)
            get_filename_component (plist_path "cmake/platforms/${yup_platform}/Info.plist" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")

            set_target_properties (${target_name} PROPERTIES
                BUNDLE                                         ON
                CXX_EXTENSIONS                                 OFF
                MACOSX_BUNDLE_EXECUTABLE_NAME                  "${target_name}"
                MACOSX_BUNDLE_GUI_IDENTIFIER                   "org.kunitoki.yup.${target_name}"
                MACOSX_BUNDLE_BUNDLE_NAME                      "${target_name}"
                MACOSX_BUNDLE_BUNDLE_VERSION                   "${target_version}"
                MACOSX_BUNDLE_LONG_VERSION_STRING              "${target_version}"
                MACOSX_BUNDLE_SHORT_VERSION_STRING             "${target_version_short}"
                MACOSX_BUNDLE_ICON_FILE                        "Icon.icns"
                MACOSX_BUNDLE_INFO_PLIST                       "${plist_path}"
                #RESOURCE                                       "${RESOURCE_FILES}"
                #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY             ""
                XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED          OFF
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       dwarf
                XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
                XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME        OFF)
        endif()

        set_target_properties (${target_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC OFF)

    elseif ("${yup_platform}" MATCHES "^(emscripten)$")
        if (NOT YUP_ARG_CONSOLE)
            set_target_properties (${target_name} PROPERTIES SUFFIX ".html")

            list (APPEND additional_definitions RIVE_WEBGL=1)
            list (APPEND additional_link_options -sUSE_GLFW=3 -sMAX_WEBGL_VERSION=2)
        endif()

        list (APPEND additional_options
            -fexceptions
            -sDISABLE_EXCEPTION_CATCHING=0)

        list (APPEND additional_link_options
            $<$<CONFIG:DEBUG>:-gsource-map>
            -fexceptions
            -sWASM=1
            -sASSERTIONS=1
            -sDISABLE_EXCEPTION_CATCHING=0
            -sERROR_ON_UNDEFINED_SYMBOLS=1
            -sDEMANGLE_SUPPORT=1
            -sSTACK_OVERFLOW_CHECK=2
            -sFORCE_FILESYSTEM=1
            -sALLOW_MEMORY_GROWTH=1
            -sNODERAWFS=0
            -sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$dynCall')

    endif()

    if (YUP_ARG_TARGET_IDE_GROUP)
        set_target_properties (${target_name} PROPERTIES FOLDER "${YUP_ARG_TARGET_IDE_GROUP}")
    endif()

    # ==== Definitions and link libraries
    target_compile_options (${target_name} PRIVATE
        ${additional_options}
        ${YUP_ARG_OPTIONS})

    target_compile_definitions (${target_name} PRIVATE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=1
        ${additional_definitions}
        ${YUP_ARG_DEFINITIONS})

    target_link_options (${target_name} PRIVATE
        ${additional_link_options}
        ${YUP_ARG_LINK_OPTIONS})

    target_link_libraries (${target_name} PRIVATE
        ${additional_libraries}
        ${YUP_ARG_MODULES})

endfunction()

#==============================================================================

function (yup_audio_plugin)
    # ==== Fetch options
    set (options CONSOLE)
    set (one_value_args TARGET_NAME TARGET_IDE_GROUP PLUGIN_CREATE_CLAP PLUGIN_CREATE_STANDALONE)
    set (multi_value_args DEFINITIONS MODULES LINK_OPTIONS)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (target_name "${YUP_ARG_TARGET_NAME}")
    set (additional_definitions "")
    set (additional_options "")
    set (additional_libraries "yup_audio_plugin_client")
    set (additional_link_options "")

    # ==== Find dependencies
    include (FetchContent)

    if (NOT "${yup_platform}" MATCHES "^(emscripten)$")
        FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG master)
        set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set (GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable (glfw)
        list (APPEND additional_libraries glfw)

        # ==== Fetch plugins SDKS
        if (YUP_ARG_PLUGIN_CREATE_CLAP)
            FetchContent_Declare(clap GIT_REPOSITORY https://github.com/free-audio/clap.git GIT_TAG main)
            FetchContent_MakeAvailable (clap)
            set_target_properties (clap-tests PROPERTIES FOLDER "Tests")

            _yup_module_setup_plugin_client_clap (${target_name} yup_audio_plugin_client ${YUP_ARG_UNPARSED_ARGUMENTS})

            list (APPEND additional_libraries clap ${target_name}_clap)
        endif()

        if (NOT YUP_ARG_PLUGIN_CREATE_CLAP AND NOT PLUGIN_CREATE_STANDALONE) #  AND NOT YUP_ARG_PLUGIN_CREATE_VST3 ...
            message (FATAL_ERROR "YUP -- Cannot enable audio plugins on WASM targets yet")
        endif()
    endif()

    if (YUP_ARG_PLUGIN_CREATE_STANDALONE)
        list (APPEND additional_definitions YUP_AUDIO_PLUGIN_ENABLE_STANDALONE=1)
    endif()

    # ==== Prepare shared binary
    add_library (${target_name} SHARED)
    target_compile_features (${target_name} PRIVATE cxx_std_17)

    # ==== Per platform configuration
    if ("${yup_platform}" MATCHES "^(osx)$")
        #if (NOT YUP_ARG_CONSOLE)
        #    set_target_properties (${target_name} PROPERTIES
        #        BUNDLE                                         ON
        #        CXX_EXTENSIONS                                 OFF
        #        MACOSX_BUNDLE_GUI_IDENTIFIER                   "org.kunitoki.yup.${target_name}"
        #        MACOSX_BUNDLE_NAME                             "${target_name}"
        #        MACOSX_BUNDLE_VERSION                          "1.0.0"
        #        #MACOSX_BUNDLE_ICON_FILE                        "Icon.icns"
        #        MACOSX_BUNDLE_INFO_PLIST                       "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms/${yup_platform}/Info.plist"
        #        #RESOURCE                                       "${RESOURCE_FILES}"
        #        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY             ""
        #        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED          OFF
        #        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT       dwarf
        #        XCODE_ATTRIBUTE_GCC_INLINES_ARE_PRIVATE_EXTERN ON
        #        XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME        OFF)
        #endif()

        set_target_properties (${target_name} PROPERTIES
            XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC OFF)

    endif()

    if (YUP_ARG_PLUGIN_CREATE_CLAP)
        set_target_properties (${target_name} PROPERTIES SUFFIX ".clap")
    endif()

    if (YUP_ARG_TARGET_IDE_GROUP)
        set_target_properties (${target_name} PROPERTIES FOLDER "${YUP_ARG_TARGET_IDE_GROUP}")
    endif()

    # ==== Definitions and link libraries
    target_compile_options (${target_name} PRIVATE
        ${additional_options}
        ${YUP_ARG_OPTIONS})

    target_compile_definitions (${target_name} PRIVATE
        $<IF:$<CONFIG:Debug>,DEBUG=1,NDEBUG=1>
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        JUCE_STANDALONE_APPLICATION=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        ${additional_definitions}
        ${YUP_ARG_DEFINITIONS})

    target_link_options (${target_name} PRIVATE
        ${additional_link_options}
        ${YUP_ARG_LINK_OPTIONS})

    target_link_libraries (${target_name} PRIVATE
        ${additional_libraries}
        ${YUP_ARG_MODULES})

endfunction()

#==============================================================================

function (yup_add_embedded_binary_resources library_name)
    set (options "")
    set (one_value_args OUT_DIR HEADER NAMESPACE)
    set (multi_value_args RESOURCE_NAMES RESOURCES)

    cmake_parse_arguments (YUP_ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set (binary_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}")
    set (binary_header_path "${binary_path}/${YUP_ARG_HEADER}")

    add_library (${library_name} OBJECT)

    target_include_directories (${library_name} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_features (${library_name} PUBLIC cxx_std_17)

    set_target_properties (${library_name} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    file (WRITE "${binary_header_path}"
        "#pragma once\n"
        "\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "\n")

    if (DEFINED YUP_ARG_NAMESPACE)
        file (APPEND "${binary_header_path}"
            "namespace ${YUP_ARG_NAMESPACE}\n"
            "{\n"
            "\n")
    endif()

    foreach (resource_name resource IN ZIP_LISTS YUP_ARG_RESOURCE_NAMES YUP_ARG_RESOURCES)
        set (full_resource_unit_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}/${resource_name}.cpp")
        set (full_resource_hex_path "${CMAKE_CURRENT_BINARY_DIR}/${YUP_ARG_OUT_DIR}/${resource_name}.inc")

        # Add symbol to header
        file (APPEND "${binary_header_path}"
            "extern const uint8_t ${resource_name}[];\n"
            "extern const std::size_t ${resource_name}_size;\n"
            "\n")

        # Write .cpp
        file (WRITE "${full_resource_unit_path}"
            "#include \"${YUP_ARG_HEADER}\"\n"
            "\n"
            "#include <cstdint>\n"
            "\n")

        if (DEFINED YUP_ARG_NAMESPACE)
            file (APPEND "${full_resource_unit_path}"
                "namespace ${YUP_ARG_NAMESPACE}\n"
                "{\n"
                "\n")
        endif()

        file (APPEND "${full_resource_unit_path}"
            "const uint8_t ${resource_name}_data[] = \n"
            "{\n"
            "#include \"${resource_name}.inc\"\n"
            "};\n"
            "\n"
            "const std::size_t ${resource_name}_size = sizeof (${resource_name}_data);\n"
            "\n")

        if (DEFINED YUP_ARG_NAMESPACE)
            file (APPEND "${full_resource_unit_path}"
                "\n"
                "} // namespace ${YUP_ARG_NAMESPACE}\n")
        endif()

        target_sources (${library_name} PRIVATE "${full_resource_unit_path}")

        _yup_file_to_byte_array (${resource} resource_byte_array)
        file (WRITE "${full_resource_hex_path}" "${resource_byte_array}")

        list (APPEND resources_hex_files "${full_resource_hex_path}")
    endforeach()

    if (DEFINED YUP_ARG_NAMESPACE)
        file (APPEND "${binary_header_path}"
            "} // namespace ${YUP_ARG_NAMESPACE}\n")
    endif()

    target_sources (${library_name} PUBLIC "${binary_header_path}")
    target_include_directories (${library_name} PUBLIC "${binary_path}")

    add_custom_target ("${library_name}_content" DEPENDS "${resources_hex_files}")
    add_dependencies (${library_name} "${library_name}_content")

endfunction()
