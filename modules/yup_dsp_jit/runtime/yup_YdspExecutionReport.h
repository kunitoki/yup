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
/** Why a bounded loop was widened, or left scalar, by the vectoriser.

    One entry per original loop, in loop order, produced only when the
    vectoriser actually ran for the kernel (the automatic/aggressive tiers on
    a target with a packed float unit). `widened` means the loop now runs at
    `laneCount` float32 lanes; any other value is the exact reason the loop
    stayed scalar - the "why is this loop scalar?" answer a host can show or
    log directly.
*/
enum class YdspVectorizationReason
{
    widened,                     //!< the loop was widened to `laneCount` lanes
    notVectorizable,             //!< malformed IR; no more specific cause is known
    unsupportedLoopBound,        //!< the bound is neither a constant nor blockSize
    multiBlockBody,              //!< the body contains an `if` or a nested loop
    nonConstantStart,            //!< the loop start is not a compile-time constant
    nonzeroStart,                //!< a blockSize loop must start at 0
    shortTripCount,              //!< the loop span is shorter than one vector
    missingHeaderCompare,        //!< the header does not hold exactly one `i < bound`
    unrecognizedInductionUpdate, //!< the body does not end with `next = i + 1; i = next`
    inductionUsedAsValue,        //!< the loop variable is used outside an array/stream index
    indirectAccess,              //!< an element is reached through a non-loop-variable index
    unrecognizedAccumulation,    //!< a carried value is not of the form `acc = acc + x`
    loopCarriedValue,            //!< the body writes a value that escapes the loop
    unsupportedWidenedOp,        //!< a select, comparison, transcendental or rounding consumes a widened value
    unsupportedWidenedType,      //!< a widened value is not float32
    stateWriteInBody,            //!< the body writes scalar state, a param or an event field
    invariantStreamStore,        //!< a stream store does not go through the loop variable
    emitInBody,                  //!< the body emits an output event
    nothingToWiden,              //!< the body has no array or stream access
    runtimeBoundWithoutStreams   //!< a blockSize loop needs a stream access at the loop variable
};

//==============================================================================
/** The outcome of trying to widen one bounded loop. */
struct YdspVectorizationResult
{
    /** The loop's index into YdspIrFunction::loops (stable across the pass). */
    int loopId = -1;

    /** widened, or the exact reason the loop stayed scalar. */
    YdspVectorizationReason reason = YdspVectorizationReason::notVectorizable;

    /** The widened lane count when reason == widened, else 1. */
    int laneCount = 1;

    /** True when the loop was widened to SIMD lanes. */
    bool widened() const noexcept
    {
        return reason == YdspVectorizationReason::widened;
    }

    /** Renders "loop 2 was widened to 4 lanes" or
        "loop 2 was not vectorized: <reason text>". */
    String describe() const;
};

//==============================================================================
/** The vectoriser's outcome for one kernel: one entry per original loop. */
struct YdspVectorizationReport
{
    /** Per-loop outcomes, parallel to the kernel's original loops. */
    std::vector<YdspVectorizationResult> loops;

    /** The number of loops that were widened. */
    int countWidened() const noexcept;

    /** The distinct "stayed scalar" reasons, deduplicated, as human text. */
    StringArray rejectionReasons() const;
};

//==============================================================================
/** The optimiser's worst-case execution analysis for one generated kernel.

    After optimisation the compiler proves that every loop in the kernel is
    statically bounded (constant or blockSize-derived). boundedIterationCount
    is the product of all loop bounds - the maximum number of times the
    kernel's loop bodies can execute per block - and provenRealtimeSafe is
    true only when every bound is statically known.
*/
struct YdspKernelReport
{
    /** The name of the kernel. */
    String name;

    /** The number of instructions in the kernel's IR after optimisation. */
    int instructionCount = 0;

    /** The product of all statically-known loop bounds in the kernel. If any
        loop is unbounded, this is 0 and provenRealtimeSafe is false. */
    int boundedIterationCount = 0;

    /** True when every loop bound is statically known. */
    bool provenRealtimeSafe = true;

    /** Rendered loop bounds ("16", "blockSize", "blockSize - 1"), one per loop. */
    yup::StringArray loopBounds;

    /** True when at least one loop was widened to SIMD lanes, and the lane count
        that was used (1 when nothing was widened, and on the wasm backend, which
        stays scalar). A widened reduction reassociates, so a kernel reporting
        `vectorized` is not bit-identical to the scalar one. */
    bool vectorized = false;
    int vectorWidth = 1;

    /** Per-loop widening outcomes: `widened` or the exact reason each *original*
        loop stayed scalar - the loops that existed before the vectoriser ran,
        in their order (synthetic scalar tail loops the vectoriser appends are
        not reported). Empty when the vectoriser did not run for this kernel. */
    yup::YdspVectorizationReport loopVectorization;

    /** True when at least one loop was fully unrolled. `boundedIterationCount`
        above is unaffected - it still answers "how many iterations could this
        run", which is the same number whether they are written out or looped -
        so this is the only way to tell the two forms apart from outside. An
        unrolled loop is bit-identical to the rolled one. */
    bool unrolled = false;

    /** True when a widened accumulator was halved. Separate from `vectorized`
        because it re-associates the sum a *second* time, and separate from
        `unrolled` because it only fires on chains long enough to be worth it -
        so neither of those answers "did this kernel's reduction get shortened". */
    bool reductionSplit = false;
};

//==============================================================================
/** The worst-case execution report of a compiled YDSP patch.

    Exposed through YdspAudioGraph::getExecutionReport() so a host can verify,
    before starting audio, that the patch fits its realtime CPU budget.
*/
class YdspExecutionReport
{
public:
    /** Returns the per-kernel worst-case analyses (const). */
    const std::vector<YdspKernelReport>& getKernels() const noexcept;

    /** Returns the per-kernel worst-case analyses (mutable, for the optimizer). */
    std::vector<YdspKernelReport>& getKernels() noexcept;

    /** Returns the total worst-case loop iterations across all kernels. */
    int getTotalBoundedIterations() const noexcept;

    /** Returns true if every kernel is proven realtime-safe. */
    bool isProvenRealtimeSafe() const noexcept;

private:
    friend class YdspAudioGraph;

    std::vector<YdspKernelReport> kernels;
};

} // namespace yup
