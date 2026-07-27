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

#include "flac_library/src/libFLAC/stream_encoder_framing.c"
#include "flac_library/src/libFLAC/stream_encoder_intrin_avx2.c"
#include "flac_library/src/libFLAC/stream_encoder_intrin_sse2.c"
#include "flac_library/src/libFLAC/stream_encoder_intrin_ssse3.c"
#include "flac_library/src/libFLAC/stream_encoder.c"

#if _MSC_VER
#include "flac_library/src/share/win_utf8_io/win_utf8_io.c"
#endif

#include "flac_include_post.h"
