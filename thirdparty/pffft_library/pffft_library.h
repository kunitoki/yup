/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

    ID:               pffft_library
    vendor:           Julien Pommier & Others
    version:          1.2.3
    name:             A pretty fast FFT and fast convolution with PFFASTCONV
    description:      A pretty fast FFT and fast convolution with PFFASTCONV.
    website:          https://github.com/marton78/pffft
    license:          BSD

    defines:          PFFFT_ENABLE_FLOAT=1 PFFFT_ENABLE_DOUBLE=1 PFFFT_ENABLE_NEON=1 _USE_MATH_DEFINES=1
 
  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include "upstream/pffft.h"
