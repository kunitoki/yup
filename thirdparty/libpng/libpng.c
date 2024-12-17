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

#include "libpng.h"

#include "upstream/png.c"
#include "upstream/pngerror.c"
#include "upstream/pngget.c"
#include "upstream/pngmem.c"
#include "upstream/pngread.c"
#include "upstream/pngpread.c"
#include "upstream/pngrio.c"
#include "upstream/pngrtran.c"
#include "upstream/pngrutil.c"
#include "upstream/pngset.c"
#include "upstream/pngtrans.c"
#include "upstream/pngwio.c"
#include "upstream/pngwrite.c"
#include "upstream/pngwtran.c"
#include "upstream/pngwutil.c"

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include "upstream/arm/arm_init.c"
#include "upstream/arm/filter_neon_intrinsics.c"
#include "upstream/arm/palette_neon_intrinsics.c"
#endif

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#include "upstream/intel/intel_init.c"
#include "upstream/intel/filter_sse2_intrinsics.c"
#endif
