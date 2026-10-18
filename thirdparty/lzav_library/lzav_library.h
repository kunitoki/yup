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

    ID:               lzav_library
    vendor:           avaneev
    version:          5.18
    name:             LZAV fast in-memory data compression
    description:      Fast general-purpose in-memory LZ77 data compression and decompression, in a single header.
    website:          https://github.com/avaneev/lzav
    repository:       https://github.com/avaneev/lzav.git
    branch:           539f189193e1e81cddcccf554c8fe17dc107e876
    submodules:       0
    license:          MIT

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// Keeps the LZAV symbols in the lzav namespace only, out of the global one
#ifndef LZAV_NS_CUSTOM
#define LZAV_NS_CUSTOM lzav
#endif

#include <lzav_library/lzav.h>
