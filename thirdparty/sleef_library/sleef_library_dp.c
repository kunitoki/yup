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

// Scalar double-precision SLEEF (Sleef_sin_u10, ...). Each upstream .c is its
// own translation unit, as SLEEF's own build does - the DORENAME renaming
// (upstream/src/libm/rename.h) therefore applies to this file alone.

#define DORENAME
// SLEEF sources carry per-call debug diagnostics (fprintf to stderr) behind
// `#ifndef NDEBUG`, which SLEEF's own release builds disable. YUP debug
// builds define DEBUG=1 (not NDEBUG) for module code, so this module pins
// NDEBUG for its translation units: without it an audio thread would print
// per sample (crackling, console flood) whenever a double-float helper sees
// |x| < |y|.
#ifndef NDEBUG
#define NDEBUG
#endif

#include "upstream/src/libm/sleefdp.c"
