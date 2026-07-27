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

#include "flac_include_pre.h"

#include <flac_library/src/libFLAC/bitmath.c>
#include <flac_library/src/libFLAC/bitreader.c>
#include <flac_library/src/libFLAC/bitwriter.c>
#include <flac_library/src/libFLAC/cpu.c>
#include <flac_library/src/libFLAC/crc.c>
#include <flac_library/src/libFLAC/fixed_intrin_avx2.c>
#include <flac_library/src/libFLAC/fixed_intrin_sse2.c>
#include <flac_library/src/libFLAC/fixed_intrin_sse42.c>
#include <flac_library/src/libFLAC/fixed_intrin_ssse3.c>

#if defined(CHECK_ORDER_IS_VALID)
#undef CHECK_ORDER_IS_VALID
#endif

#include <flac_library/src/libFLAC/fixed.c>
#include <flac_library/src/libFLAC/float.c>
#include <flac_library/src/libFLAC/format.c>
#include <flac_library/src/libFLAC/lpc.c>
#include <flac_library/src/libFLAC/lpc_intrin_avx2.c>
#include <flac_library/src/libFLAC/lpc_intrin_fma.c>
#include <flac_library/src/libFLAC/lpc_intrin_neon.c>
#include <flac_library/src/libFLAC/lpc_intrin_sse2.c>
#include <flac_library/src/libFLAC/lpc_intrin_sse41.c>
#include <flac_library/src/libFLAC/md5.c>
#include <flac_library/src/libFLAC/memory.c>
#include <flac_library/src/libFLAC/stream_decoder.c>
#include <flac_library/src/libFLAC/window.c>

#include "flac_include_post.h"
