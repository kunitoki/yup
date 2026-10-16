# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

# Builds a host tool at build time, with the host compiler, and copies its executable to OUTPUT.
#
#   cmake -DSOURCE_DIR=<dir> -DBUILD_DIR=<dir> -DEXECUTABLE_NAME=<name> -DOUTPUT=<path> -P yup_build_host_tool.cmake
#
# The copy only happens when the executable changed, so whatever depends on OUTPUT rebuilds only then.

foreach (required_variable SOURCE_DIR BUILD_DIR EXECUTABLE_NAME OUTPUT)
    if (NOT DEFINED ${required_variable})
        message (FATAL_ERROR "yup_build_host_tool: ${required_variable} is required")
    endif()
endforeach()

# The outer build may target another platform: never leak its compilers or SDK into the host build
foreach (variable SDKROOT CC CXX CFLAGS CXXFLAGS LDFLAGS CPATH LIBRARY_PATH IPHONEOS_DEPLOYMENT_TARGET MACOSX_DEPLOYMENT_TARGET)
    unset (ENV{${variable}})
endforeach()

if (NOT EXISTS "${BUILD_DIR}/CMakeCache.txt")
    execute_process (
        COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=
        RESULT_VARIABLE result)

    if (NOT result EQUAL 0)
        message (FATAL_ERROR "yup_build_host_tool: failed to configure ${SOURCE_DIR}")
    endif()
endif()

execute_process (
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config Release --parallel
    RESULT_VARIABLE result)

if (NOT result EQUAL 0)
    message (FATAL_ERROR "yup_build_host_tool: failed to build ${SOURCE_DIR}")
endif()

file (GLOB_RECURSE candidate_executables "${BUILD_DIR}/${EXECUTABLE_NAME}")
list (FILTER candidate_executables EXCLUDE REGEX "\\.dSYM/")

if (NOT candidate_executables)
    message (FATAL_ERROR "yup_build_host_tool: ${EXECUTABLE_NAME} not found in ${BUILD_DIR}")
endif()

list (GET candidate_executables 0 executable)
file (COPY_FILE "${executable}" "${OUTPUT}" ONLY_IF_DIFFERENT)
