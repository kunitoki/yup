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

#include "libwebp_undef.h"
#include "src/webp/decode.h"
#include "libwebp_undef.h"
#include "src/dec/common_dec.h"
#include "libwebp_undef.h"

/* Split from libwebp_decode.c — idec_dec.c defines its own MemBuffer, MemDataSize,
   RemapMemBuffer etc. which conflict with demux.c's identically-named types. */

#define DecoderState DecoderState_idec
#include "src/dec/idec_dec.c"
#undef DecoderState
