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

#if defined(__BYTE_ORDER) && defined(__BIG_ENDIAN)
#if __BYTE_ORDER == __BIG_ENDIAN
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif
#elif defined(_BIG_ENDIAN) || defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__MIPSEB__)
#define IS_BIG_ENDIAN 1
#else
#define IS_BIG_ENDIAN 0
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__x86_64__) || defined(_M_X64)
#define IS_INTEL_64 1
#else
#define IS_INTEL_64 0
#endif
#endif

#undef USE_DITHERING
#undef WEBP_HAVE_GIF
#undef WEBP_HAVE_GL
#undef WEBP_HAVE_JPEG
#undef WEBP_HAVE_NEON_RTCD
#undef WEBP_HAVE_PNG
#undef WEBP_HAVE_SDL
#undef WEBP_HAVE_TIFF
#undef WEBP_USE_THREAD
#undef HAVE_GLUT_GLUT_H
#undef HAVE_OPENGL_GLUT_H

#undef HAVE_CONFIG_H
#define HAVE_CONFIG_H 1

#undef WEBP_DISABLE_STATS
#define WEBP_DISABLE_STATS 1

#undef AC_APPLE_UNIVERSAL_BUILD
#if (__APPLE__) && __intel__
#define AC_APPLE_UNIVERSAL_BUILD 1
#endif

#undef HAVE_BUILTIN_BSWAP16
#undef HAVE_BUILTIN_BSWAP32
#undef HAVE_BUILTIN_BSWAP64
#if ! _MSC_VER
#define HAVE_BUILTIN_BSWAP16 1
#define HAVE_BUILTIN_BSWAP32 1
#define HAVE_BUILTIN_BSWAP64 1
#endif

#undef HAVE_CPU_FEATURES_H
#define HAVE_CPU_FEATURES_H 1

#undef HAVE_DLFCN_H
#define HAVE_DLFCN_H 1

#undef HAVE_GL_GLUT_H
#define HAVE_GL_GLUT_H 1

#undef HAVE_INTTYPES_H
#define HAVE_INTTYPES_H 1

#undef HAVE_MEMORY_H
#define HAVE_MEMORY_H 1

#undef HAVE_PTHREAD_PRIO_INHERIT
#define HAVE_PTHREAD_PRIO_INHERIT (! _MSC_VER)

#undef HAVE_SHLWAPI_H
#define HAVE_SHLWAPI_H 1

#undef HAVE_STDINT_H
#define HAVE_STDINT_H 1

#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1

#undef HAVE_STRINGS_H
#define HAVE_STRINGS_H 1

#undef HAVE_STRING_H
#define HAVE_STRING_H 1

#undef HAVE_SYS_STAT_H
#undef HAVE_SYS_TYPES_H
#undef HAVE_UNISTD_H

#if ! _MSC_VER
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#endif

#undef HAVE_WINCODEC_H
#if _MSC_VER
#define HAVE_WINCODEC_H 1
#endif

#undef HAVE_WINDOWS_H
#if _MSC_VER
#define HAVE_WINDOWS_H 1
#endif

#undef STDC_HEADERS
#define STDC_HEADERS 1

#undef WEBP_HAVE_NEON
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#define WEBP_HAVE_NEON 1
#endif

#undef WEBP_HAVE_SSE2
#if IS_INTEL_64
#define WEBP_HAVE_SSE2 1
#endif

#undef WEBP_HAVE_SSE41
#if IS_INTEL_64 && _MSC_VER
// #define WEBP_HAVE_SSE41 1
#endif

#undef WEBP_NEAR_LOSSLESS
#define WEBP_NEAR_LOSSLESS 1

#undef WORDS_BIGENDIAN
#if IS_BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif

#undef LT_OBJDIR
#define LT_OBJDIR ""

#undef PACKAGE
#define PACKAGE "WebP"
#undef PACKAGE_NAME
#define PACKAGE_NAME PACKAGE
#undef PACKAGE_TARNAME
#define PACKAGE_TARNAME PACKAGE

#undef PACKAGE_VERSION
#define PACKAGE_VERSION "1.4.0"
#undef VERSION
#define VERSION PACKAGE_VERSION

#undef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT "https://bugs.chromium.org/p/webp"

#undef PACKAGE_STRING
#define PACKAGE_STRING (PACKAGE_NAME " " PACKAGE_VERSION)

#undef PACKAGE_URL
#define PACKAGE_URL "http://developers.google.com/speed/webp"

#ifndef YUP_WEBP_ALLOW_ENCODER
#define YUP_WEBP_ALLOW_ENCODER 0
#endif

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

#if YUP_WEBP_ALLOW_ENCODER
#include "src/enc/frame_enc.c"
#include "libwebp_undef.h"
#include "src/enc/alpha_enc.c"
#include "libwebp_undef.h"
#include "src/enc/iterator_enc.c"
#include "libwebp_undef.h"
#include "src/enc/analysis_enc.c"
#include "libwebp_undef.h"
#include "src/enc/config_enc.c"
#include "libwebp_undef.h"
#include "src/enc/filter_enc.c"
#include "libwebp_undef.h"
#include "src/enc/predictor_enc.c"
#include "libwebp_undef.h"
#include "src/enc/near_lossless_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_csp_enc.c"
#include "libwebp_undef.h"
#include "src/enc/picture_tools_enc.c"
#include "libwebp_undef.h"
#include "src/enc/quant_enc.c"
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
#endif

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
#include "src/dsp/ssim_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_sse41.c"
#include "libwebp_undef.h"
#include "src/dsp/dec_clip_tables.c"
#if YUP_WEBP_ALLOW_ENCODER
#include "src/dsp/enc.c"
#include "libwebp_undef.h"
#include "src/dsp/enc_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/enc_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/enc_sse41.c"
#include "libwebp_undef.h"
#endif
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
#if YUP_WEBP_ALLOW_ENCODER
#include "src/dsp/lossless_enc.c"
#include "libwebp_undef.h"
#endif
#include "src/dsp/lossless_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_sse2.c"
#include "libwebp_undef.h"
#if YUP_WEBP_ALLOW_ENCODER
#include "src/dsp/lossless_enc_neon.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc_sse2.c"
#include "libwebp_undef.h"
#include "src/dsp/lossless_enc_sse41.c"
#include "libwebp_undef.h"
#endif
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
#if YUP_WEBP_ALLOW_ENCODER
#include "src/utils/huffman_encode_utils.c"
#endif
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
#include "src/utils/quant_levels_dec_utils.c"
#include "libwebp_undef.h"
