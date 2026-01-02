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

#define OPUS_BUILD 1
#define USE_ALLOCA 1

#include "opus_library.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#define CELT_ENCODER_C 1
#define CELT_DECODER_C 1
#include "opus_custom.h"
#undef CELT_ENCODER_C
#undef CELT_DECODER_C

#include "celt/bands.c"
#include "celt/celt.c"
#include "celt/celt_encoder.c"
#include "celt/celt_decoder.c"
#include "celt/cwrs.c"
#include "celt/entcode.c"
#include "celt/entdec.c"
#include "celt/entenc.c"
#include "celt/kiss_fft.c"
#include "celt/laplace.c"
#include "celt/mathops.c"
#include "celt/mdct.c"
#include "celt/modes.c"
#include "celt/pitch.c"
#include "celt/celt_lpc.c"
#include "celt/quant_bands.c"
#include "celt/rate.c"
#include "celt/vq.c"

#if defined(OPUS_ARM_PRESUME_NEON_INTR)
#include "celt/arm/pitch_neon_intr.c"
#include "celt/arm/celt_neon_intr.c"
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
