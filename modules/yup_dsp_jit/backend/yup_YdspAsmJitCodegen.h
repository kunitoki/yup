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
/** Register types used by one native emitter, independently of the compiler host. */
struct YdspX64Traits
{
    using Compiler = asmjit::x86::Compiler;
    using Gp = asmjit::x86::Gp;
    using Fp = asmjit::x86::Vec;
    using Mem = asmjit::x86::Mem;
    using Cond = asmjit::x86::CondCode;
    static constexpr bool isX64 = true;
};

struct YdspARM64Traits
{
    using Compiler = asmjit::a64::Compiler;
    using Gp = asmjit::a64::Gp;
    using Fp = asmjit::a64::Vec;
    using Mem = asmjit::a64::Mem;
    using Cond = asmjit::arm::CondCode;
    static constexpr bool isX64 = false;
};

//==============================================================================
/** Compiles one YdspIrFunction into native machine code via AsmJit.

    This is the architecture-independent facade of the native backend; the
    actual lowering is split per target:

      - YdspAsmJitCodegenX64   - x86-64 (SSE) lowering
      - YdspAsmJitCodegenARM64 - AArch64 (ASIMD) lowering

    Both emitters are available on every compiler host. emit() chooses by
    target, independent of the host CPU. load() checks execution compatibility;
    compile() combines both steps for the current host.
*/
class YdspAsmJitCodegen
{
public:
    /** Emits a kernel for an explicit target without allocating executable memory.
        Helper calls are recorded as symbols and resolved only by load().
    */
    static ResultValue<YdspNativeArtifact> emit (const YdspTargetTriple& target, const YdspIrFunction& fn, YdspDiagnostics& diagnostics);

#if ! YUP_WASM
    /** Installs an artifact only when its OS/architecture matches this process.
        The artifact must come from a trusted compiler/bundle.
    */
    static YdspKernelFn load (asmjit::JitRuntime& runtime, const YdspTargetTriple& target, const YdspNativeArtifact& artifact, YdspDiagnostics& diagnostics);

    /** Compiles the IR function; returns nullptr and records a diagnostic on failure. */
    static YdspKernelFn compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize = nullptr);

    /** Compiles an event-handler IR function; returns nullptr and records a diagnostic on failure. */
    static YdspEventHandlerFn compileEventHandler (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize = nullptr);
#endif

    /** Returns the state memory size in bytes for the given function. */
    static size_t stateSize (const YdspIrFunction& fn);

    /** Returns the scalar segment's size in bytes (stateArrays = state + this). */
    static size_t stateScalarSize (const YdspIrFunction& fn);
};

//==============================================================================
/** Shared, architecture-independent lowering used by the native backends.

    Owns the register/offset bookkeeping, the state layout and the generic
    instruction lowering; every target-specific instruction form is delegated
    to a virtual hook that YdspAsmJitCodegenX64 and YdspAsmJitCodegenARM64
    implement. The orchestration (context setup, register allocation, block
    emission, error reporting) therefore lives in one place, and each
    architecture only provides its own encodings.

    @internal
*/
template <typename Traits>
class YdspAsmJitCodegenImpl
{
public:
    using YdspAsm = typename Traits::Compiler;
    using YdspGp = typename Traits::Gp;
    using YdspFp = typename Traits::Fp;
    using YdspMem = typename Traits::Mem;
    using YdspCond = typename Traits::Cond;
    using YdspInstPosition = std::pair<int, int>;

    /** Emits position-independent code for the supplied target environment. */
    ResultValue<YdspNativeArtifact> emit (const asmjit::Environment& environment,
                                          const YdspIrFunction& fn,
                                          YdspDiagnostics& diagnostics,
                                          bool isEventHandler);

protected:
    virtual ~YdspAsmJitCodegenImpl() = default;

    //==============================================================================
    // Architecture hooks (implemented by the two concrete backends)

    /** Builds [base + offset]. */
    virtual YdspMem memPtr (const YdspGp& base, int32_t offset) const = 0;

    /** Builds [base + index * 2^scaleLog2 + offset]; offset must be 0 on AArch64. */
    virtual YdspMem memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const = 0;

    /** Builds the effective address of a state access (scalar slot when
        indexValue < 0, array element otherwise). */
    virtual YdspMem emitStateMem (YdspValueType type, int base, int indexValue) = 0;

