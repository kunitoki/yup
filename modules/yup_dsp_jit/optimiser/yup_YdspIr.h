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
/** The instruction set of the YDSP typed IR.

    The IR is a non-SSA, block-structured intermediate representation with
    mutable virtual registers (value ids). Instructions write their result to
    `result` (a value id) or have side effects (stores, terminators).
*/
enum class YdspIrOp
{
    // Constants
    constF,
    constI,
    constB,

    // Runtime values
    loadBlockSize,  // i32
    loadSampleRate, // float
    // Payload fields of the dispatching event, addressed by their byte offset
    // in `memIndex` (valid only inside an event handler).
    loadEventFieldF, // float
    loadEventFieldI, // i32

    // Output events. The two stores stage one payload field of the pending emit
    // record, addressed by the same YdspEventContext-relative byte offset the
    // loads use (see ydspEventShapes); emitEvent then commits that record to the
    // output-event queue.
    storeEventFieldF, // float: stages a at byte offset memIndex
    storeEventFieldI, // i32: stages a at byte offset memIndex
    emitEvent,        // commits: shape = ivalue, sampleOffset = a, endpoint = memIndex

    // Parameters (input value endpoints) and meters (output value endpoints).
    // Loads carry the slot index in `a`, stores in `memIndex`.
    loadParam,     // float params[a]
    storeParam,    // float params[memIndex] = a          (block-mode automation)
    loadParamOut,  // float paramOut[a]
    storeParamOut, // float paramOut[memIndex] = a

    // State (history) memory
    loadStateF,       // float stateScalars[a]
    storeStateF,      // float stateScalars[memIndex] = a
    loadStateI,       // int stateInts[a]
    storeStateI,      // int stateInts[memIndex] = a
    loadStateArrayF,  // float stateFloatArrays[memIndex][a]
    storeStateArrayF, // float stateFloatArrays[memIndex][a] = b
    loadStateArrayI,  // int stateIntArrays[memIndex][a]
    storeStateArrayI, // int stateIntArrays[memIndex][a] = b

    // Audio streams
    loadInput,   // float inputs[memIndex][a]
    loadOutput,  // float outputs[memIndex][a]   (reading an output stream)
    storeOutput, // float outputs[memIndex][a] = b

    // Float arithmetic
    addF,
    subF,
    mulF,
    divF,
    modF,
    negF,

    // Int arithmetic
    addI,
    subI,
    mulI,
    divI,
    modI,
    negI,

    // Ring-buffer wrap: `(a >= b) ? 0 : a`. For the `@` delay primitive, whose
    // write pointer is always within [0, b - 1] before the increment, this is
    // exactly `a % b` - but it lowers to a compare plus a conditional move
    // instead of an integer division (a helper *call* on x86-64).
    wrapI,
    advanceWrapI, // increment a and wrap to zero at immediate bound

    minI,
    maxI,
    absI,
    clampI, // clamp(a, b, c) = min(max(a, b), c)
    signI,  // -1 / 0 / 1

    // Int bitwise
    andI, // a & b
    orI,  // a | b
    xorI, // a ^ b
    shlI, // a << b
    shrI, // a >> b (arithmetic)

    // Comparisons
    eqF,
    neF,
    ltF,
    leF,
    gtF,
    geF,
    eqI,
    neI,
    ltI,
    leI,
    gtI,
    geI,

    // Logic
    andB,
    orB,
    notB,

    // Conversions
    itof,   // int32/int64 -> float32/float64 (width from the value types)
    ftoi,   // float32/float64 -> int32/int64 (width from the value types)
    extI,   // int32 -> int64 (sign-extend)
    truncI, // int64 -> int32 (truncate low bits)
    extF,   // float32 -> float64
    truncF, // float64 -> float32

    // Float intrinsics (unary)
    absF,
    sqrtF,
    floorF,
    ceilF,
    rintF,
    sinF,
    cosF,
    tanF,
    asinF,
    acosF,
    atanF,
    sinhF,
    coshF,
    tanhF,
    expF,
    logF,
    log10F,
    signF,

    // Float intrinsics (binary / ternary)
    powF,
    minF,
    maxF,
    fmodF,
    atan2F,
    clampF,
    lerpF,
    selectB, // a = cond (bool), b, c

    // a * b + c with a *single* rounding - the one operation in this IR whose
    // result a backend cannot reach by composing the others.
    //
    // float32 only, and that restriction is what makes it portable rather than
    // a target-specific fast path: AArch64 and FMA3 x86-64 have it in hardware,
    // and every other target reaches the identical value by computing the whole
    // expression in float64 and rounding once at the end (2p + 2 = 50 bits fit
    // in float64's 53, so the double rounding is provably innocuous for normal
    // results). No such fallback exists for float64 - it would need float128 -
    // so `fma()` on float64 operands is rejected in the analyzer rather than
    // silently producing a different answer on different backends.
    //
    // @see YdspOptimizer::lowerFusedMultiplyAdd, YdspOptimizer::contractMultiplyAdd
    fmaF, // a * b + c
    fmsubF, // c - a * b

