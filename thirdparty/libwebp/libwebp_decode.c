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

#include "libwebp.h"
#include "libwebp_config.h"

#include "libwebp_undef.h"
#include "src/webp/decode.h"
#include "libwebp_undef.h"
#include "src/webp/encode.h"
#include "libwebp_undef.h"
#include "src/webp/demux.h"
#include "libwebp_undef.h"
#include "src/dsp/dsp.h"
#include "libwebp_undef.h"
#include "src/dec/common_dec.h"
#include "libwebp_undef.h"

#include "src/dec/alpha_dec.c"
#include "libwebp_undef.h"
#include "src/dec/buffer_dec.c"
#include "libwebp_undef.h"
#include "src/dec/frame_dec.c"
#include "libwebp_undef.h"
#include "src/dec/io_dec.c"
#include "libwebp_undef.h"
#include "src/dec/quant_dec.c"
#include "libwebp_undef.h"
#include "src/dec/tree_dec.c"
#include "libwebp_undef.h"
#include "src/dec/vp8_dec.c"
#include "libwebp_undef.h"
#include "src/dec/vp8l_dec.c"
#include "libwebp_undef.h"
#include "src/dec/webp_dec.c"
#include "libwebp_undef.h"

#define ParseVP8X ParseVP8X_DEMUX
#include "src/demux/demux.c"
#undef ParseVP8X
#include "libwebp_undef.h"
#include "src/dsp/alpha_processing.c"
#include "libwebp_undef.h"
#include "src/dsp/cpu.c"
#include "libwebp_undef.h"
#include "src/dsp/cost.c"
#include "libwebp_undef.h"
#include "src/dsp/cost_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/cost_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/dec.c"
#include "libwebp_undef.h"
#include "src/dsp/ssim.c"
#include "libwebp_undef.h"
#define kWeight kWeightSSE2
#include "src/dsp/ssim_sse2.c"
#undef kWeight
#include "libwebp_undef.h"
#include "src/dsp/dec_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_sse41.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_clip_tables.c"
#include "src/dsp/filters.c"
#include "libwebp_undef.h"
#define GradientPredictor_C GradientPredictor_C_NEON
#include "src/dsp/filters_neon.c"
#undef GradientPredictor_C
#include "libwebp_undef.h"
#include "src/dsp/filters_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/rescaler.c"
#include "libwebp_undef.h"
#include "src/dsp/rescaler_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/rescaler_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/upsampling.c"
#include "libwebp_undef.h"
#include "src/dsp/upsampling_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/upsampling_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/upsampling_sse41.c"
#include "libwebp_undef.h"
#include "src/dsp/yuv.c"
#include "libwebp_undef.h"
#include "src/dsp/yuv_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/yuv_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/yuv_sse41.c"
#include "libwebp_undef.h"
#include "src/dsp/alpha_processing_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/alpha_processing_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/alpha_processing_sse41.c"
#include "libwebp_undef.h"
#include "src/utils/bit_reader_utils.c"
#include "libwebp_undef.h"
#include "src/utils/bit_writer_utils.c"
#include "libwebp_undef.h"
#include "src/utils/color_cache_utils.c"
#include "libwebp_undef.h"
#include "src/utils/filters_utils.c"
#include "libwebp_undef.h"
#include "src/utils/huffman_utils.c"
#include "libwebp_undef.h"
#include "src/utils/quant_levels_utils.c"
#include "libwebp_undef.h"
#include "src/utils/rescaler_utils.c"
#include "libwebp_undef.h"
#include "src/utils/random_utils.c"
#include "libwebp_undef.h"
#include "src/utils/thread_utils.c"
#include "libwebp_undef.h"
#include "src/utils/palette.c"
#include "libwebp_undef.h"
#include "src/utils/utils.c"
#include "libwebp_undef.h"
#define clip_8b clip_8b_quant
#include "src/utils/quant_levels_dec_utils.c"
#undef clip_8b
#include "libwebp_undef.h"
