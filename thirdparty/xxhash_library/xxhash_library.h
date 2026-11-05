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

    ID:               xxhash_library
    vendor:           Cyan4973
    version:          0.8.3
    name:             xxHash extremely fast hash algorithm
    description:      Extremely fast non-cryptographic hash algorithm, in a single header.
    website:          https://github.com/Cyan4973/xxHash
    repository:       https://github.com/Cyan4973/xxHash.git
    branch:           e626a72bc2321cd320e953a0ccf1584cad60f363
    submodules:       0
    license:          BSD-2-Clause

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

// Every xxHash function is inlined into the including translation unit, so no symbol is exported
#ifndef XXH_INLINE_ALL
#define XXH_INLINE_ALL
#endif

#include <xxhash_library/xxhash.h>
