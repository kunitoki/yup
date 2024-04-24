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

function (_yup_comma_or_space_separated_list input_list output_variable)
    string (REPLACE "," " " temp1_list ${input_list})
    string (REPLACE " " ";" temp2_list ${temp1_list})
    _yup_strip_list ("${temp2_list}" final_list)
    set (${output_variable} "${final_list}" PARENT_SCOPE)
endfunction()

function (_yup_source_group sources base)
    get_filename_component (base "${base}" ABSOLUTE)
    foreach (source ${sources})
        get_filename_component (source "${source}" ABSOLUTE)
        get_filename_component (parent_dir "${source}" DIRECTORY)
        if (NOT "${parent_dir}" STREQUAL "")
            string (REPLACE "${base}" "" parent_dir_stripped "${parent_dir}")
            string (REPLACE "/" "\\\\" group "${parent_dir_stripped}")
            source_group ("${group}" FILES "${source}")
        endif()
    endforeach()
endfunction()

#==============================================================================

macro(_yup_setup_platform)
    if (DEFINED IOS)
        set (yup_platform "ios")
    elseif (DEFINED ANDROID)
        set (yup_platform "android")
    elseif (DEFINED EMSCRIPTEN)
        set (yup_platform "emscripten")
    elseif (DEFINED APPLE)
        set (yup_platform "osx")
    elseif (DEFINED UNIX)
        set (yup_platform "linux")
    elseif (DEFINED WIN32)
        if (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
            set (yup_platform "uwp")
        else()
            set (yup_platform "win32")
        endif()
    else()
        set (yup_platform "unknown")
    endif()
endmacro()

#==============================================================================

function (_yup_target_include_directories target_name folder)
    if (EXISTS "${folder}/include")
        target_include_directories ("${target_name}" PUBLIC "${folder}/include")
    endif()

    if (EXISTS "${folder}/platform_include")
        target_include_directories ("${target_name}" PUBLIC "${folder}/platform_include/${yup_platform}")

        if ("${yup_platform}" MATCHES "^(ios|osx|android|linux|emscripten)$")
            target_include_directories ("${target_name}" PUBLIC "${folder}/platform_include/posix")
        endif()

        if ("${yup_platform}" MATCHES "^(ios|osx)$")
            target_include_directories ("${target_name}" PUBLIC "${folder}/platform_include/apple")
        endif()

        if ("${yup_platform}" MATCHES "^(win32|uwp)$")
            target_include_directories ("${target_name}" PUBLIC "${folder}/platform_include/msft")
        endif()
    endif()

    target_include_directories ("${target_name}" PRIVATE "${folder}/source/common")
    target_include_directories ("${target_name}" PRIVATE "${folder}/source/${yup_platform}")

    if ("${yup_platform}" MATCHES "^(ios|osx|android|linux|emscripten)$")
       target_include_directories ("${target_name}" PRIVATE "${folder}/source/posix")
    endif()

    if ("${yup_platform}" MATCHES "^(ios|osx)$")
        target_include_directories ("${target_name}" PRIVATE "${folder}/source/apple")
    endif()

    if ("${yup_platform}" MATCHES "^(win32|uwp)$")
        target_include_directories ("${target_name}" PRIVATE "${folder}/source/msft")
    endif()
endfunction()

function (_yup_collect_source_files source_files_var header_files_var folder)
    set(source_files "")
    set(source_extensions ".c;.cc;.cpp;.cxx;.h;.hh;.hpp;.hxx;.inl")

    if (APPLE)
        list (APPEND source_extensions ".m" ".mm")
    endif()

    foreach (extension ${source_extensions})
        file (GLOB_RECURSE found_source_files
            "${folder}/source/common/*${extension}"
            "${folder}/source/${yup_platform}/*${extension}")
        list (APPEND source_files ${found_source_files})

        if ("${yup_platform}" MATCHES "^(ios|osx|android|linux|emscripten)$")
            file (GLOB_RECURSE posix_source_files "${folder}/source/posix/*${extension}")
            list (APPEND source_files ${posix_source_files})
        endif()

        if ("${yup_platform}" MATCHES "^(ios|osx)$")
            file (GLOB_RECURSE apple_source_files "${folder}/source/apple/*${extension}")
            list (APPEND source_files ${apple_source_files})
        endif()

        if ("${yup_platform}" MATCHES "^(win32|uwp)$")
            file (GLOB_RECURSE msft_source_files "${folder}/source/msft/*${extension}")
            list (APPEND source_files ${msft_source_files})
        endif()
    endforeach()
    set (${source_files_var} "${source_files}" PARENT_SCOPE)

    set (header_extensions ".h;.hh;.hpp;.inl")
    set (header_files "")
    foreach (extension ${header_extensions})
        file (GLOB_RECURSE found_header_files "${folder}/include/*${extension}")
        list (APPEND header_files ${found_header_files})
    endforeach()
    set (${header_files_var} "${header_files}" PARENT_SCOPE)
endfunction()

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

    set (module_sources "${module_sources}" PARENT_SCOPE)
endfunction()

#==============================================================================

function (_yup_prepare_frameworks frameworks weak_frameworks output_variable)
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

function (yup_add_target target_name target_path dependencies public_definitions public_includes public_libs private_definitions private_includes private_libs)
    _yup_collect_source_files (sources headers ${target_path})
    _yup_source_group ("${sources}" ${target_path})
    _yup_source_group ("${headers}" ${target_path})

    add_library (${target_name} STATIC ${sources} ${headers})
    target_compile_features (${target_name} PRIVATE cxx_std_17)

    target_compile_definitions (${target_name} PUBLIC ${public_definitions})
    target_compile_definitions (${target_name} PRIVATE ${private_definitions})

    _yup_target_include_directories (${target_name} ${target_path})
    target_include_directories (${target_name} PUBLIC ${public_includes})
    target_include_directories (${target_name} PRIVATE ${private_includes})

    target_link_libraries (${target_name} PUBLIC ${public_libs})
    target_link_libraries (${target_name} PRIVATE ${private_libs})

    set_target_properties (${target_name} PROPERTIES CXX_VISIBILITY_PRESET "hidden")
    set_target_properties (${target_name} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
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

    # ==== Assign configs to variables from module declaration string
    set (module_dependencies "")
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

    set (parsed_dependencies "")
    foreach (module_config ${module_configs})
        string (REGEX REPLACE "^(.+):([ \t\r\n]+.*)$" "\\1" module_config_key ${module_config})
        string (REGEX REPLACE "^.+:[ \t\r\n]+(.+)$" "\\1" module_config_value ${module_config})

        if (${module_config_key} STREQUAL "dependencies")
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
        endif()
    endforeach()

    # ==== Scan sources to include
    _yup_module_collect_sources (${module_path} all_module_sources)

    # ==== Setup libs and frameworks
    set (module_frameworks "")
    if ("${yup_platform}" MATCHES "^(ios)$")
        list (JOIN module_ios_libs " " module_libs)
        _yup_prepare_frameworks ("${module_ios_frameworks}" "${module_ios_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(osx)$")
        list (JOIN module_osx_libs " " module_libs)
        _yup_prepare_frameworks ("${module_osx_frameworks}" "${module_osx_weak_frameworks}" module_frameworks)
    elseif ("${yup_platform}" MATCHES "^(linux)$")
        list (JOIN module_linux_libs " " module_libs)
    elseif ("${yup_platform}" MATCHES "^(win32|uwp)$" AND NOT)
        if (MINGW)
            list (JOIN module_mingw_libs " " module_libs)
        else()
            list (JOIN module_windows_libs " " module_libs)
        endif()
    endif()

    # ==== Prepare include paths
    get_filename_component (module_include_path ${module_path} DIRECTORY)

    set (module_additional_include_paths "")
    foreach (searchpath ${module_searchpaths})
        if (EXISTS "${module_path}/${searchpath}")
            list (APPEND module_additional_include_paths "${module_path}/${searchpath}")
        endif()
    endforeach()

    # ==== Prepare defines
    if ("${yup_platform}" MATCHES "^(emscripten)$")
        list (APPEND module_defines ${module_wasm_defines})
    endif()

    # ==== Setup module sources and properties
    target_sources (${module_name} INTERFACE ${module_sources})

    set_target_properties (${module_name} PROPERTIES
        CXX_STANDARD                17
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       "hidden"
        VISIBILITY_INLINES_HIDDEN   ON)

    target_compile_definitions (${module_name} INTERFACE
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
        ${module_defines})

    target_include_directories (${module_name} INTERFACE
        ${module_include_path}
        ${module_additional_include_paths})

    target_link_libraries (${module_name} INTERFACE
        ${module_libs}
        ${module_frameworks})

    target_link_libraries (${module_name} INTERFACE
        ${module_dependencies})

    #set (${module_name}_Deps "${module_dependencies}")
    #set (${module_name}_Deps ${${module_name}_Deps} PARENT_SCOPE)

    #set (${module_name}_Configs "${module_user_configs}")
    #set (${module_name}_Configs ${${module_name}_Configs} PARENT_SCOPE)

    #file (GLOB_RECURSE all_module_files "${module_path}/*")
    #_yup_source_group (${all_module_files} "${module_path}")
    #add_library (${module_name}-files INTERFACE ${all_module_files})

endfunction()
