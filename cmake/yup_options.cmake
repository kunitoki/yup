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

option (YUP_TARGET_ANDROID "Target Android project" OFF)
option (YUP_TARGET_ANDROID_BUILD_GRADLE "When building for Android, build the gradle infrastructure" OFF)
option (YUP_ENABLE_PROFILING "Enable the profiling code using Perfetto SDK" OFF)
option (YUP_ENABLE_COVERAGE "Enable code coverage collection for tests" OFF)
option (YUP_BUILD_JAVA_SUPPORT "Build the Java support" OFF)
option (YUP_BUILD_EXAMPLES "Build the examples" ${PROJECT_IS_TOP_LEVEL})
option (YUP_BUILD_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL})
option (YUP_BUILD_WHEEL "Build the wheel" OFF)
