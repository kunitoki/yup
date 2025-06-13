/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

    ID:                 rive_decoders
    vendor:             rive
    version:            1.0
    name:               Rive Decoders.
    description:        The Rive Decoders is a companion library for ratser image decoding.
    website:            https://github.com/rive-app/rive-runtime
    license:            MIT

    searchpaths:        include
    appleFrameworks:    ImageIO
    enableARC:          1

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if YUP_MODULE_AVAILABLE_libpng
#define RIVE_PNG 1
#endif

#if YUP_MODULE_AVAILABLE_libwebp
#define RIVE_WEBP 1
#endif

#include "include/rive/decoders/bitmap_decoder.hpp"
