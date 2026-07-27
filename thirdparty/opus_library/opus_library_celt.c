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

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#define CELT_ENCODER_C 1
#define CELT_DECODER_C 1
#include <opus_custom.h>
#undef CELT_ENCODER_C
#undef CELT_DECODER_C

#include <opus_library/celt/bands.c>
#include <opus_library/celt/celt.c>
#include <opus_library/celt/celt_encoder.c>
#include <opus_library/celt/celt_decoder.c>
#include <opus_library/celt/cwrs.c>
#include <opus_library/celt/entcode.c>
#include <opus_library/celt/entdec.c>
#include <opus_library/celt/entenc.c>
#include <opus_library/celt/kiss_fft.c>
#include <opus_library/celt/laplace.c>
#include <opus_library/celt/mathops.c>
#include <opus_library/celt/mdct.c>
#include <opus_library/celt/modes.c>
#include <opus_library/celt/pitch.c>
#include <opus_library/celt/celt_lpc.c>
#include <opus_library/celt/quant_bands.c>
#include <opus_library/celt/rate.c>
#include <opus_library/celt/vq.c>

#if defined(OPUS_ARM_PRESUME_NEON_INTR)
#include <opus_library/celt/arm/pitch_neon_intr.c>
#include <opus_library/celt/arm/celt_neon_intr.c>
#endif

#if __clang__
#pragma clang diagnostic pop
#endif
