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

function (_yup_prepare_gradle_android)
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
    _yup_set_default (YUP_ANDROID_CPP_VERSION "17")
    _yup_set_default (YUP_ANDROID_APPLICATION_ID "com.yup.default_app")
    _yup_set_default (YUP_ANDROID_APPLICATION_NAMESPACE "${YUP_ANDROID_APPLICATION_ID}")
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
        foreach (mipmap_directory mimpap-hdpi mipmap-mdpi mipmap-xhdpi mipmap-xxhdpi mipmap-xxxhdpi)
            file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/app/src/main/res/${mipmap_directory})
            file (COPY ${YUP_ANDROID_TARGET_ICON} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/app/src/main/res/${mipmap_directory}/ic_launcher.png)
        endforeach()
    endif()

endfunction()

#==============================================================================

function (_yup_copy_sdl2_activity_android)
    _yup_message (STATUS "Copying SDL2 activity to Android project")

    set (JAVA_SOURCE_RELATIVE_FOLDER app/src/main/java)
    set (SOURCE_FOLDER ${CMAKE_BINARY_DIR}/externals/SDL2/android-project/${JAVA_SOURCE_RELATIVE_FOLDER}/org)
    file (COPY ${SOURCE_FOLDER} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${JAVA_SOURCE_RELATIVE_FOLDER})
endfunction()
