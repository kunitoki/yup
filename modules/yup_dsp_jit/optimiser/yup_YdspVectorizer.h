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
    value simply carries `lanes > 1` in YdspIrFunction::valueLanes, and the
    existing arithmetic opcodes are reused (`addF` on a 4-lane value *is* a
    packed add). Only two opcodes are added, for the two places where the
    operand and result lane counts differ: `vsplat` and `vreduceAddF`.

    The target shape is a bank of parallel `state float[N]` arrays stepped once
    per sample - an additive oscillator bank, a modal filter bank - which is a
    unit-stride, same-index read-then-write loop, usually with one accumulating
    reduction.

    Deliberate restrictions, each of which keeps a whole class of risk out:

    - **Only constant trip counts divisible by the width.** There is therefore
      no scalar epilogue and no vector loop to insert, so no block is created,
      emptied or reordered and the CFG-linear block layout both backends depend
      on is preserved by construction rather than by care.
    - **Only a single-block loop body**, so there is no control flow to
      if-convert and no nested loop to reason about.
    - **Only state-array accesses are widened.** Stream accesses (`in[i]`,
      `out[i]`) are always block-size bound in practice, and widening them
      would need the epilogue this pass does not have.
    - **A transcendental, a comparison or a `select` on a widened value
      disqualifies the loop** rather than being scalarised through
      extract/insert. In the shapes this exists for, the transcendentals are
      loop-invariant and loop-invariant code motion has already hoisted them
      into the preheader, where they stay scalar and are splatted once.

    Reductions reassociate: lane `j` accumulates elements `j, j + W, j + 2W …`
    and the lanes are then summed pairwise, so a reduction is *not* bit-exact
    against the scalar order. Element-wise work is bit-exact. The vector
    accumulator is also what breaks the reduction's serial dependency chain,
    which is the larger half of the win.

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
};

} // namespace yup
