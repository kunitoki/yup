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

    ID:               zlib
    vendor:           zlib
    version:          1.2.3
    name:             Interface of the 'zlib' general purpose compression library
    description:      Interface of the 'zlib' general purpose compression library.
    website:          https://www.zlib.net/
    license:          Public Domain

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#if defined (__APPLE__)
#include <TargetConditionals.h>
#endif

#if __cplusplus
extern "C" {
#endif

#define ZLIB_INTERNAL
#define NO_DUMMY_DECL
#include "src/zlib.h"

#if __cplusplus
} // extern "C"
#endif
