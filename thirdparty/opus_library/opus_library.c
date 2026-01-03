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

#if ! NDEBUG
#define OPUS_WILL_BE_SLOW 1
#endif

#include "opus_library.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#pragma-messages"
#pragma clang diagnostic ignored "-Wnonnull"
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <opus_library/src/opus.c>
#include <opus_library/src/opus_decoder.c>
#include <opus_library/src/opus_encoder.c>
#include <opus_library/src/extensions.c>
#include <opus_library/src/opus_multistream.c>
#include <opus_library/src/opus_multistream_encoder.c>
#include <opus_library/src/opus_multistream_decoder.c>
#include <opus_library/src/repacketizer.c>
#include <opus_library/src/opus_projection_encoder.c>
#include <opus_library/src/opus_projection_decoder.c>
#include <opus_library/src/mapping_matrix.c>
#include <opus_library/src/analysis.c>
#include <opus_library/src/mlp.c>
#include <opus_library/src/mlp_data.c>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
