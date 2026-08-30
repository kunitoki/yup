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
// Internal per-architecture type aliases shared by the native (asmjit) backends.
#if ASMJIT_ARCH_X86
using YdspAsm = asmjit::x86::Compiler;
using YdspGp = asmjit::x86::Gp;
using YdspFp = asmjit::x86::Vec;
using YdspMem = asmjit::x86::Mem;
using YdspCond = asmjit::x86::CondCode;
#elif ASMJIT_ARCH_ARM
using YdspAsm = asmjit::a64::Compiler;
using YdspGp = asmjit::a64::Gp;
using YdspFp = asmjit::a64::Vec;
using YdspMem = asmjit::a64::Mem;
using YdspCond = asmjit::arm::CondCode;
#else
#error "yup_dsp_jit requires an x86-64 or AArch64 backend"
#endif

//==============================================================================
/** Compiles one YdspIrFunction into native machine code via AsmJit.

    This is the architecture-independent facade of the native backend; the
    actual lowering is split per target:

      - YdspAsmJitCodegenX64   - x86-64 (SSE) lowering
      - YdspAsmJitCodegenARM64 - AArch64 (ASIMD) lowering

    The matching backend is selected automatically from the host architecture.
    Available on desktop targets only (the wasm backend serves YUP_WASM).

    Uses the AsmJit Compiler API (virtual registers + built-in register
    allocation) on the host architecture.
*/
class YdspAsmJitCodegen
{
public:
    /** Compiles the IR function; returns nullptr and records a diagnostic on failure. */
    static YdspKernelFn compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize = nullptr);

    /** Compiles an event-handler IR function; returns nullptr and records a diagnostic on failure. */
    static YdspEventHandlerFn compileEventHandler (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize = nullptr);

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
class YdspAsmJitCodegenImpl
{
public:
    /** Compiles the IR function; returns nullptr and records a diagnostic on failure. */
    YdspKernelFn compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, bool isEventHandler = false, size_t* generatedCodeSize = nullptr);

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

    virtual void loadGpFromMem (const YdspGp& dst, const YdspMem& src) = 0;
    virtual void storeGpToMem (const YdspMem& dst, const YdspGp& src) = 0;
    virtual void loadFloatFromMem (const YdspFp& dst, const YdspMem& src) = 0;
    virtual void storeFloatToMem (const YdspMem& dst, const YdspFp& src) = 0;
    virtual void moveFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void moveGpToFp (const YdspFp& dst, const YdspGp& src) = 0;

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

    /** Every lane of dst = the scalar src (YdspIrOp::vsplat). */
    virtual void emitSplatFloat (const YdspFp& dst, const YdspFp& src) = 0;

    /** Scalar dst = the sum of src's lanes (YdspIrOp::vreduceAddF). */
    virtual void emitReduceAddFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void roundFloat (const YdspFp& reg, int mode) = 0;
    virtual void intBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB) = 0;
    virtual void intUnaryNeg (const YdspGp& dst, const YdspGp& src) = 0;
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
    virtual void emitFloatToInt (const YdspGp& dst, const YdspFp& src) = 0;
    virtual void emitExtendInt (const YdspGp& dst, const YdspGp& src) = 0;
    virtual void emitTruncateInt (const YdspGp& dst, const YdspGp& src) = 0;
    virtual void emitExtendFloat (const YdspFp& dst, const YdspFp& src) = 0;
    virtual void emitTruncateFloat (const YdspFp& dst, const YdspFp& src) = 0;

    virtual void jump (const asmjit::Label& target) = 0;
    virtual void branchIfZero (const YdspGp& cond, const asmjit::Label& target) = 0;
    virtual void branchIfNotZero (const YdspGp& cond, const asmjit::Label& target) = 0;

    virtual YdspFp newFp (const char* name) = 0;
    virtual YdspFp newFp64 (const char* name) = 0;
    virtual YdspFp newFpVector (const char* name) = 0;
    virtual YdspFp newFp128 (const char* name) = 0;

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

    void planCompareSelectFusion (const YdspIrFunction& fn);

    void loadFloatConst (const YdspFp& dst, double value, YdspValueType type);
    void emitStateAccess (YdspValueType type, bool isLoad, const asmjit::Reg& reg, int base, int indexValue);
    void emitInstruction (const YdspIrFunction& fn, const YdspIrInst& inst, int blockIndex, int instIndex);
    void emitTerminator (const YdspIrBlock& block, int blockIndex);

    void emitLibmUnary (float (*f32fn) (float), double (*f64fn) (double), const YdspIrInst& inst);
    void emitLibmBinary (float (*f32fn) (float, float), double (*f64fn) (double, double), const YdspIrInst& inst);
    void emitEmitEvent (const YdspIrInst& inst);

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

    // One register per stream, holding inputs[i] / outputs[i] for the whole
    // kernel: without them every stream access re-loads the channel pointer
    // from the context, a dependent load per access per sample.
    std::vector<YdspGp> inputBaseRegs;
    std::vector<YdspGp> outputBaseRegs;

    // Segment byte offsets (region order [f32][i32][f64][i64] in each).
    int int32ScalarOffset = 0; // scalar region starts within the scalar segment
    int float64ScalarOffset = 0;
    int int64ScalarOffset = 0;
    int int32ArrayOffset = 0; // array region starts within the array segment
    int float64ArrayOffset = 0;
    int int64ArrayOffset = 0;

    bool isEventHandler = false;
    int activeVectorWidth = YdspVectorizer::vectorWidth;
    YdspGp eventCtxReg; // the YdspEventContext pointer, addressed by byte offset

    std::vector<int> paramOffsets;    // byte offset per param slot (heterogeneous)
    std::vector<int> paramOutOffsets; // byte offset per meter slot (heterogeneous)

    std::vector<YdspValueType> valueTypes;

    // Lane count per value id, empty when the vectoriser did not run.
    std::vector<int> valueLanes;

    std::vector<asmjit::Label> blockLabels;
    std::vector<asmjit::Reg> regs;

    // planCompareSelectFusion(), keyed by (block, instruction) rather than by
    // value id: an if-converted select writes a mutable local, which the IR
    // defines more than once, so its result does not identify it.
    using YdspInstPosition = std::pair<int, int>;

    std::map<YdspInstPosition, YdspIrInst> fusedSelectCompares;
    std::set<YdspInstPosition> elidedCompares;
};

} // namespace yup
