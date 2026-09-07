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
    //==============================================================================
    /** Constructs an optimizer reporting into the given diagnostics. */
    explicit YdspOptimizer (YdspDiagnostics& diagnostics);

    //==============================================================================
    /** Builds and optimises the IR for the given analyzed program. */
    std::unique_ptr<YdspIrProgram> build (const YdspAnalyzedProgram& program);

    //==============================================================================
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

    /** Marks the compile as fast-math, which the codegen answers by lowering
        widened float32 transcendentals to SLEEF's `u35` 4-lane set (instead of
        the `u10` set used when this is false). Scalar float32 transcendentals
        stay on libm either way - the platform libm scalar is per-target tuned,
        and measured SLEEF scalar calls regressed against it.

        The flag is mirrored onto each YdspIrFunction the optimiser builds, so
        the codegen can read it from the IR it is given.

        @see YdspIrFunction::fastMath
    */
    void setFastMath (bool isEnabled) noexcept { fastMath = isEnabled; }

    /** Tells the optimiser whether the target can lower a widened
        transcendental at all (native targets link sleef_library; wasm does
        not).

        When true, the vectoriser admits the SLEEF-backed transcendental
        opcodes to its packed set, so a loop whose widened value feeds a
        `sin()`/`exp()`/... is vectorised instead of being refused. The codegen
        then lowers those calls at `u35` under fastMath and `u10` otherwise.

        @see YdspIrFunction::vectorMathEnabled
    */
    void setTargetHasVectorMath (bool isSupported) noexcept { vectorMathEnabled = isSupported; }

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

    //==============================================================================
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

    /** Runs block-local common-subexpression elimination. */
    void commonSubexpressionElimination (YdspIrFunction& fn);

    /** Folds a scalar state write-back into the value it moves.

        A sample-mode scalar state update is `v = op (...)` followed by
        `movF`/`movI y = v`, where `y` is the loop-carried state register the
        builder redefines every sample. Folding rewrites the producer to write
        `y` directly, redirects every use of `v` to `y`, and drops the move -
        one instruction and no extra register on the loop-carried chain (the
        ladder filter's per-stage `fmov`s, a ring buffer's `wp = wp + 1`
        `mov`s, and the sample loop's own induction move). The canonical
        write-back of a constant-bound loop's induction is left alone - fusion,
        unrolling and the vectoriser pattern-match the loop body on it before
        this pass runs - while per-sample carried state and the blockSize-bound
        sample-loop induction fold.

        `v` is rarely single-use: the builder binds the *variable* to `v` for
        later in-block reads, and copy propagation plus the inlined-parameter
        moves add more. Every such use must live in the move's own block,
        strictly after the producer, and never in the block's terminator
        condition - a read that survives in a later block (the value carried
        out of an `if`) binds to the state register `y` and would have no
        definition once the producer is renamed, so the fold is refused when
        any use falls outside the block. It is also refused when something
        redefines `y` before a redirected use or reads `y` between the producer
        and the move, and folds into the same `y` are applied one at a time:
        each application can expose such a conflict to the next, which a batch
        rewrite against a static snapshot cannot see.

        Runs after contraction, so the fold applies to fused `fmaF` links too.
    */
    void foldStateWriteBacks (YdspIrFunction& fn);

    /** Rematerializes scalar leaf values that would otherwise be parked around
        a libm call.

        A coefficient that is a parameter load or a compile-time constant,
        defined before a sample loop but used only after the loop body's last
        libm call, crosses that call on every iteration; when the preserved
        pool is full the allocator parks it on the stack around the call (the
        per-sample `str`/`ldr [sp]` traffic in the compressor and chorus
        kernels). Re-defining it right after the call splits its live range so
        it never spans a call: the clone re-executes once per sample - one
        reload, the same value - and the original definition becomes dead,
        which also frees a preserved register for values that cannot be
        recomputed. Leaf defs only (`constF`, `constI`, `loadParam`), single
        defs, linear (fall-through) loop bodies, and every use must sit after
        the last call in that body; anything else keeps its old live range.
    */
    void rematerializePostCallLeaves (YdspIrFunction& fn);


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

    /** Fuses adjacent loops with identical bounds into one.

        Two consecutive single-block loops that run over the same bound and
        touch disjoint memory are merged into a single loop whose body runs
        both original bodies per iteration, halving the header-compare and
        back-edge overhead. Conservative by design: the loops must be laid out
        back to back, both bodies must be single blocks, the second loop's
        induction must provably start at 0, and nothing one body writes may be
        read or written by the other (see the pass source for the exact
        preconditions).

        Runs after loop-invariant code motion and before the vectoriser, so a
        fused loop is widened and unrolled as one.

        @see fullyUnrollBoundedLoops
    */
    void loopFusion (YdspIrFunction& fn);

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

    /** Balances a widened reduction chain into a tree.

        A serial widened reduction `acc = ((acc + x1) + x2) + ...` has a
        dependency depth of one vector add per unrolled copy. The pass pairs
        the independent addends into a balanced tree (depth ceil(log2(n+1)),
        snapshotting the pre-chain accumulator value as one more leaf) instead
        of a chain, which is what the latency-bound reduction of a widened
        bank-loop is sensitive to. Kept behind the unroller: only an unrolled
        loop exposes the addends as separate values.

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

    /** Populates the execution report from an optimised IR program. */
    static void buildReport (const YdspIrProgram& program, YdspExecutionReport& report);

private:
    void runPasses (YdspIrFunction& fn);

    YdspDiagnostics& diagnostics;
    bool vectorizationEnabled = false;
    int vectorWidth = YdspVectorizer::vectorWidth;
    bool unrollingEnabled = false;
    bool reductionSplittingEnabled = false;
    bool contractionEnabled = false;
    bool fastMath = false;
    bool targetHasFusedMultiplyAdd = true;
    bool targetHasPackedFusedMultiplyAdd = false;
    bool vectorMathEnabled = false;
};

} // namespace yup
