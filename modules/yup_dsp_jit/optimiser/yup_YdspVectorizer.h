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
/** Widens constant-bound loops over parallel state arrays to SIMD lanes.

    An IR → IR pass, so it is testable without a JIT and every backend sees the
    same widened IR. It does not introduce a new instruction set: a widened
    value simply carries `lanes > 1`, and the existing arithmetic opcodes are
    reused (`addF` on a 4-lane value *is* a packed add). Only two opcodes are
    added, for the two places where the operand and result lane counts differ:
    `vsplat` and `vreduceAddF`.

    The target shapes are a bank of parallel `state float[N]` arrays stepped
    once per sample - an additive oscillator bank, a modal filter bank - and
    the per-sample stream loop (`out[i] = in[i] * k`, or a block-mode
    `for i in 0..blockSize` over streams), both unit-stride, same-index
    read-then-write loops, usually with one accumulating reduction.

    Deliberate restrictions, each of which keeps a whole class of risk out:

    - **Only a single-block loop body**
    - **Scalar tails only for constant bounds and constant starts.**
    - **Stream accesses are widened only at the induction variable.**
    - **A transcendental, a comparison or a `select` on a widened value
      disqualifies the loop**

    @see YdspOptimizer::setVectorizationEnabled
*/
class YdspVectorizer
{
public:
    /** The compatibility lane count: SSE2 / ASIMD baseline. */
    static constexpr int vectorWidth = 4;

    /** Widens every qualifying loop in the function.

        Returns true if at least one loop was widened, in which case
        `fn.vectorized` is set and `fn.valueLanes` is populated.
    */
    static bool run (YdspIrFunction& fn);

    /** Widens every qualifying loop using a target-derived float32 width.

        Only 4, 8 and 16 lanes are accepted. The 4-lane overload above is
        retained for IR clients that target the portable SSE2 / ASIMD subset.
    */
    static bool run (YdspIrFunction& fn, int targetVectorWidth);

    /** Widens every qualifying loop, recording each loop's outcome.

        Same behaviour as run (fn, targetVectorWidth); additionally, every
        original loop gets a YdspVectorizationResult - `widened` with its lane
        count, or the exact reason it stayed scalar - which is also stored on
        fn.vectorizationResults for the execution report.
    */
    static bool run (YdspIrFunction& fn, int targetVectorWidth, YdspVectorizationReport& report);
};

} // namespace yup
