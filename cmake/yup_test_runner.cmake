# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2025 - kunitoki@gmail.com
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

include_guard (GLOBAL)

# The directory holding the reusable GoogleTest runner (yup::runGoogleTests and the
# YUP_IMPLEMENT_GTEST_MAIN macro). The runner is compiled by each test executable,
# so it is shipped as source rather than as a prebuilt module.
set (YUP_TEST_RUNNER_DIR "${CMAKE_CURRENT_LIST_DIR}/tools/test_runner" CACHE INTERNAL "")
set (YUP_TEST_RUNNER_SOURCE "${YUP_TEST_RUNNER_DIR}/yup_test_runner.cpp" CACHE INTERNAL "")

# ==============================================================================
# Adds YUP's shared GoogleTest runner to a test executable.
#
#   yup_add_test_runner (<target>)
#
# This compiles the runner into <target>, exposes the <yup_test_runner.h> header
# on its include path, and links the GoogleTest / GoogleMock targets when they are
# available. Define the entry point in one of the target's translation units with
# YUP_IMPLEMENT_GTEST_MAIN(), or call yup::runGoogleTests() from a custom main. The
# target must already link the relevant YUP modules (yup_gui on desktop, yup_events
# on Emscripten).
function (yup_add_test_runner target_name)
    if (NOT TARGET ${target_name})
        message (FATAL_ERROR "yup_add_test_runner: '${target_name}' is not a target")
    endif()

    target_sources (${target_name} PRIVATE "${YUP_TEST_RUNNER_SOURCE}")
    target_include_directories (${target_name} PRIVATE "${YUP_TEST_RUNNER_DIR}")

    # Link GoogleTest / GoogleMock, preferring the namespaced aliases when present.
    if (TARGET GTest::gmock)
        target_link_libraries (${target_name} PRIVATE GTest::gmock)
    elseif (TARGET gmock)
        target_link_libraries (${target_name} PRIVATE gmock)
    endif()

    if (TARGET GTest::gtest)
        target_link_libraries (${target_name} PRIVATE GTest::gtest)
    elseif (TARGET gtest)
        target_link_libraries (${target_name} PRIVATE gtest)
    endif()
endfunction()
