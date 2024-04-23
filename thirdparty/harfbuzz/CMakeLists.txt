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

cmake_minimum_required(VERSION 3.28)

set (target_name "harfbuzz")
set (dependencies "")
set (public_definitions "")
set (public_includes "")
set (public_libs "")
set (private_definitions "HAVE_ATEXIT=1") # HAVE_FREETYPE=1
set (private_includes "")
set (private_libs "")

yup_add_target (${target_name} ${CMAKE_CURRENT_LIST_DIR}
    "${dependencies}"
    "${public_definitions}"
    "${public_includes}"
    "${public_libs}"
    "${private_definitions}"
    "${private_includes}"
    "${private_libs}")
