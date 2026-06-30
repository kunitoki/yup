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

#if defined (__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#endif

#include "upstream/png.c"
#include "upstream/pngerror.c"
#include "upstream/pngget.c"
#include "upstream/pngmem.c"
#include "upstream/pngread.c"

#define png_pass_start yup_pngpread_pass_start
#define png_pass_inc yup_pngpread_pass_inc
#define png_pass_ystart yup_pngpread_pass_ystart
#define png_pass_yinc yup_pngpread_pass_yinc
#include "upstream/pngpread.c"
#undef png_pass_start
#undef png_pass_inc
#undef png_pass_ystart
#undef png_pass_yinc

#include "upstream/pngrio.c"
#include "upstream/pngrtran.c"

#define png_pass_start yup_pngrutil_pass_start
#define png_pass_inc yup_pngrutil_pass_inc
#define png_pass_ystart yup_pngrutil_pass_ystart
#define png_pass_yinc yup_pngrutil_pass_yinc
#include "upstream/pngrutil.c"
#undef png_pass_start
#undef png_pass_inc
#undef png_pass_ystart
#undef png_pass_yinc

#include "upstream/pngset.c"
#include "upstream/pngtrans.c"
#include "upstream/pngwio.c"
#include "upstream/pngwrite.c"
#include "upstream/pngwtran.c"

#define png_pass_start yup_pngwutil_pass_start
#define png_pass_inc yup_pngwutil_pass_inc
#define png_pass_ystart yup_pngwutil_pass_ystart
#define png_pass_yinc yup_pngwutil_pass_yinc
#include "upstream/pngwutil.c"
#undef png_pass_start
#undef png_pass_inc
#undef png_pass_ystart
#undef png_pass_yinc

#if defined (__clang__)
 #pragma clang diagnostic pop
#endif

#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#include "upstream/arm/arm_init.c"
#include "upstream/arm/filter_neon_intrinsics.c"
#include "upstream/arm/palette_neon_intrinsics.c"
#endif

#if defined(__SSE2__) || defined(_M_X64) || defined(_M_IX86)
#include "upstream/intel/intel_init.c"
#include "upstream/intel/filter_sse2_intrinsics.c"
#endif
