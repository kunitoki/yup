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

#include "libvorbis.h"

#include "libvorbis_include_pre.h"

#include <libvorbis/lib/analysis.c>
#include <libvorbis/lib/bitrate.c>
#include <libvorbis/lib/block.c>
#include <libvorbis/lib/envelope.c>
#include <libvorbis/lib/floor0.c>
#define FLOOR1_fromdB_LOOKUP FLOOR1_fromdB_LOOKUP_floor1_c
#include <libvorbis/lib/floor1.c>
#undef FLOOR1_fromdB_LOOKUP
#include <libvorbis/lib/info.c>
#include <libvorbis/lib/lookup.c>
#include <libvorbis/lib/lpc.c>
#include <libvorbis/lib/lsp.c>
#include <libvorbis/lib/mapping0.c>
#include <libvorbis/lib/mdct.c>
#include <libvorbis/lib/psy.c>
#include <libvorbis/lib/registry.c>
#include <libvorbis/lib/res0.c>
#include <libvorbis/lib/sharedbook.c>
#include <libvorbis/lib/smallft.c>
#include <libvorbis/lib/synthesis.c>
#include <libvorbis/lib/vorbisenc.c>
#include <libvorbis/lib/vorbisfile.c>
#include <libvorbis/lib/window.c>

#include "libvorbis_include_post.h"
