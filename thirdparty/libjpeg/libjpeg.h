/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:               libjpeg
    vendor:           libjpeg-turbo
    version:          3.1.4.1
    name:             libjpeg-turbo JPEG codec library
    description:      libjpeg-turbo is a JPEG image codec that uses SIMD instructions to accelerate baseline JPEG compression and decompression.
    website:          https://libjpeg-turbo.org/
    upstream:         https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.1.4.1.tar.gz
    license:          IJG/BSD-3-Clause/Zlib

    searchpaths:      src

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YUP_LIBJPEG_TURBO_8BIT_ONLY 1
#include "jpeglib.h"
#undef YUP_LIBJPEG_TURBO_8BIT_ONLY

#ifdef __cplusplus
} // extern "C"
#endif