    /** Builds the effective address of a packed float32 state-array access.

        Separate from emitStateMem() because a 16-byte access cannot always use
        the same addressing mode as a 4-byte one: AArch64's LDR/STR of a Q
        register only encodes a register offset shifted by 0 or 4, so the
        element index cannot be scaled in the addressing mode at all. */
    virtual YdspMem emitVectorStateMem (int base, int indexValue) = 0;

    /** Builds the effective address of a packed float32 stream access through
        the pre-loaded channel base register. Same rationale as
        emitVectorStateMem(): AArch64 cannot scale a Q-register access. */
    virtual YdspMem emitVectorStreamMem (const YdspGp& base, int indexValue) = 0;

    /** Called before each block is emitted, so a target can drop any addressing
        it memoized for the block just finished. */
    virtual void beginBlock (int blockIndex) { (void) blockIndex; }

    /** Allows a target to inspect the target environment and function
        metadata before planning and emission begin. */
    virtual void configureTarget (const asmjit::Environment& environment, const YdspIrFunction& fn)
    {
        (void) environment;
        (void) fn;
    }

    /** Emits a target-specific zeroing sequence for float/double `+0.0`
        constants, returning true if handled. */
    virtual bool emitZeroFloatConst (const YdspFp& dst, bool is64)
    {
        (void) dst;
        (void) is64;
        return false;
    }

    /** Emits a target-specific zeroing sequence for a vector register,
        returning true if handled. */
    virtual bool emitZeroVector (const YdspFp& dst)
    {
        (void) dst;
        return false;
    }

    /** True when emitZeroVector() can materialize a zero vector without
        reading a scalar zero constant. */
    virtual bool supportsZeroVector() const noexcept { return false; }

    /** True when vector operations on this target use non-destructive 3-operand
        VEX encodings. */
    virtual bool usesVexVectors() const noexcept { return activeVectorWidth > 4; }

    /** Called after the last instruction of a block, ahead of its terminator:
        the place for what a target sets up on the way into a loop. */
    virtual void endBlock (int blockIndex) { (void) blockIndex; }

    /** Called after each instruction that writes a value id, so a target can
        drop anything it memoized from that id's previous contents.

        The IR is not SSA: one value id can be written several times inside a
        single block, which is what a fully unrolled loop does to its induction
        variable. A per-block memo keyed by value id is therefore not safe
        without this, and the failure is silent - a stale scaled index register
        addresses the wrong element rather than failing to assemble. */
    virtual void onValueRedefined (int value) { (void) value; }

    /** Pre-materializes any state addressing the target would otherwise
        recompute at every access, once, in the prologue.

        Only AArch64 needs this: it has no base + index + offset addressing
        mode, so reaching a state array element means adding the (compile-time
        constant) region base to the array pointer first. x86-64 folds the
        whole thing into one addressing mode and does nothing here. */
    virtual void prepareStateAddressing (const YdspIrFunction& fn) { (void) fn; }

    /** Loads, once in the prologue, every increment-and-wrap bound the target
        cannot encode in the compare itself.

        Only AArch64 needs this: its compare immediate is 12 bits wide, so the
        last index of a ring longer than that would otherwise be rebuilt in a
        register on every sample. */
    virtual void prepareWrapBounds (const YdspIrFunction& fn) { (void) fn; }

    virtual void loadGpFromMem (const YdspGp& dst, const YdspMem& src) = 0;
    virtual void storeGpToMem (const YdspMem& dst, const YdspGp& src) = 0;
    virtual void loadFloatFromMem (const YdspFp& dst, const YdspMem& src) = 0;
    virtual void storeFloatToMem (const YdspMem& dst, const YdspFp& src) = 0;
    virtual void moveFloat (const YdspFp& dst, const YdspFp& src) = 0;

