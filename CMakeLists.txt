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

cmake_minimum_required (VERSION 3.28)

include (cmake/yup.cmake)
_yup_get_project_version_string (${CMAKE_CURRENT_LIST_DIR}/modules yup_version)
_yup_message (STATUS "Building project version ${yup_version}")

if (YUP_PLATFORM_MAC)
    set (CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
    set (CMAKE_XCODE_GENERATE_SCHEME OFF)
endif()

project (yup VERSION ${yup_version})
set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# Options
option (YUP_TARGET_ANDROID "Target Android project" OFF)
option (YUP_TARGET_ANDROID_BUILD_GRADLE "When building for Android, build the gradle infrastructure" OFF)
option (YUP_EXPORT_MODULES "Export the modules to the parent project" ON)
option (YUP_ENABLE_PROFILING "Enable the profiling code using Perfetto SDK" OFF)
option (YUP_ENABLE_COVERAGE "Enable code coverage collection for tests" OFF)
option (YUP_ENABLE_PLUGINVAL "Enable pluginval validation for VST3 plugins" OFF)
option (YUP_ENABLE_STATIC_PYTHON_LIBS "Use static Python libraries" OFF)
option (YUP_BUILD_JAVA_SUPPORT "Build the Java support" OFF)
option (YUP_BUILD_EXAMPLES "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option (YUP_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option (YUP_BUILD_WHEEL "Build the wheel" OFF)

# Dependencies modules
if (YUP_EXPORT_MODULES)
    _yup_message (STATUS "Exporting modules")

    if (YUP_PLATFORM_DESKTOP)
        _yup_message (STATUS "Enabling python module support")
        set (enable_python ON)
        if (YUP_PLATFORM_LINUX)
            set (YUP_ENABLE_STATIC_PYTHON_LIBS OFF)
        endif()
    else()
        _yup_message (STATUS "Disabling python module support")
        set (enable_python OFF)
    endif()

    yup_add_default_modules (${CMAKE_CURRENT_LIST_DIR} ENABLE_PYTHON ${enable_python})
endif()

# Enable profiling
if (YUP_ENABLE_PROFILING)
    _yup_message (STATUS "Enabling profiling")
    _yup_fetch_perfetto()
endif()

# Setup pluginval
if (YUP_ENABLE_PLUGINVAL)
    _yup_message (STATUS "Setting up pluginval")
    yup_setup_pluginval()
endif()

# Targets
if (YUP_BUILD_EXAMPLES)
    _yup_message (STATUS "Building examples")
    add_subdirectory (examples/console)
    add_subdirectory (examples/app)
    add_subdirectory (examples/graphics)
    if (YUP_PLATFORM_DESKTOP)
        add_subdirectory (examples/plugin)
    endif()
endif()

if (YUP_BUILD_TESTS AND NOT YUP_PLATFORM_MOBILE)
    _yup_message (STATUS "Building tests")
    add_subdirectory (tests)
endif()

if (YUP_BUILD_WHEEL)
    _yup_message (STATUS "Building wheel")
    add_subdirectory (python)
endif()
