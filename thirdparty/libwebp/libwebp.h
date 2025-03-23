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

  BEGIN_JUCE_MODULE_DECLARATION

    ID:               libwebp
    vendor:           libwebp
    version:          1.4.0
    name:             WebP codec is a library to encode and decode images in WebP format
    description:      WebP codec is a library to encode and decode images in WebP format.
    website:          https://developers.google.com/speed/webp
    license:          MIT

    searchpaths:      upstream upstream/src

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "src/webp/types.h"
#include "src/webp/mux.h"
#include "src/webp/demux.h"
#include "src/webp/decode.h"
#include "src/webp/encode.h"

#ifdef __cplusplus
} // extern "C"
#endif
