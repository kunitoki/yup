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
/** The IR of a whole program: one function per processor. */
struct YdspIrProgram
{
    std::vector<std::unique_ptr<YdspIrFunction>> kernels;

    // Event-handler functions, one per analyzed handler, in program order.
    std::vector<std::unique_ptr<YdspIrFunction>> eventHandlers;

    const YdspAnalyzedProgram* analyzed = nullptr;
};

//==============================================================================
/** Builds the typed IR from an analyzed program and optimises it.

    The builder lowers each processor's AST into a block-structured IR with
    mutable virtual registers, enforcing the parameter sampling semantics
    (params loaded once per block) and lowering the delay primitives (' and
    @) into hidden state. The optimiser then runs constant folding, algebraic
    simplification, copy propagation, dead-code elimination and loop-invariant
    code motion, and can produce a worst-case execution report.
*/
class YdspOptimizer
{
public:
    /** Constructs an optimizer reporting into the given diagnostics. */
    explicit YdspOptimizer (YdspDiagnostics& diagnostics);

    /** Builds and optimises the IR for the given analyzed program. */
    std::unique_ptr<YdspIrProgram> build (const YdspAnalyzedProgram& program);

    /** Enables the bounded-loop vectoriser (off by default).

        Only the native backends lower `lanes > 1`, so the caller enables this
        for a target that has a packed float unit. YdspWasmCodegen has no
        `0xFD`-prefix opcode family at all and rejects a widened function.

        @see YdspVectorizer
    */
    void setVectorizationEnabled (bool shouldVectorize) noexcept { vectorizationEnabled = shouldVectorize; }

    /** Sets the float32 lane count used by the vectoriser when enabled.

        Native callers use 4 for SSE2/ASIMD and 8 for AVX2. Invalid widths
        leave the function scalar rather than producing malformed IR.
    */
    void setVectorWidth (int width) noexcept { vectorWidth = width; }

    /** Enables the constant-trip-count loop unroller (off by default).

        Trading code size for branches is only the right trade where the code is
        already resident: a wasm module is downloaded and parsed before it runs,
        and the browser's own engine re-optimises the loop anyway, so that target
        leaves it off. Leaving it off also lets a caller inspect the IR a
        specific pass produced without a later one having multiplied it.

        @see fullyUnrollBoundedLoops
    */
    void setUnrollingEnabled (bool shouldUnroll) noexcept { unrollingEnabled = shouldUnroll; }

    /** Enables splitting a widened accumulator in two (off by default).

        Kept separate from the unroller it depends on for two reasons: it
        re-associates a sum a second time, so a caller that wants exactly the
        vectoriser's association can decline it; and whether shortening the
        serial chain wins depends on whether the reduction is latency-bound or
        throughput-bound on the target, which is a measurement rather than a
        rule.

        @see splitWidenedReductionChains
    */
    void setReductionSplittingEnabled (bool shouldSplit) noexcept { reductionSplittingEnabled = shouldSplit; }

    /** Enables rewriting `a * b + c` into a single fused multiply-add (off by
        default).

        This is the one switch here that changes the samples a patch produces:
        it removes a rounding. It changes them *identically on every backend* -
        `fmaF` has one defined value and a target without the instruction
        reaches it by computing one width up (see lowerFusedMultiplyAdd) - so
        turning it on does not make the native and wasm outputs disagree. It
        makes both disagree with the unfused build, which is why it is opt-in
        rather than the default.

        Runs after the vectoriser and the unroller. It forms a widened FMA only
        when setTargetHasPackedFusedMultiplyAdd() permits it; otherwise it
        leaves a bank loop unchanged. The scalar shapes this exists for are
        per-sample recurrences, where it removes a link from the critical path
        rather than an instruction from a throughput-bound body.

        @see contractMultiplyAdd, YdspIrOp::fmaF
    */
    void setContractionEnabled (bool shouldContract) noexcept { contractionEnabled = shouldContract; }

    /** Tells the optimiser whether the target lowers `fmaF` natively (true by
        default).

        When false, every `fmaF` - whether written as `fma()` in the patch or
        produced by contraction - is expanded into float64 arithmetic that
        rounds once, which is the same value the instruction would have
        produced. Set it from the actual target: AArch64 always has `fmadd`,
        x86-64 only with FMA3, and wasm has no fused form at all.

        One caveat, and it is pre-existing rather than introduced here: the
        expansion is exact for normal results, but where the result is subnormal
        the second rounding is not provably innocuous. yup_dsp_jit sets no
        flush-to-zero mode of its own, so a native kernel inherits the host's
        (AudioGraphProcessor's process callbacks run under ScopedNoDenormals)
        while wasm mandates full IEEE with no flush at all - so those two
        targets already disagree in the subnormal range regardless of this.

        @see lowerFusedMultiplyAdd
    */
    void setTargetHasFusedMultiplyAdd (bool isSupported) noexcept { targetHasFusedMultiplyAdd = isSupported; }

    /** Tells the optimiser whether this target can lower packed float32 FMA.

        The contraction pass only forms a widened fmaF when this is true;
        lowerFusedMultiplyAdd() has an exact scalar fallback but deliberately
        does not scalarise a vector operation.
    */
    void setTargetHasPackedFusedMultiplyAdd (bool isSupported) noexcept { targetHasPackedFusedMultiplyAdd = isSupported; }

    /** Populates the execution report from an optimised IR program. */
    static void buildReport (const YdspIrProgram& program, YdspExecutionReport& report);

    /** Runs the constant-folding pass over the given function.

        Folds instructions whose operands are all compile-time constants into
        a single constant instruction. Because the IR is non-SSA, only value
        ids defined exactly once are ever treated as constant (multi-defined
        registers, such as loop induction variables, are left alone).
    */
    void constantFolding (YdspIrFunction& fn);

