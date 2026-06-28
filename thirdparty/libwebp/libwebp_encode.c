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

#include "libwebp.h"
#include "libwebp_config.h"

// sharpyuv
#include "src/sharpyuv/sharpyuv_cpu.c"
#include "src/sharpyuv/sharpyuv_csp.c"
#define clip clip_sharpyuv_dsp
#include "src/sharpyuv/sharpyuv_dsp.c"
#undef clip
#define Shift Shift_gamma
#include "src/sharpyuv/sharpyuv_gamma.c"
#undef Shift
#include "src/sharpyuv/sharpyuv_neon.c"
#include "src/sharpyuv/sharpyuv_sse2.c"
#define clip_8b clip_8b_sharpyuv
#define clip clip_sharpyuv
#include "src/sharpyuv/sharpyuv.c"
#undef clip
#undef clip_8b
#undef YUV_FIX
#undef VP8GetCPUInfo

// encoder source files
#define NearLossless NearLossless_pixel
#define NearLosslessComponent NearLosslessComponent_pixel
#define NearLosslessDiff NearLosslessDiff_pixel
#define clip clip_predictor_enc
#include "src/enc/predictor_enc.c"
#undef clip
#undef NearLossless
#undef NearLosslessComponent
#undef NearLosslessDiff
#include "libwebp_undef.h"
#include "src/enc/near_lossless_enc.c"
#include "libwebp_undef.h"
#include "src/enc/frame_enc.c"
#include "libwebp_undef.h"
#include "src/enc/alpha_enc.c"
#include "libwebp_undef.h"
#include "src/enc/iterator_enc.c"
#include "libwebp_undef.h"
#define clip clip_analysis_enc
#include "src/enc/analysis_enc.c"
#undef clip
#include "libwebp_undef.h"
#include "src/enc/config_enc.c"
#include "libwebp_undef.h"
#include "src/enc/filter_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_csp_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_psnr_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_rescale_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_tools_enc.c"
#include "libwebp_undef.h"
#define kZigzag kZigzag_quant_enc
#include "src/enc/quant_enc.c"
#undef kZigzag
#include "libwebp_undef.h"
#include "src/enc/syntax_enc.c"
#include "libwebp_undef.h"
#include "src/enc/token_enc.c"
#include "libwebp_undef.h"
#include "src/enc/tree_enc.c"
#include "libwebp_undef.h"
#include "src/enc/vp8l_enc.c"
#include "libwebp_undef.h"
#include "src/enc/webp_enc.c"
#include "libwebp_undef.h"
#include "src/enc/cost_enc.c"
#include "libwebp_undef.h"
#include "src/enc/histogram_enc.c"
#include "libwebp_undef.h"
#include "src/enc/backward_references_enc.c"
#include "libwebp_undef.h"
#include "src/enc/backward_references_cost_enc.c"
#include "libwebp_undef.h"

// encoder DSP files
#define kZigzag kZigzag_enc
#define clip_8b clip_8b_enc
#include "src/dsp/enc.c"
#undef kZigzag
#undef clip_8b
#include "libwebp_undef.h"
#include "src/dsp/enc_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/enc_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/enc_sse41.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc_sse41.c"
#include "libwebp_undef.h"

// encoder utilities
#include "src/utils/huffman_encode_utils.c"
#include "libwebp_undef.h"
