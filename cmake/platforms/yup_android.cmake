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

include_guard (DIRECTORY)

include (${CMAKE_CURRENT_LIST_DIR}/../yup_utilities.cmake)

#==============================================================================

function (_yup_android_prepare_gradle)
    set (options "")
    set (one_value_args
        MIN_SDK_VERSION COMPILE_SDK_VERSION TARGET_SDK_VERSION
        TARGET_NAME TARGET_ICON ABI TOOLCHAIN PLATFORM STL CPP_VERSION CMAKE_VERSION
        APPLICATION_ID APPLICATION_NAMESPACE APPLICATION_CMAKELISTS_PATH APPLICATION_VERSION)
    set (multi_value_args "")

    cmake_parse_arguments (YUP_ANDROID "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    # Prepare variables
    _yup_set_default (YUP_ANDROID_MIN_SDK_VERSION "21")
    _yup_set_default (YUP_ANDROID_COMPILE_SDK_VERSION "34")
    _yup_set_default (YUP_ANDROID_TARGET_SDK_VERSION "${YUP_ANDROID_COMPILE_SDK_VERSION}")
    _yup_set_default (YUP_ANDROID_TARGET_NAME "default_app")
    _yup_set_default (YUP_ANDROID_TOOLCHAIN "clang")
    _yup_set_default (YUP_ANDROID_PLATFORM "android-${YUP_ANDROID_MIN_SDK_VERSION}")
    _yup_set_default (YUP_ANDROID_STL "c++_shared")
    _yup_set_default (YUP_ANDROID_CPP_VERSION "20")
    _yup_set_default (YUP_ANDROID_APPLICATION_NAMESPACE "org.yup")
    _yup_set_default (YUP_ANDROID_APPLICATION_ID "org.yup.default_app")
    _yup_set_default (YUP_ANDROID_APPLICATION_VERSION "1.0")
    _yup_set_default (YUP_ANDROID_APPLICATION_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
    _yup_set_default (YUP_ANDROID_ABI "arm64-v8a")
    _yup_set_default (YUP_ANDROID_CMAKE_VERSION "${CMAKE_VERSION}")

    _yup_join_list_with_separator ("${YUP_ANDROID_ABI}" "\n            " "abiFilters += \"" "\"" YUP_ANDROID_ABI)
    _yup_version_string_to_version_code (${YUP_ANDROID_APPLICATION_VERSION} YUP_ANDROID_APPLICATION_VERSION_CODE)
    file (RELATIVE_PATH YUP_ANDROID_APPLICATION_PATH "${CMAKE_CURRENT_BINARY_DIR}/app" "${YUP_ANDROID_APPLICATION_PATH}")

    # Prepare files
    set (BASE_FILES_PATH "${CMAKE_SOURCE_DIR}/cmake/platforms/android")
    configure_file (${BASE_FILES_PATH}/build.gradle.kts.in ${CMAKE_CURRENT_BINARY_DIR}/build.gradle.kts)
    configure_file (${BASE_FILES_PATH}/settings.gradle.kts.in ${CMAKE_CURRENT_BINARY_DIR}/settings.gradle.kts)
    configure_file (${BASE_FILES_PATH}/app/build.gradle.kts.in ${CMAKE_CURRENT_BINARY_DIR}/app/build.gradle.kts)
    configure_file (${BASE_FILES_PATH}/app/proguard-rules.pro.in ${CMAKE_CURRENT_BINARY_DIR}/app/proguard-rules.pro)
    configure_file (${BASE_FILES_PATH}/app/src/main/java/org/yup/YupActivity.java.in ${CMAKE_CURRENT_BINARY_DIR}/app/src/main/java/org/yup/YupActivity.java)
    configure_file (${BASE_FILES_PATH}/app/src/main/AndroidManifest.xml.in ${CMAKE_CURRENT_BINARY_DIR}/app/src/main/AndroidManifest.xml)
    configure_file (${BASE_FILES_PATH}/gradle/libs.versions.toml.in ${CMAKE_CURRENT_BINARY_DIR}/gradle/libs.versions.toml COPYONLY)
    configure_file (${BASE_FILES_PATH}/gradle/wrapper/gradle-wrapper.jar.in ${CMAKE_CURRENT_BINARY_DIR}/gradle/wrapper/gradle-wrapper.jar COPYONLY)
    configure_file (${BASE_FILES_PATH}/gradle/wrapper/gradle-wrapper.properties.in ${CMAKE_CURRENT_BINARY_DIR}/gradle/wrapper/gradle-wrapper.properties COPYONLY)
    configure_file (${BASE_FILES_PATH}/gradlew.in ${CMAKE_CURRENT_BINARY_DIR}/gradlew COPYONLY)
    configure_file (${BASE_FILES_PATH}/gradlew.bat.in ${CMAKE_CURRENT_BINARY_DIR}/gradlew.bat COPYONLY)
    configure_file (${BASE_FILES_PATH}/gradle.properties.in ${CMAKE_CURRENT_BINARY_DIR}/gradle.properties COPYONLY)

    # Copy icons
    if (YUP_ANDROID_TARGET_ICON)
        set (base_icon_path "${CMAKE_CURRENT_BINARY_DIR}/app/src/main/res")

        find_program (sips_program sips)
        if (sips_program)
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-ldpi)
            _yup_execute_process_or_fail (${sips_program} -z 36 36 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-ldpi/ic_launcher.png")
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-mdpi)
            _yup_execute_process_or_fail (${sips_program} -z 48 48 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-mdpi/ic_launcher.png")
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-hdpi)
            _yup_execute_process_or_fail (${sips_program} -z 72 72 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-hdpi/ic_launcher.png")
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-xhdpi)
            _yup_execute_process_or_fail (${sips_program} -z 96 96 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-xhdpi/ic_launcher.png")
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-xxhdpi)
            _yup_execute_process_or_fail (${sips_program} -z 144 144 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-xxhdpi/ic_launcher.png")
            file (MAKE_DIRECTORY ${base_icon_path}/mipmap-xxxhdpi)
            _yup_execute_process_or_fail (${sips_program} -z 192 192 "${YUP_ANDROID_TARGET_ICON}" --out "${base_icon_path}/mipmap-xxxhdpi/ic_launcher.png")
        else()
            configure_file (${YUP_ANDROID_TARGET_ICON} ${base_icon_path}/mipmap-xxxhdpi/ic_launcher.png COPYONLY)
        endif()
    endif()

endfunction()

#==============================================================================

function (_yup_android_copy_sdl2_activity)
    set (JAVA_SOURCE_RELATIVE_FOLDER app/src/main/java)
    set (SOURCE_FOLDER ${CMAKE_BINARY_DIR}/externals/SDL2/android-project/${JAVA_SOURCE_RELATIVE_FOLDER}/org)
    file (COPY ${SOURCE_FOLDER} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${JAVA_SOURCE_RELATIVE_FOLDER})
endfunction()

#==============================================================================

# Function to add Java bytecode compilation to a YUP module
function (_yup_android_module_add_java_support module_name)
    _yup_message (STATUS "_yup_android_module_add_java_support called for module: ${module_name}")

    if (NOT YUP_PLATFORM_ANDROID)
        _yup_message (STATUS "Not Android platform, skipping Java support for ${module_name}")
        return()
    endif()

    _yup_message (STATUS "Android platform detected, checking for Java sources in ${module_name}")

    # Look for Java sources in the module's native/java directory
    get_target_property(module_path ${module_name} YUP_MODULE_PATH)
    _yup_message (STATUS "Module path for ${module_name}: ${module_path}")

    set (java_dir "${module_path}/native/java")
    _yup_message (STATUS "Looking for Java sources in: ${java_dir}")

    if (NOT EXISTS "${java_dir}")
        _yup_message (STATUS "Java directory does not exist: ${java_dir}")
        return()
    endif()

    # Find all Java source files
    file (GLOB_RECURSE java_sources "${java_dir}/*.java")
    if (NOT java_sources)
        _yup_message (STATUS "No Java source files found in: ${java_dir}")
        return()
    endif()

    _yup_message (STATUS "Found Java sources for ${module_name}: ${java_sources}")

    # Determine minimum SDK version from module or use default
    set (min_sdk 23)

    # Compile each Java class individually
    set (java_headers "")
    set (java_targets "")

    foreach (java_source ${java_sources})
        # Extract class name from file path
        get_filename_component (class_name "${java_source}" NAME_WE)

        # Generate individual class bytecode
        _yup_android_add_single_java_class (
            MODULE_NAME ${module_name}
            CLASS_NAME ${class_name}
            JAVA_SOURCE "${java_source}"
            MIN_SDK_VERSION ${min_sdk}
            OUTPUT_VARIABLE_NAME "java${class_name}Bytecode"
        )

        # Collect headers and targets
        if (${module_name}_${class_name}_JAVA_HEADER)
            list (APPEND java_headers "${${module_name}_${class_name}_JAVA_HEADER}")
        endif()

        if (${module_name}_${class_name}_JAVA_TARGET)
            list (APPEND java_targets "${${module_name}_${class_name}_JAVA_TARGET}")
        endif()
    endforeach()
endfunction()

#==============================================================================

# Function to compile a single Java class to DEX bytecode
function (_yup_android_add_single_java_class)
    _yup_message (STATUS "_yup_android_add_single_java_class called")

    cmake_parse_arguments (
        JAVA_ARG
        ""
        "MODULE_NAME;CLASS_NAME;JAVA_SOURCE;MIN_SDK_VERSION;OUTPUT_VARIABLE_NAME"
        ""
        ${ARGN}
    )

    _yup_message (STATUS "Processing Java class: ${JAVA_ARG_CLASS_NAME} for module: ${JAVA_ARG_MODULE_NAME}")
    _yup_message (STATUS "Java source file: ${JAVA_ARG_JAVA_SOURCE}")

    if (NOT DEFINED JAVA_ARG_MIN_SDK_VERSION)
        set (JAVA_ARG_MIN_SDK_VERSION 23)
    endif()

    if (NOT DEFINED JAVA_ARG_OUTPUT_VARIABLE_NAME)
        set (JAVA_ARG_OUTPUT_VARIABLE_NAME "java${JAVA_ARG_CLASS_NAME}Bytecode")
    endif()

    if (NOT JAVA_ARG_JAVA_SOURCE)
        _yup_message (WARNING "_yup_android_add_single_java_class: No Java source specified for ${JAVA_ARG_CLASS_NAME}")
        return()
    endif()

    # Set up paths
    set (java_build_dir "${CMAKE_CURRENT_BINARY_DIR}/java_build/${JAVA_ARG_MODULE_NAME}")
    set (classes_dir "${java_build_dir}/classes_${JAVA_ARG_CLASS_NAME}")
    set (dex_file "${java_build_dir}/classes.dex")  # d8 always creates classes.dex
    set (dex_gz_file "${java_build_dir}/${JAVA_ARG_CLASS_NAME}.dex.gz")
    set (output_header "${java_build_dir}/yup_${JAVA_ARG_CLASS_NAME}_bytecode.h")

    _yup_message (STATUS "Output header will be: ${output_header}")
    _yup_message (STATUS "Java build directory: ${java_build_dir}")

    # Find required tools
    find_program (JAVAC_EXECUTABLE javac REQUIRED)

    # Find Android SDK and build tools
    if (NOT DEFINED ENV{ANDROID_SDK_ROOT} AND NOT DEFINED ENV{ANDROID_HOME})
        _yup_message (FATAL_ERROR "ANDROID_SDK_ROOT or ANDROID_HOME must be set to compile Java bytecode")
    endif()

    if (DEFINED ENV{ANDROID_SDK_ROOT})
        set (ANDROID_SDK_ROOT "$ENV{ANDROID_SDK_ROOT}")
    else()
        set (ANDROID_SDK_ROOT "$ENV{ANDROID_HOME}")
    endif()

    _yup_message (STATUS "Using Android SDK: ${ANDROID_SDK_ROOT}")

    # Find the latest build tools version
    file (GLOB build_tools_versions "${ANDROID_SDK_ROOT}/build-tools/*")
    list (SORT build_tools_versions)
    list (REVERSE build_tools_versions)
    list (GET build_tools_versions 0 latest_build_tools)

    if (NOT EXISTS "${latest_build_tools}")
        _yup_message (FATAL_ERROR "Could not find Android build tools in ${ANDROID_SDK_ROOT}/build-tools/")
    endif()

    set (d8_executable "${latest_build_tools}/d8")
    if (WIN32)
        set (d8_executable "${d8_executable}.bat")
    endif()

    if (EXISTS "${d8_executable}")
        _yup_message (STATUS "Found ${d8_executable} tool in ${latest_build_tools}")
    else()
        _yup_message (FATAL_ERROR "Could not find ${d8_executable} tool in ${latest_build_tools}")
    endif()

    # Find Android platform jar
    set (android_jar "${ANDROID_SDK_ROOT}/platforms/android-${JAVA_ARG_MIN_SDK_VERSION}/android.jar")
    if (NOT EXISTS "${android_jar}")
        _yup_message (FATAL_ERROR "Could not find android.jar for API level ${JAVA_ARG_MIN_SDK_VERSION} at ${android_jar}")
    endif()

    # Create build directory
    file (MAKE_DIRECTORY "${classes_dir}")

    # Get class file name and path with package structure
    get_filename_component (source_name ${JAVA_ARG_JAVA_SOURCE} NAME_WE)

    # Extract package path from Java source file by reading the package declaration
    file (READ "${JAVA_ARG_JAVA_SOURCE}" java_content)
    string (REGEX MATCH "package[ \t]+([a-zA-Z0-9_.]+)" package_match "${java_content}")
    if (package_match)
        string (REGEX REPLACE "package[ \t]+([a-zA-Z0-9_.]+)" "\\1" package_name "${package_match}")
        string (REPLACE "." "/" package_path "${package_name}")
        set (class_file "${classes_dir}/${package_path}/${source_name}.class")
    else()
        set (class_file "${classes_dir}/${source_name}.class")
    endif()

    _yup_message (STATUS "Expected class file: ${class_file}")

    # Compile Java source immediately
    _yup_message (STATUS "Compiling Java source: ${JAVA_ARG_JAVA_SOURCE}")
    execute_process (
        COMMAND ${JAVAC_EXECUTABLE}
            -cp "${android_jar}"
            -d "${classes_dir}"
            -source 8
            -target 8
            "${JAVA_ARG_JAVA_SOURCE}"
        RESULT_VARIABLE javac_result
        OUTPUT_VARIABLE javac_output
        ERROR_VARIABLE javac_error)

    if (NOT javac_result EQUAL 0)
        _yup_message (FATAL_ERROR "Failed to compile Java source ${JAVA_ARG_JAVA_SOURCE}: ${javac_error}")
    endif()

    # Convert to DEX immediately
    file (GLOB_RECURSE class_files "${classes_dir}/*/*.class")
    _yup_message (STATUS "Converting to DEX: ${class_files}")
    execute_process (
        COMMAND ${d8_executable}
            --release
            --lib "${android_jar}"
            --min-api ${JAVA_ARG_MIN_SDK_VERSION}
            --output "${java_build_dir}"
            ${class_files}
        RESULT_VARIABLE dex_result
        OUTPUT_VARIABLE dex_output
        ERROR_VARIABLE dex_error)

    if (NOT dex_result EQUAL 0)
        _yup_message (FATAL_ERROR "Failed to convert to DEX: ${dex_error}")
    endif()

    # Compress DEX file immediately
    _yup_message (STATUS "Compressing DEX file: ${dex_file}")
    find_program (GZIP_EXECUTABLE gzip REQUIRED)
    execute_process (
        COMMAND ${GZIP_EXECUTABLE} -c "${dex_file}"
        OUTPUT_FILE "${dex_gz_file}"
        RESULT_VARIABLE gzip_result
        ERROR_VARIABLE gzip_error)

    if (NOT gzip_result EQUAL 0)
        _yup_message (FATAL_ERROR "Failed to compress DEX file: ${gzip_error}")
    endif()

    # Generate C++ header with bytecode immediately
    _yup_message (STATUS "Generating header immediately: ${output_header}")

    # Change to the build directory to avoid path issues
    execute_process (
        COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${dex_gz_file}
            -DOUTPUT_FILE=${output_header}
            -DVARIABLE_NAME=${JAVA_ARG_OUTPUT_VARIABLE_NAME}
            -DCLASS_NAME=${JAVA_ARG_CLASS_NAME}
            -DJAVA_SOURCE=${JAVA_ARG_JAVA_SOURCE}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/yup_generate_java_header.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE header_result
        OUTPUT_VARIABLE header_output
        ERROR_VARIABLE header_error)

    if (NOT header_result EQUAL 0)
        _yup_message (FATAL_ERROR "Failed to generate header: ${header_error}")
    endif()

    _yup_message (STATUS "Successfully generated header: ${output_header}")

    # Copy to module path
    get_target_property (module_path ${JAVA_ARG_MODULE_NAME} YUP_MODULE_PATH)
    execute_process (
        COMMAND ${CMAKE_COMMAND}
            -E copy
            "${output_header}"
            "${module_path}/native/generated"
        RESULT_VARIABLE copy_result
        ERROR_VARIABLE copy_error)

    if (NOT copy_result EQUAL 0)
        _yup_message (FATAL_ERROR "Failed to copy header: ${copy_error}")
    endif()

    _yup_message (STATUS "Successfully copied header: ${output_header} to ${module_path}/native/generated/${output_header}")

endfunction()
