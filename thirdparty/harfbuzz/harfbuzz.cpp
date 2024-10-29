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

#include "harfbuzz.h"

#include "upstream/hb-directwrite.cc"
#include "upstream/hb-subset-plan.cc"
#include "upstream/hb-ucd.cc"
#include "upstream/hb-coretext.cc"
#include "upstream/hb-ot-shaper-syllabic.cc"
#include "upstream/hb-ot-var.cc"
#include "upstream/hb-ot-map.cc"
#include "upstream/hb-icu.cc"
#include "upstream/hb-blob.cc"
#include "upstream/hb-draw.cc"
#include "upstream/hb-ot-shape-normalize.cc"
#include "upstream/hb-unicode.cc"
#include "upstream/hb-ot-shaper-thai.cc"
#include "upstream/hb-face-builder.cc"
#include "upstream/hb-ot-color.cc"
#include "upstream/hb-face.cc"
#include "upstream/hb-ot-cff1-table.cc"
#include "upstream/hb-paint.cc"
#include "upstream/hb-subset-cff1.cc"
#include "upstream/hb-subset-instancer-solver.cc"
#include "upstream/hb-ot-font.cc"
#include "upstream/hb-subset.cc"
#include "upstream/hb-common.cc"
#include "upstream/hb-buffer-serialize.cc"
#include "upstream/hb-subset-cff-common.cc"
#include "upstream/hb-fallback-shape.cc"
#include "upstream/hb-uniscribe.cc"
#include "upstream/graph/gsubgpos-context.cc"
#include "upstream/hb-ot-shaper-khmer.cc"
#include "upstream/hb-paint-extents.cc"
#include "upstream/hb-static.cc"
#include "upstream/hb-subset-repacker.cc"
#include "upstream/hb-style.cc"
#include "upstream/hb-ot-tag.cc"
#include "upstream/hb-glib.cc"
#include "upstream/hb-ot-shaper-myanmar.cc"
#include "upstream/hb-ft.cc"
#include "upstream/hb-ot-shape-fallback.cc"
#include "upstream/hb-gobject-structs.cc"
#include "upstream/hb-buffer.cc"
#include "upstream/hb-wasm-api.cc"
#include "upstream/hb-number.cc"
#include "upstream/hb-ot-cff2-table.cc"
#include "upstream/hb-shape-plan.cc"
#include "upstream/hb-outline.cc"
#include "upstream/hb-ot-metrics.cc"
#include "upstream/hb-ot-shaper-hangul.cc"
#include "upstream/hb-ot-shaper-use.cc"
#include "upstream/hb-ot-shaper-indic.cc"
#include "upstream/hb-wasm-shape.cc"
#include "upstream/hb-ot-shaper-indic-table.cc"
#include "upstream/hb-ot-layout.cc"
#include "upstream/hb-ot-shaper-default.cc"
#include "upstream/hb-ot-shape.cc"
#include "upstream/hb-map.cc"
#include "upstream/hb-font.cc"
#include "upstream/hb-graphite2.cc"
#include "upstream/hb-cairo.cc"
#include "upstream/hb-shaper.cc"
#include "upstream/hb-aat-layout.cc"
#include "upstream/hb-ot-face.cc"
#include "upstream/hb-ot-meta.cc"
#include "upstream/hb-ot-shaper-hebrew.cc"
#include "upstream/hb-subset-input.cc"
#include "upstream/hb-cairo-utils.cc"
#include "upstream/hb-aat-map.cc"
#include "upstream/hb-ot-shaper-vowel-constraints.cc"
#include "upstream/hb-subset-cff2.cc"
#include "upstream/hb-set.cc"
#include "upstream/hb-shape.cc"
#include "upstream/hb-ot-name.cc"
#include "upstream/hb-gdi.cc"
#include "upstream/hb-ot-math.cc"
#include "upstream/hb-buffer-verify.cc"
#include "upstream/hb-ot-shaper-arabic.cc"