    // Additional float intrinsics
    asinhF,
    acoshF,
    atanhF,
    roundF,
    copysignF,

    // Moves (for mutable locals)
    movF,
    movI,
    movB,

    // Lane movement (the only opcodes whose operand and result lane counts
    // differ - every other opcode is reused as-is at `lanes > 1`, so `addF` on
    // a 4-lane value *is* a packed add).
    vsplat,      // every lane of the result = the scalar a
    vreduceAddF, // scalar result = sum of a's lanes (reassociates - see the vectoriser)
};

//==============================================================================
/** Infers the storage type of a value id from its producing opcode.

    Used as a defensive fallback when the optimiser did not persist explicit
    value types; the optimiser normally fills YdspIrFunction::valueTypes, which
    is authoritative. bool values are stored as i32.
*/
inline YdspValueType inferTypeFromOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::constF:
        case YdspIrOp::loadSampleRate:
        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::modF:
        case YdspIrOp::negF:
        case YdspIrOp::itof:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::asinF:
        case YdspIrOp::acosF:
        case YdspIrOp::atanF:
        case YdspIrOp::sinhF:
        case YdspIrOp::coshF:
        case YdspIrOp::tanhF:
        case YdspIrOp::asinhF:
        case YdspIrOp::acoshF:
        case YdspIrOp::atanhF:
        case YdspIrOp::roundF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::signF:
        case YdspIrOp::powF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::fmodF:
        case YdspIrOp::atan2F:
        case YdspIrOp::copysignF:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::movF:
        case YdspIrOp::extF:
        case YdspIrOp::truncF:
        case YdspIrOp::vsplat:
        case YdspIrOp::vreduceAddF:
            return YdspValueType::float32Type;

        case YdspIrOp::constI:
        case YdspIrOp::loadBlockSize:
        case YdspIrOp::loadEventFieldI:
        case YdspIrOp::loadStateI:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::addI:
        case YdspIrOp::subI:
        case YdspIrOp::mulI:
        case YdspIrOp::divI:
        case YdspIrOp::modI:
        case YdspIrOp::wrapI:
        case YdspIrOp::advanceWrapI:
        case YdspIrOp::negI:
        case YdspIrOp::minI:
        case YdspIrOp::maxI:
        case YdspIrOp::absI:
        case YdspIrOp::clampI:
        case YdspIrOp::signI:
        case YdspIrOp::andI:
        case YdspIrOp::orI:
        case YdspIrOp::xorI:
        case YdspIrOp::shlI:
        case YdspIrOp::shrI:
        case YdspIrOp::ftoi:
        case YdspIrOp::extI:
        case YdspIrOp::truncI:
        case YdspIrOp::movI:
            return YdspValueType::int32Type;

        default:
            return YdspValueType::boolType;
    }
}

//==============================================================================
/** A single IR instruction (compact node; field meaning depends on op). */
struct YdspIrInst
{
    YdspIrOp op = YdspIrOp::constF;

    int result = -1; // value id written, or -1

    int a = -1; // operand value ids
    int b = -1;
    int c = -1;

    int memIndex = -1; // param/state-slot/stream index, or array region base

    double fvalue = 0.0; // constF payload
    int64_t ivalue = 0;  // constI payload
    bool bvalue = false; // constB payload
};

//==============================================================================
/** A basic block: straight-line instructions plus a terminator. */
enum class YdspIrTerm
{
    fallthrough, // continue to the next block
    branch,      // jump to target
    branchIf     // jump to target if cond, else target2
};

struct YdspIrBlock
{
    std::vector<YdspIrInst> insts;

    YdspIrTerm term = YdspIrTerm::fallthrough;
    int termCond = -1;
    int termTarget = -1;
    int termTarget2 = -1;
};

//==============================================================================
/** A structured bounded loop. */
struct YdspIrLoop
{
    int id = -1;
    int headerBlock = -1; // induction compare + branchIf(body, exit)
    int exitBlock = -1;   // first block after the loop
    int induction = -1;   // value id of the induction variable (i32)
    YdspLoopBound bound;

    // Set when the loop has been fully unrolled into its preheader. The entry
    // stays so the report can still answer "how many iterations could this run",
    // but the header and body blocks are empty and fall through, so there is no
    // longer a region for a backend to recover around them.
    bool unrolled = false;
};

//==============================================================================
/** A compiled kernel function: typed IR plus resource layout. */
struct YdspIrFunction
{
    String name;

    bool isSampleMode = true;
    bool isInit = false; // one-shot init kernel (no sample/block loop)

