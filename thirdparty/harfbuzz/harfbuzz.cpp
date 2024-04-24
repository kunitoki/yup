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

#include "source/common/hb-directwrite.cc"
#include "source/common/hb-subset-plan.cc"
#include "source/common/hb-ucd.cc"
#include "source/common/hb-coretext.cc"
#include "source/common/hb-ot-shaper-syllabic.cc"
#include "source/common/hb-ot-var.cc"
#include "source/common/hb-ot-map.cc"
#include "source/common/hb-icu.cc"
#include "source/common/hb-blob.cc"
#include "source/common/hb-draw.cc"
#include "source/common/hb-ot-shape-normalize.cc"
#include "source/common/hb-unicode.cc"
#include "source/common/hb-ot-shaper-thai.cc"
#include "source/common/hb-face-builder.cc"
#include "source/common/hb-ot-color.cc"
#include "source/common/hb-face.cc"
#include "source/common/hb-ot-cff1-table.cc"
#include "source/common/hb-paint.cc"
#include "source/common/hb-subset-cff1.cc"
#include "source/common/hb-subset-instancer-solver.cc"
#include "source/common/hb-ot-font.cc"
#include "source/common/hb-subset.cc"
#include "source/common/hb-common.cc"
#include "source/common/hb-buffer-serialize.cc"
#include "source/common/hb-subset-cff-common.cc"
#include "source/common/hb-fallback-shape.cc"
#include "source/common/hb-uniscribe.cc"
#include "source/common/graph/gsubgpos-context.cc"
#include "source/common/hb-ot-shaper-khmer.cc"
#include "source/common/hb-paint-extents.cc"
#include "source/common/hb-static.cc"
#include "source/common/hb-subset-repacker.cc"
#include "source/common/hb-style.cc"
#include "source/common/hb-ot-tag.cc"
#include "source/common/hb-glib.cc"
#include "source/common/hb-ot-shaper-myanmar.cc"
#include "source/common/hb-ft.cc"
#include "source/common/hb-ot-shape-fallback.cc"
#include "source/common/hb-gobject-structs.cc"
#include "source/common/hb-buffer.cc"
#include "source/common/hb-wasm-api.cc"
#include "source/common/hb-number.cc"
#include "source/common/hb-ot-cff2-table.cc"
#include "source/common/hb-shape-plan.cc"
#include "source/common/hb-outline.cc"
#include "source/common/hb-ot-metrics.cc"
#include "source/common/hb-ot-shaper-hangul.cc"
#include "source/common/hb-ot-shaper-use.cc"
#include "source/common/hb-ot-shaper-indic.cc"
#include "source/common/hb-wasm-shape.cc"
#include "source/common/hb-ot-shaper-indic-table.cc"
#include "source/common/hb-ot-layout.cc"
#include "source/common/hb-ot-shaper-default.cc"
#include "source/common/hb-ot-shape.cc"
#include "source/common/hb-map.cc"
#include "source/common/hb-font.cc"
#include "source/common/hb-graphite2.cc"
#include "source/common/hb-cairo.cc"
#include "source/common/hb-shaper.cc"
#include "source/common/hb-aat-layout.cc"
#include "source/common/hb-ot-face.cc"
#include "source/common/hb-ot-meta.cc"
#include "source/common/hb-ot-shaper-hebrew.cc"
#include "source/common/hb-subset-input.cc"
#include "source/common/hb-cairo-utils.cc"
#include "source/common/hb-aat-map.cc"
#include "source/common/hb-ot-shaper-vowel-constraints.cc"
#include "source/common/hb-subset-cff2.cc"
#include "source/common/hb-set.cc"
#include "source/common/hb-shape.cc"
#include "source/common/hb-ot-name.cc"
#include "source/common/hb-gdi.cc"
#include "source/common/hb-ot-math.cc"
#include "source/common/hb-buffer-verify.cc"
#include "source/common/hb-ot-shaper-arabic.cc"
