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

#include "flac_library.h"

#include "flac_include_pre.h"

#include <libFLAC/bitmath.c>
#include <libFLAC/bitreader.c>
#include <libFLAC/bitwriter.c>
#include <libFLAC/cpu.c>
#include <libFLAC/crc.c>
#include <libFLAC/fixed.c>
#include <libFLAC/float.c>
#include <libFLAC/format.c>
#include <libFLAC/lpc.c>
#include <libFLAC/lpc_intrin_neon.c>
#include <libFLAC/md5.c>
#include <libFLAC/memory.c>
//#include <libFLAC/metadata_iterators.c>
//#include <libFLAC/metadata_object.c>
#include <libFLAC/stream_decoder.c>
#include <libFLAC/stream_encoder_framing.c>
#include <libFLAC/window.c>

#include "flac_include_post.h"
