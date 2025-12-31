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

#include "bitmath.c"
#include "bitreader.c"
#include "bitwriter.c"
#include "cpu.c"
#include "crc.c"
#include "fixed_intrin_avx2.c"
#include "fixed_intrin_sse2.c"
#include "fixed_intrin_sse42.c"
#include "fixed_intrin_ssse3.c"

#if defined(CHECK_ORDER_IS_VALID)
#undef CHECK_ORDER_IS_VALID
#endif

#include "fixed.c"
#include "float.c"
#include "format.c"
#include "lpc.c"
#include "lpc_intrin_avx2.c"
#include "lpc_intrin_fma.c"
#include "lpc_intrin_neon.c"
#include "lpc_intrin_sse2.c"
#include "lpc_intrin_sse41.c"
#include "md5.c"
#include "memory.c"
#include "stream_decoder.c"
#include "window.c"

//#include "metadata_iterators.c"
//#include "metadata_object.c"
//#include "ogg_decoder_aspect.c"
//#include "ogg_encoder_aspect.c"
//#include "ogg_helper.c"
//#include "ogg_mapping.c"

#include "flac_include_post.h"
