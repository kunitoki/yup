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

#pragma once

namespace yup
{

//==============================================================================
/** The optimisation policy applied to a YDSP compilation.

    `automatic` selects the native target available on the compiler's host.
    It is spelt out instead of `auto`, which is a C++ keyword.
*/
enum class YdspOptimizationTier
{
    baseline,
    automatic,
    aggressive
};

//==============================================================================
/** Chooses whether compilation follows the host CPU or a portable baseline. */
enum class YdspTargetPolicy
{
    host,
    baseline
};

//==============================================================================
/** Native instruction-set targets supported by the YDSP compiler. */
enum class YdspNativeTarget
{
    scalar,
    sse2,
    avx2,
    avx512,
    asimd
};

//==============================================================================
/** Options controlling one YdspCompiler::compile() call.

    The default is the host-selected `automatic` tier, with `fastMath` enabled:
    native targets lower widened transcendentals to SLEEF's `u35` 4-lane set
    and fused multiply-add contraction is allowed (scalar float32 values always
    stay on the platform libm). The WebAssembly backend never widens
    transcendentals (it does not link sleef_library), but it otherwise honors
    `fastMath`: scalar float32 contraction is fused there too, and a target
    without a fused multiply-add instruction (wasm included) expands the fused
    op through the exact float64 sequence, so it rounds once and stays
    bit-stable with the native default. Set `fastMath` to false for strict
    1-ULP behavior: widened transcendentals then use SLEEF's `u10` tier on
    native and nothing contracts anywhere.
*/
struct YdspCompileOptions
{
    YdspOptimizationTier optimizationTier = YdspOptimizationTier::automatic;
    bool fastMath = true;
    YdspTargetPolicy targetPolicy = YdspTargetPolicy::host;
    YdspNativeTarget baselineTarget = YdspNativeTarget::scalar;
    bool emitOptimizationReport = false;
};

//==============================================================================
/** The native-code decisions made by the most recent compilation.

    The report is populated when YdspCompileOptions::emitOptimizationReport
    is true. It records the target actually emitted after capability and
    profitability checks, rather than merely echoing the requested target.
*/
struct YdspOptimizationReport
{
    YdspOptimizationTier optimizationTier = YdspOptimizationTier::automatic;
    bool fastMath = false;
    YdspNativeTarget selectedIsa = YdspNativeTarget::scalar;
    String selectedMicroarchitecture;
    int vectorWidth = 1;
    bool vectorizationEnabled = false;
    bool unrollingEnabled = false;
    bool reductionSplittingEnabled = false;
    bool contractionEnabled = false;
    StringArray rejectedTransforms;
    size_t generatedCodeSize = 0;
    double compileTimeMilliseconds = 0.0;
    bool cacheHit = false;
    String cacheDecision;
    String benchmarkDecision;
};

} // namespace yup
