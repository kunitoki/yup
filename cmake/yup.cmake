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


function (_yup_source_group sources base)
    foreach(source ${sources})
        get_filename_component(parent_dir "${source}" PATH)
        if(NOT "${parent_dir}" STREQUAL "")
            string(REPLACE "${base}" "" parent_dir_stripped "${parent_dir}")
            string(REPLACE "/" "\\\\" group "${parent_dir_stripped}")
            source_group("${group}" FILES "${source}")
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
    set(source_extensions ".c;.cc;.cpp;.cxx;.m;.mm;.ui;.h;.hh;.hpp;.inl")
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

function (yup_add_module module_name module_path)
    if (IS_ABSOLUTE ${module_path})
    else()
        set (module_path "${CMAKE_CURRENT_SOURCE_DIR}/${module_path}")
    endif()

    message (STATUS "YUP -- Processing module " ${module_name} " at " ${module_path})
    set (${module_name}_Found OFF PARENT_SCOPE)

    if (NOT EXISTS ${module_path})
        message (FATAL_ERROR "YUP -- Module location not found")
    endif()

    set (module_header "${module_path}/${module_name}.h")
    if (NOT EXISTS ${module_header})
        message (FATAL_ERROR "YUP -- Module header ${module_header} not found")
    endif()

    #set (module_cmake "${module_path}/CMakeLists.txt")
    #if (NOT EXISTS ${module_cmake})
    #    message (FATAL_ERROR "YUP -- Module cmake ${module_cmake} not found")
    #endif()

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

    set (module_osx_frameworks "")
    set (module_ios_frameworks "")
    set (module_linux_libs "")
    set (module_linux_packages "")
    set (module_mingw_libs "")

    set (parsed_dependencies "")
    foreach (module_config ${module_configs})
        string (REGEX REPLACE "^(.+):([ \t\r\n]+.*)$" "\\1" module_config_key ${module_config})
        string (REGEX REPLACE "^.+:[ \t\r\n]+(.+)$" "\\1" module_config_value ${module_config})

        if (${module_config_key} STREQUAL "dependencies")
            string (REPLACE "," ";" parsed_dependencies ${module_config_value})
        elseif (${module_config_key} STREQUAL "OSXFrameworks")
            string (REPLACE " " ";" module_osx_frameworks ${module_config_value})
        elseif (${module_config_key} STREQUAL "iOSFrameworks")
            string (REPLACE " " ";" module_ios_frameworks ${module_config_value})
        elseif (${module_config_key} STREQUAL "linuxLibs")
            string (REPLACE " " ";" module_linux_libs ${module_config_value})
        elseif (${module_config_key} STREQUAL "linuxPackages")
            string (REPLACE " " ";" module_linux_packages ${module_config_value})
        elseif (${module_config_key} STREQUAL "mingwLibs")
            string (REPLACE " " ";" module_mingw_libs ${module_config_value})
        endif()
    endforeach()
    _yup_strip_list ("${parsed_dependencies}" module_dependencies)

    get_filename_component (module_include_path ${module_path} DIRECTORY)

    if (APPLE)
        file (GLOB all_module_sources
            "${module_path}/${module_name}*.h"
            "${module_path}/${module_name}*.c"
            "${module_path}/${module_name}*.cpp"
            "${module_path}/${module_name}*.m"
            "${module_path}/${module_name}*.mm")

        set (module_sources "")
        foreach (module_source ${all_module_sources})
            if (module_source MATCHES "^.*\.cpp$")
                get_filename_component (source_directory ${module_source} DIRECTORY)
                get_filename_component (source_file ${module_source} NAME_WLE)
                set (imported_module_source "${source_directory}/${source_file}.mm")
                if (${imported_module_source} IN_LIST all_module_sources)
                    continue()
                endif()
                list (APPEND module_sources ${module_source})
            elseif (module_source MATCHES "^.*\.c$")
                get_filename_component (source_directory ${module_source} DIRECTORY)
                get_filename_component (source_file ${module_source} NAME_WLE)
                set (imported_module_source "${source_directory}/${source_file}.m")
                if (${imported_module_source} IN_LIST all_module_sources)
                    continue()
                endif()
                list (APPEND module_sources ${module_source})
            else()
                list (APPEND module_sources ${module_source})
            endif()
        endforeach()

        set (frameworks "")
        foreach (framework ${module_osx_frameworks})
            list (APPEND frameworks "-framework ${framework}")
        endforeach()
        list (JOIN frameworks " " module_frameworks)
    else()
        file (GLOB module_sources
            "${module_path}/${module_name}*.h"
            "${module_path}/${module_name}*.c"
            "${module_path}/${module_name}*.cpp")

        set (module_frameworks "")
    endif()

    file (GLOB_RECURSE all_module_files "${module_path}/*")
    #list (FILTER all_module_files EXCLUDE REGEX "^\..*$")

    set (${module_name}_Found ON)
    set (${module_name}_Found ${${module_name}_Found} PARENT_SCOPE)

    set (${module_name}_Includes "${module_include_path}")
    set (${module_name}_Includes ${${module_name}_Includes} PARENT_SCOPE)

    set (${module_name}_Deps "${module_dependencies}")
    set (${module_name}_Deps ${${module_name}_Deps} PARENT_SCOPE)

    set (${module_name}_Configs "${module_user_configs}")
    set (${module_name}_Configs ${${module_name}_Configs} PARENT_SCOPE)

    set (${module_name}_Files "${all_module_files}")
    set (${module_name}_Files ${${module_name}_Files} PARENT_SCOPE)

    set (${module_name}_Sources "${module_sources}")
    set (${module_name}_Sources ${${module_name}_Sources} PARENT_SCOPE)

    set (${module_name}_Libs "${module_frameworks}")
    set (${module_name}_Libs ${${module_name}_Libs} PARENT_SCOPE)

    #message (STATUS "id =     ${module_name}")
    #message (STATUS "path =   ${module_include_path}")
    #message (STATUS "deps =   ${module_dependencies}")
    #message (STATUS "cfgs =   ${module_user_configs}")
    #message (STATUS "srcs =   ${module_sources}")

    add_library (${module_name} INTERFACE ${all_module_files})

endfunction()