    /** Runs the algebraic-simplification pass over the given function.

        Applies identity/annihilator peepholes (x + 0, x * 1, x * 0, x / 1,
        x & 0, ...) for instructions whose operands are literal constants in
        the same block.
    */
    void algebraicSimplification (YdspIrFunction& fn);

    /** Runs the copy-propagation pass over the given function.

        Replaces uses of a `mov` destination with its source within a block,
        stopping at redefinitions of either register.
    */
    void copyPropagation (YdspIrFunction& fn);

    /** Runs the if-conversion pass over the given function.

        Turns a short, else-less `if` whose body is one side-effect-free block
        into straight-line code plus a `select` per assignment, so a data
        dependent branch in a sample loop becomes a conditional move.

        No block is added or removed: the condition block absorbs the body and
        both fall through, which leaves every block index - and so every loop
        bound and every region the wasm backend recovers - exactly as it was.
    */
    void ifConversion (YdspIrFunction& fn);

    /** Runs the store-to-load-forwarding pass over the given function.

        Rewrites a state-array load into a move from the value most recently
        stored to that same element, so `a[i] = x; ... = a[i];` stops going
        through memory. The load's result is unchanged, so this needs no SSA
        property: it replaces one definition with an identical value.

        Only forwards within a block, and only when the region, the index value
        and the stored value are all provably unchanged in between. An
        intervening array store blocks the forward unless it writes a different
        region through the same index and element width - two such addresses
        differ by their region bases alone, so they cannot alias whatever the
        index holds.
    */
    void storeToLoadForwarding (YdspIrFunction& fn);

    /** Runs the dead-code-elimination pass over the given function.

        Removes value-producing instructions whose result is never used,
        iterating to a fixed point so removal chains (e.g. an unused load
        feeding an unused multiply) are fully cleaned up.
    */
    void deadCodeElimination (YdspIrFunction& fn);

    /** Runs the loop-invariant-code-motion pass over the given function.

        Hoists pure, invariant computations out of bounded loops into the
        function entry block. Only single-assignment values defined in the
        entry block (or constants) are ever treated as invariant - loop-carried
        registers (the induction variable is written both by the prologue and
        the loop body) and path-dependent registers redefined inside the loop
        are not - and loads from input/output streams, params and state memory
        are conservatively left in place.
    */
    void loopInvariantCodeMotion (YdspIrFunction& fn);

    /** Fully unrolls every small constant-trip-count loop in the function.

        The body is copied into the preheader once per iteration, verbatim and
        in order - including the loop variable's own increment, so the sequence
        of induction values is exactly the one the loop produced and nothing has
        to be substituted. What disappears is the per-iteration bound compare
        and back edge.

        No block is added, removed or reordered, which is what the wasm
        backend's region recovery and every loop bound depend on: the header and
        the body are emptied and left falling through, the same trick
        ifConversion() uses. The loop's entry in `fn.loops` stays, so the report
        still knows the worst-case iteration count, but it is marked `unrolled`
        so no backend tries to recover a loop region around blocks that no
        longer branch.

        Runs after the vectoriser, so a widened loop is unrolled at its widened
        trip count (16 modes at four lanes is four copies, not sixteen) - and
        running before it would leave nothing loop-shaped to widen.
    */
    void fullyUnrollBoundedLoops (YdspIrFunction& fn);

    /** Halves the serial depth of a chain of adds into a widened accumulator.

        Unrolling a widened bank loop leaves `acc = acc + x` repeated once per
        copy, and each add waits on the one before it. Sending the odd links to
        a second accumulator and adding the two at the end halves that serial
        depth for the cost of a single instruction.

        Halving can fire more than once, because the scan resumes just inside
        the chain it rewrote and finds the suffix still on the accumulator: an
        eight-link chain halves twice and ends at depth four. That is not the
        depth a balanced tree would reach - restarting the block scan after
        each split would go further, and has not been measured.

        Only a *widened* accumulator is touched. Its association has already
        been changed by the vectoriser - lane j sums elements j, j+4, j+8 … and
        the lanes are then folded pairwise - and that is documented as not
        bit-exact, so this stays inside a licence the language already takes. A
        scalar chain carries no such licence and is left alone.
    */
    void splitWidenedReductionChains (YdspIrFunction& fn);

    /** Rewrites `t = a * b; r = t + c` into `r = fma (a, b, c)`.

        Only where the multiply is read by nothing but the add, is defined
        exactly once, and can be moved down to the add's position without any
        of its operands having changed in between. Handles scalar float32 and
        packed float32 when the target has a packed FMA instruction.

        When both of the add's operands are such multiplies, only one can be
        fused, and which one is a latency decision - see the comment at the
        choice. Removing the multiply is left to dead-code elimination, which
        has to run afterwards for the pass to pay for itself.

        @see setContractionEnabled
    */
    void contractMultiplyAdd (YdspIrFunction& fn);

    /** Expands every `fmaF` into float64 arithmetic that rounds once.

        For targets with no fused multiply-add instruction. The result is the
        value the instruction would have produced, not an approximation of it,
        which is what lets `fma()` mean the same thing on every backend.

        @see setTargetHasFusedMultiplyAdd
    */
    void lowerFusedMultiplyAdd (YdspIrFunction& fn);

private:
    void runPasses (YdspIrFunction& fn);

    YdspDiagnostics& diagnostics;
    bool vectorizationEnabled = false;
    int vectorWidth = YdspVectorizer::vectorWidth;
    bool unrollingEnabled = false;
    bool reductionSplittingEnabled = false;
    bool contractionEnabled = false;
    bool targetHasFusedMultiplyAdd = true;
    bool targetHasPackedFusedMultiplyAdd = false;
};

} // namespace yup
