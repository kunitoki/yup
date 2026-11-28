# ==============================================================================
#
#   This file is part of the YUP library.
#   Copyright (c) 2026 - kunitoki@gmail.com
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

# Initial cache of the CI builds using ccache, loaded with `cmake -C`: it keeps what they store small.

# Debug builds keep the line tables only: backtraces still show files and lines, objects are much smaller
foreach (language IN ITEMS C CXX OBJC OBJCXX)
    set (CMAKE_${language}_FLAGS_DEBUG "-g1" CACHE STRING "Flags used by the ${language} compiler during DEBUG builds.")
endforeach()

# ccache can't cache the sources using a precompiled header (the ones of SDL), and would store the header as well
set (CMAKE_DISABLE_PRECOMPILE_HEADERS ON CACHE BOOL "Disable precompiled headers.")
