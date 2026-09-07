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

// SIMD double-precision SLEEF (Sleef_sind2_u10, ...). The module compiles one
// ISA; its rename header (upstream/src/libm/renamesse2.h or renameadvsimd.h)
// exports the canonical names without an ISA suffix.

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

#if defined(__aarch64__) || defined(_M_ARM64)
  #define ENABLE_ADVSIMD
#elif defined(__x86_64__) || defined(_M_X64)
  #define ENABLE_SSE2
#else
  #error "sleef_library supports x86-64 (SSE2) and AArch64 (ASIMD) only"
#endif

#include "upstream/src/libm/sleefsimddp.c"