    // Event-handler functions (lowered `event <input> (<param>: <shape>) { ... }`).
    bool isEventHandler = false;
    YdspEventShape eventShape = YdspEventShape::noteOn; // the dispatching shape
    String eventInputName;                              // the `input event` channel the handler binds to
    String ownerProcessorName;                          // the processor whose kernel shares the state layout

    int numParams = 0;    // input value endpoints
    int numParamsOut = 0; // output value endpoints
    int numInputs = 0;    // input streams
    int numOutputs = 0;   // output streams

    int float32Scalars = 0;       // f32 state scalars + hidden prev/delay slots
    int float64Scalars = 0;       // f64 state scalars
    int int32Scalars = 0;         // i32 state scalars + hidden ring write pointers
    int int64Scalars = 0;         // i64 state scalars
    int float32ArrayElements = 0; // total f32 elements across arrays + hidden rings
    int float64ArrayElements = 0; // total f64 elements across arrays
    int int32ArrayElements = 0;   // total i32 elements across arrays
    int int64ArrayElements = 0;   // total i64 elements across arrays

    // Byte offset within a voice's scalar segment of the processor's
    // `[[ role: voiceActivity ]]` flag, or -1 when it declares none. The
    // runtime reads it to skip a released voice whose flag is 0.
    int activityByteOffset = -1;

    // Per-endpoint element types (drive codegen addressing and the host runtime).
    std::vector<YdspValueType> inputTypes;    // input streams, in order
    std::vector<YdspValueType> outputTypes;   // output streams, in order
    std::vector<YdspValueType> paramTypes;    // input value endpoints, in order
    std::vector<YdspValueType> paramOutTypes; // output value endpoints, in order

    // The value type of every value id (indexed by value id). Populated by the
    // optimiser; the codegen dispatches on operand/result width.
    std::vector<YdspValueType> valueTypes;

    // The lane count of every value id, parallel to valueTypes. Left empty
    // unless the vectoriser ran, and 1 for every value it did not widen, so
    // laneCountOf() answers 1 for an untouched function.
    std::vector<int> valueLanes;

    // Set by the vectoriser when it widened at least one loop.
    bool vectorized = false;
    int vectorWidth = 1;

    // Populated by the vectoriser whenever it runs: one entry per original
    // loop (see YdspVectorizationResult), recording whether it was widened or
    // the exact reason it stayed scalar. Empty when the vectoriser did not run.
    std::vector<YdspVectorizationResult> vectorizationResults;

    // Set when a widened accumulator was halved. Unlike the loop flags this is
    // per-function rather than per-loop: the chain it rewrites lives in a block,
    // and after unrolling that block no longer belongs to any loop.
    bool reductionSplit = false;

    std::vector<YdspIrBlock> blocks;
    std::vector<YdspIrLoop> loops;

    /** Returns the lane count of a value id (1 unless the vectoriser widened it). */
    int laneCountOf (int value) const noexcept
    {
        if (value < 0 || static_cast<size_t> (value) >= valueLanes.size())
            return 1;

        return valueLanes[static_cast<size_t> (value)];
    }

    /** Returns the total number of instructions across all blocks. */
    int getInstructionCount() const noexcept;

    /** Returns the state memory size in bytes for the given function.

        Covers the scalar segment (every scalar slot) plus the array segment
        (every array element: delay lines, rings). This layout is shared by
        every codegen backend and by the host runtime, which allocates state
        from it. */
    size_t stateSize() const noexcept
    {
        const auto f32 = static_cast<size_t> (float32Scalars) + static_cast<size_t> (float32ArrayElements);
        const auto f64 = static_cast<size_t> (float64Scalars) + static_cast<size_t> (float64ArrayElements);
        const auto i32 = static_cast<size_t> (int32Scalars) + static_cast<size_t> (int32ArrayElements);
        const auto i64 = static_cast<size_t> (int64Scalars) + static_cast<size_t> (int64ArrayElements);

        return f32 * 4 + f64 * 8 + i32 * 4 + i64 * 8;
    }

    /** Returns the scalar segment's size in bytes (stateArrays = state + this).

        May be 4-mod-8 for odd f32+i32 scalar counts; the array segment base is
        deliberately not 8-aligned (both native targets and wasm permit
        unaligned accesses). */
    size_t stateScalarSize() const noexcept
    {
        return static_cast<size_t> (float32Scalars * 4 + int32Scalars * 4
                                    + float64Scalars * 8 + int64Scalars * 8);
    }
};

//==============================================================================
/** The per-kernel result of the optimiser. */
struct YdspIrFunctionResult
{
    std::unique_ptr<YdspIrFunction> function;
    YdspAnalyzedProcessor analyzed; // copy of the analyzed processor (reporting)
};

} // namespace yup