    virtual void floatBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) = 0;
    virtual void floatUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src, YdspValueType type) = 0;

    /** dst = a * b + c, with a single rounding (YdspIrOp::fmaF), float32.

        Only called when the target reported the instruction: the optimiser
        expands `fmaF` into float64 arithmetic before codegen otherwise, so a
        backend without one never sees the opcode.

        @see YdspOptimizer::setTargetHasFusedMultiplyAdd
    */
    virtual void emitFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c) = 0;
    virtual void emitFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c) = 0;

    /** dst = a * b + c in packed float32 lanes (YdspIrOp::fmaF). */
    virtual void emitVectorFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c) = 0;
    virtual void emitVectorFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c) = 0;

    //==============================================================================
    // Packed float32 forms, used when the vectoriser widened a value. The lane
    // count belongs to the active IR function and is reflected in the virtual
    // register type. Dispatch happens in emitInstruction() from
    // YdspIrFunction::valueLanes, so a target needs no register introspection
    // to tell a scalar apart from a vector.

    virtual void loadVectorFromMem (const YdspFp& dst, const YdspMem& src) = 0;
    virtual void storeVectorToMem (const YdspMem& dst, const YdspFp& src) = 0;
    virtual void moveVector (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void vectorBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) = 0;
    virtual void vectorUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src) = 0;
    virtual void vectorFloatCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) = 0;
    virtual void vectorSelectFloat (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) = 0;

    virtual void vectorIntBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB, bool saturating) = 0;
    virtual void vectorIntShift (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspGp& shiftAmount) = 0;
    virtual void vectorIntUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src) = 0;
    virtual void vectorIntClamp (const YdspFp& dst, const YdspFp& src, const YdspFp& lo, const YdspFp& hi) = 0;
    virtual void vectorIntCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) = 0;
    virtual void vectorSelectInt (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) = 0;
    virtual void vectorIntToFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void vectorFloatToInt (const YdspFp& dst, const YdspFp& src, bool bounded) = 0;

    /** Every lane of dst = the scalar src (YdspIrOp::vsplat). */
    virtual void emitSplatFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void emitSplatInt (const YdspFp& dst, const YdspGp& src) = 0;

    /** Scalar dst = the sum of src's lanes (YdspIrOp::vreduceAddF). */
    virtual void emitReduceAddFloat (const YdspFp& dst, const YdspFp& src) = 0;

    /** Inserts scalar float32 `srcScalar` into `lane` of `dst`, starting from
        `srcVec` when valid or a zeroed vector otherwise (YdspIrOp::vinsertF). */
    virtual void emitInsertFloat (const YdspFp& dst, const YdspFp& srcVec, const YdspFp& srcScalar, int lane) = 0;

    /** Extracts `lane` from float32 vector `srcVec` into scalar `dst`
        (YdspIrOp::vextractF). */
    virtual void emitExtractFloat (const YdspFp& dst, const YdspFp& srcVec, int lane) = 0;

    /** Reinterprets a scalar float32 register's bits as int32 (YdspIrOp::bitcastFtoI). */
    virtual void emitBitcastFloatToInt (const YdspGp& dst, const YdspFp& src) = 0;

    /** Reinterprets a scalar int32 register's bits as float32 (YdspIrOp::bitcastItoF). */
    virtual void emitBitcastIntToFloat (const YdspFp& dst, const YdspGp& src) = 0;

    /** Adds a compile-time byte offset to a 64-bit GP pointer: dst = src + imm. */
    virtual void emitAddPointerImm (const YdspGp& dst, const YdspGp& src, int32_t imm) = 0;
    virtual void roundFloat (const YdspFp& reg, int mode) = 0;
    virtual void intBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB) = 0;
    virtual void emitSaturatingIntBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB) = 0;
    YdspGp integerSaturationLimit (const YdspGp& a, const YdspGp& b, bool multiply);
    virtual void intUnaryNeg (const YdspGp& dst, const YdspGp& src) = 0;
    void emitSaturatingIntNeg (const YdspGp& dst, const YdspGp& src);
    virtual void emitIntDivision (YdspIrOp op, const YdspGp& dst, const YdspGp& a, const YdspGp& b, bool is64) = 0;
    virtual void emitNotB (const YdspGp& dst, const YdspGp& src) = 0;

    virtual void emitFloatCompare (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst) = 0;
    virtual void emitIntCompare (YdspCond cond, const YdspGp& a, const YdspGp& b, const YdspGp& dst) = 0;
    virtual void emitFloatCompareToReg (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst) = 0;

    /** dst = cond ? whenTrue : whenFalse, without a branch. */
    virtual void emitSelectFloat (const YdspGp& cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) = 0;
    virtual void emitSelectInt (const YdspGp& cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse) = 0;

    /** Compares two operands into the target's condition flags, emitting
        nothing else - so the matching emitSelect*OnFlags() can consume them
        directly instead of going through a 0/1 general-purpose register. */
    virtual void emitFloatCompareToFlags (const YdspFp& a, const YdspFp& b) = 0;
    virtual void emitIntCompareToFlags (const YdspGp& a, const YdspGp& b) = 0;

    /** dst = <flags match cond> ? whenTrue : whenFalse. Must be emitted
        directly after the matching emitFloatCompareToFlags/emitIntCompareToFlags.

        Nothing may come between the two, which also means the register
        allocator must not be able to break the pair. It cannot: everything it
        inserts around an instruction is a move, a spill or a reload, and none
        of those touch the flags on either target - the same assumption the
        existing compare-then-`cset`/`setcc` sequences already rely on. */
    virtual void emitSelectFloatOnFlags (YdspCond cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) = 0;
    virtual void emitSelectIntOnFlags (YdspCond cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse) = 0;

    /** dst = (value >= bound) ? 0 : value, without a branch (see YdspIrOp::wrapI). */
    virtual void emitWrapInt (const YdspGp& dst, const YdspGp& value, const YdspGp& bound) = 0;
    virtual void emitAdvanceWrapInt (const YdspGp& dst, const YdspGp& value, int32_t bound) = 0;

    virtual void emitIntToFloat (const YdspFp& dst, const YdspGp& src) = 0;
    virtual void emitFloatToInt (const YdspGp& dst, const YdspFp& src, bool bounded) = 0;
    virtual void emitExtendInt (const YdspGp& dst, const YdspGp& src) = 0;
    virtual void emitTruncateInt (const YdspGp& dst, const YdspGp& src) = 0;
    virtual void emitExtendFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void emitTruncateFloat (const YdspFp& dst, const YdspFp& src) = 0;

    virtual void jump (const asmjit::Label& target) = 0;
    virtual void branchIfZero (const YdspGp& cond, const asmjit::Label& target) = 0;
    virtual void branchIfNotZero (const YdspGp& cond, const asmjit::Label& target) = 0;

    /** Jumps to `target` when the condition flags - left by the immediately
        preceding emitFloatCompareToFlags()/emitIntCompareToFlags() - satisfy
        `cond`, or fail to satisfy it when `branchWhenTrue` is false.

        This is the branch counterpart of the select-on-flags forms: a
        branchIfZero/branchIfNotZero first materialises the comparison into a
        0/1 register (setcc/cset) and tests it, and fusion lets the branch
        consume the flags directly instead. It must be emitted back-to-back
        with the compare-to-flags: the pair carries no register operands
        between them, so the allocator has nothing it could insert in between. */
    virtual void branchOnFlags (YdspCond cond, bool branchWhenTrue, const asmjit::Label& target) = 0;

    virtual YdspFp newFp (const char* name) = 0;
    virtual YdspFp newFp64 (const char* name) = 0;
    virtual YdspFp newFpVector (const char* name) = 0;
    virtual YdspFp newIntVector (const char* name) = 0;

    virtual YdspFp newFp128 (const char* name) { return newFpVector (name); }
    virtual YdspFp newInt128 (const char* name) { return newIntVector (name); }

    //==============================================================================
    // Shared helpers (architecture-independent)

    bool isVectorValue (int value) const noexcept;
    YdspFp newFpOfType (YdspValueType type, const char* name);
    asmjit::Reg newRegFor (const std::vector<YdspValueType>& types, const YdspIrInst& inst);
    YdspGp gp (int value) const;
    YdspFp fp (int value) const;
    void moveGp (const YdspGp& dst, const YdspGp& src);

    YdspMem rawConstMem (uint64_t bits, bool is64);
    YdspMem packedConstMem (uint64_t bits, bool is64);
    YdspMem vectorConstMem (uint32_t bits);
    YdspMem rampConstMem();

    void computeLoopScopes (const YdspIrFunction& fn);
    void prepareRampConsts (const YdspIrFunction& fn);

    /** The channel pointer of a stream for an access in the given block. */
    YdspGp streamBase (int blockIndex, bool isInput, int slot) const;

    /** What one reference in the given block counts for when ranking
        registers: ten times more per enclosing loop. */
    double referenceCost (int blockIndex) const noexcept;

    /** The allocator weight (1 to 240) of a register with that many
        loop-weighted references. Pinned registers use 255. */
    static uint32_t weightFor (double references) noexcept;

    void planCompareFusion (const YdspIrFunction& fn);
    void emitPlannedIntCompareToFlags (const YdspIrInst& compare);
    void planOperandLowering (const YdspIrFunction& fn);

    /** Finds the subtractions (and additions) whose own flags can stand in
        for a following `x < 0` / `x >= 0` test of their result, which is the
        hand-written ring wrap `r = a - b; if (r < 0) { r = r + n; }`.

        On AArch64 arithmetic leaves flags untouched unless asked; on x86-64
        intervening additions/subtractions with immediates are lowered via `lea`
        so the flags survive through to the select. */
    void planFlagReuse (const YdspIrFunction& fn);

    /** Finds the loops whose induction only indexes streams, and makes them
        count from `-n` up to zero against the end of each stream.

        The increment then closes the loop with its own flags, which removes
        the compare from every sample. */
    void planEndRelativeLoops (const YdspIrFunction& fn);
    void emitEndRelativeLoopSetup (int preheaderBlock);

    /** Strength-reduces induction-indexed state-array and stream accesses in
        rolled loops (not already claimed by planEndRelativeLoops()) into
        advancing base pointer GP registers initialized in the preheader and
        stepped at the latch. */
    void planLoopPointers (const YdspIrFunction& fn);
    void emitLoopPointerSetup (int preheaderBlock);
    void emitLoopPointerAdvance (int latchBlock);

    void emitFlagSettingArithmetic (const YdspIrInst& inst, YdspInstPosition position);

    void materializeMathCallTargets (const YdspIrFunction& fn);
    void loadExternalSymbol (const YdspGp& target, const void* address);
    void loadExternalSymbol (const YdspGp& target, const String& name);

    void loadFloatConst (const YdspFp& dst, double value, YdspValueType type);
    void emitStateAccess (YdspValueType type, bool isLoad, const asmjit::Reg& reg, int base, int indexValue);
    void emitInstruction (const YdspIrFunction& fn, const YdspIrInst& inst, int blockIndex, int instIndex);
    void emitTerminator (const YdspIrBlock& block, int blockIndex);

    void emitLibmUnary (float (*f32fn) (float), double (*f64fn) (double), const YdspIrInst& inst);
    void emitLibmBinary (float (*f32fn) (float, float), double (*f64fn) (double, double), const YdspIrInst& inst);
    void emitEmitEvent (const YdspIrInst& inst);

    /** Lowers a float32/float64 transcendental (unary or binary) for the value
        width `inst.result` carries.

        Scalar float32 values stay on libm (per-target tuned; measured SLEEF
        scalar calls regressed against it).
        Widened float32 values call the 4-lane SLEEF set, at `u35` under
        fastMath and `u10` otherwise; on x86-64 an 8-lane value splits into two
        4-lane calls.
    */
    void emitTranscendentalCall (const YdspIrFunction& fn, const YdspIrInst& inst, bool isBinary);

    /** Inlines the fastMath scalar float32 exp on AArch64.

        e^x is entire, so a degree-8 polynomial is ~1 ulp across [-1, 1]. The
        poly is evaluated Estrin-style (two degree-3 chunks plus x^4 and x^8
        corrections) to keep the serial FMA depth at four instead of nine, and
        arguments outside [-1, 1] take a rare, predictable branch back to the
        libm call. Exists so the straight-line form can be benchmarked against
        the libm scalar call; x86-64 keeps the call for now. */
    void emitInlineFastExp (const YdspIrInst& inst);

    /** One 4-lane SLEEF call; a widened value wider than 4 lanes fans out into
        one of these per 4-lane chunk on the target that carries it. */
    void emitVectorMathInvoke (const String& symbol, bool isBinary, const YdspFp& ret, const YdspFp& a, const YdspFp& b);

    int stateScalarBase (YdspValueType type, int slot) const;
    int stateArrayBase (YdspValueType type, int element) const;

    //==============================================================================
    // Shared state

    YdspAsm* cc = nullptr;

    YdspGp inputsReg;
    YdspGp outputsReg;
    YdspGp paramsReg;
    YdspGp paramOutReg;
    YdspGp stateReg;
    YdspGp stateArraysReg;
    YdspGp numSamplesReg;
    YdspFp sampleRateReg;
    YdspGp outputEventsReg;
    YdspGp traceQueueReg;

    /** The outermost loop each block sits in, as an index into the function's
        loops, or -1: what a loop sets up in its preheader is keyed by this.
        Only a loop that is entered through its preheader alone counts. */
    std::vector<int> blockLoopScope;
    int currentBlock = 0;

    // Scope, input or output, slot.
    using StreamBaseKey = std::tuple<int, bool, int>;

    std::map<StreamBaseKey, YdspGp> streamBaseRegs;
    std::map<int, std::vector<StreamBaseKey>> loopStreamBases; // by preheader block
    std::map<YdspInstPosition, YdspFp> vrampRegs;
    std::map<int, std::vector<YdspFp>> loopRampConsts; // by preheader block

    const YdspIrFunction* activeFn = nullptr;

    bool isEventHandler = false;
    int activeVectorWidth = YdspVectorizer::vectorWidth;

    /** Scratch stack buffer (64 bytes) reused by every 4-lane SLEEF call site.
        The 8-lane split stages only the operands that must outlive a call
        through it, so no vector value is left live in a register across a call
        - the calls do not preserve the vector register file. */
    YdspMem sleefScratch;
    bool sleefScratchReady = false;

    YdspGp eventCtxReg;

    std::vector<int> paramOffsets;
    std::vector<int> paramOutOffsets;
    std::vector<YdspValueType> valueTypes;
    std::vector<int> valueLanes;
    std::vector<asmjit::Label> blockLabels;
    std::vector<int> blockLoopDepth;
    std::vector<asmjit::Reg> regs;

    std::map<YdspInstPosition, YdspIrInst> fusedSelectCompares;
    std::map<int, YdspIrInst> fusedBranchCompares;

    std::unordered_map<const void*, YdspGp> libmTargetRegs;
    std::map<String, YdspGp> vectorMathTargetRegs;

    std::map<String, asmjit::Label> externalSymbols;

    std::set<YdspInstPosition> elidedCompares;
    std::map<YdspInstPosition, int64_t> integerImmediates;
    std::map<YdspInstPosition, int32_t> arrayDisplacements;
    std::map<int, int64_t> integerCompareImmediates;
    std::set<int> elidedIntegerConstants;
    std::set<int> elidedFloatConstants;
    std::set<YdspInstPosition> zeroVectorSplats;
    std::map<YdspInstPosition, int> fusedProductSubtracts;
    std::set<YdspInstPosition> elidedNegations;

    /** A loop that counts up to zero: see planEndRelativeLoops(). */
    struct EndRelativeLoop
    {
        int headerBlock = -1;
        int latchBlock = -1;
        int induction = -1;
        int bound = -1;
        YdspInstPosition seed { -1, -1 };

        // Stream slot (inputs first, then outputs offset by the input count)
        // to the address one past the stream's last sample.
        std::map<int, YdspGp> streamEnds;
        std::map<int, uint32_t> streamScales;
    };

    std::vector<EndRelativeLoop> endRelativeLoops;

    struct LoopPointer
    {
        enum class Kind
        {
            stateArray,
            stream
        };

        Kind kind = Kind::stateArray;
        int baseOffset = 0;
        bool isInput = false;
        int streamSlot = -1;
        int streamAccessBlock = -1;
        int offsetValue = -1;
        uint32_t elemBytes = 4;
        YdspGp ptrReg;
    };

    struct AdvancingPointerLoop
    {
        int headerBlock = -1;
        int latchBlock = -1;
        int induction = -1;
        int step = 1;
        int startValue = -1;
        std::optional<int64_t> startConst;
        std::vector<LoopPointer> pointers;
    };

    std::vector<AdvancingPointerLoop> advancingPointerLoops;
    std::map<YdspInstPosition, YdspGp> loopPointerAccesses;
    std::set<YdspInstPosition> elidedLoopOffsetAdds;

    std::set<YdspInstPosition> flagSettingArithmetic;
    std::set<YdspInstPosition> flagPreservingArithmetic;
    std::map<YdspInstPosition, bool> flagReusingSelects; // select -> true when it tests `x < 0`
};

} // namespace yup
