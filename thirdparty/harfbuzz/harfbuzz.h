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

/*
  ==============================================================================

  BEGIN_JUCE_MODULE_DECLARATION

    ID:               harfbuzz
    vendor:           harfbuzz
    version:          1.0.0
    name:             Harfbuzz Text Shaping Engine
    description:      HarfBuzz is a text shaping engine.
    website:          https://github.com/harfbuzz/harfbuzz
    license:          MIT

    defines:          HAVE_ATEXIT=1 HB_ONLY_ONE_SHAPER HAVE_OT HB_NO_FALLBACK_SHAPE HB_NO_FALLBACK_SHAPE HB_NO_WIN1256 HB_NO_EXTERN_HELPERS HB_DISABLE_DEPRECATED HB_NO_COLOR HB_NO_BITMAP HB_NO_BUFFER_SERIALIZE HB_NO_BUFFER_VERIFY HB_NO_BUFFER_MESSAGE HB_NO_SETLOCALE HB_NO_STYLE HB_NO_VERTICAL HB_NO_LAYOUT_COLLECT_GLYPHS HB_NO_LAYOUT_RARELY_USED HB_NO_LAYOUT_UNUSED HB_NO_OT_FONT_GLYPH_NAMES HB_NO_PAINT HB_NO_MMAP HB_NO_META
    windowsOptions:   /bigobj
    searchpaths:      upstream

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include "upstream/hb.h"
#include "upstream/hb-ot.h"
