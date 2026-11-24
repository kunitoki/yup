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

namespace yup
{

//==============================================================================

size_t YdspAsmJitCodegen::stateSize (const YdspIrFunction& fn)
{
    return fn.stateSize();
}

size_t YdspAsmJitCodegen::stateScalarSize (const YdspIrFunction& fn)
{
    return fn.stateScalarSize();
}

//==============================================================================

namespace
{

// Native kernel/event headers contain 64-bit pointers on both target ISAs.
#if ! YUP_WASM
static_assert (offsetof (YdspKernelContext, outputEvents) == 56);
static_assert (offsetof (YdspEventContext, outputEvents) == 72);
#endif

constexpr int nativeEventPayloadOffset (int hostOffset) noexcept
{
    return hostOffset - static_cast<int> (offsetof (YdspEventContext, pitch)) + 28;
}

String nativeSymbolName (const void* address);
#if ! YUP_WASM
const void* nativeSymbolAddress (const String& name);
#endif

uint32_t floatBits (double value)
{
    float f = static_cast<float> (value);
    uint32_t bits = 0;
    std::memcpy (&bits, &f, sizeof (bits));
    return bits;
}

uint64_t doubleBits (double value)
{
    uint64_t bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));
    return bits;
}

// The condition each comparison opcode tests. Float comparisons use the
// unsigned codes: comiss/fcmp report an ordered result through the carry flag,
// so the unsigned forms are the ones that exclude NaN.
template <typename YdspCond>
YdspCond conditionForComparison (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqF:
        case YdspIrOp::eqI:
            return YdspCond::kEqual;
        case YdspIrOp::neF:
        case YdspIrOp::neI:
            return YdspCond::kNotEqual;

        case YdspIrOp::ltF:
        case YdspIrOp::ltUI:
            return YdspCond::kUnsignedLT;
        case YdspIrOp::leF:
            return YdspCond::kUnsignedLE;
        case YdspIrOp::gtF:
            return YdspCond::kUnsignedGT;
        case YdspIrOp::geF:
            return YdspCond::kUnsignedGE;

        case YdspIrOp::ltI:
            return YdspCond::kSignedLT;
        case YdspIrOp::leI:
            return YdspCond::kSignedLE;
        case YdspIrOp::gtI:
            return YdspCond::kSignedGT;
        case YdspIrOp::geI:
            return YdspCond::kSignedGE;

        default:
            return YdspCond::kEqual;
    }
}

//==============================================================================
// Captures detailed error info from asmjit's ErrorHandler callback
class AsmJitErrorCollector : public asmjit::ErrorHandler
{
public:
    void handle_error (asmjit::Error err, const char* message, asmjit::BaseEmitter* origin) override
    {
        lastError = err;
        if (message != nullptr)
            lastMessage = message;
    }

    asmjit::Error lastError = asmjit::kErrorOk;
    String lastMessage;
};

//==============================================================================
// Removes moves whose destination and source are the same register.

// asmjit's register allocator already coalesces most of its own moves; this
// pass deletes the no-op moves that survive it - a move-family instruction
// whose two register operands are identical. Despite what the mirrored
// hi_snex AsmCleanupPass comment claimed, this runs *after* the register
// allocator: x86::Compiler/A64::Compiler attach their RA pass at attach time,
// and add_pass() appends to the end of the list, so this is a post-allocation
// peephole over physical registers. That is safe here: the only instructions
// it deletes are `mov wN,wN` / `mov eax,eax` zero-extensions (unobservable,
// int32 values are never read through their 64-bit view), identical
// `fmov sN,sN` / `mov vN.16b,vN.16b` float copies, and the mov/movaps/...
// family - none of which carries a side effect.
template <bool isX64>
class RedundantMoveCleanupPass final : public asmjit::FuncPass
{
public:
    explicit RedundantMoveCleanupPass (asmjit::BaseCompiler& cc) noexcept
        : asmjit::FuncPass (cc, "ydsp-redundant-move-cleanup")
    {
    }

    asmjit::Error run_on_function (asmjit::axl::Arena&, asmjit::Logger*, asmjit::FuncNode* func) override
    {
        auto* node = func->next();
        const auto* end = func->end_node();

        while (node != nullptr && node != end)
        {
            if (node->type() == asmjit::NodeType::kInst)
            {
                auto* inst = node->as<asmjit::InstNode>();

                if (inst->op_count() >= 2 && isMove (inst->inst_id()) && inst->op (0) == inst->op (1))
                {
                    auto* victim = node;
                    node = node->next();
                    cc().remove_nodes (victim, victim);
                    continue;
                }
            }

            node = node->next();
        }

        return asmjit::kErrorOk;
    }

private:
    static bool isMove (asmjit::InstId instId) noexcept
    {
        if constexpr (isX64)
        {
            static constexpr asmjit::InstId moveIds[] = {
                asmjit::x86::Inst::kIdMov,
                asmjit::x86::Inst::kIdMovaps,
                asmjit::x86::Inst::kIdMovups,
                asmjit::x86::Inst::kIdMovapd,
                asmjit::x86::Inst::kIdMovupd,
                asmjit::x86::Inst::kIdMovdqa,
                asmjit::x86::Inst::kIdMovdqu,
                asmjit::x86::Inst::kIdVmovaps,
                asmjit::x86::Inst::kIdVmovups,
                asmjit::x86::Inst::kIdVmovdqa,
                asmjit::x86::Inst::kIdVmovdqu,
                asmjit::x86::Inst::kIdMovq,
                asmjit::x86::Inst::kIdMovss,
                asmjit::x86::Inst::kIdMovsd,
            };
            for (const auto id : moveIds)
                if (instId == id)
                    return true;
        }
        else
        {
            static constexpr asmjit::InstId moveIds[] = {
                asmjit::a64::Inst::kIdMov,
                asmjit::a64::Inst::kIdFmov_v,
                asmjit::a64::Inst::kIdMov_v,
            };
            for (const auto id : moveIds)
                if (instId == id)
                    return true;
        }
        return false;
    }
};

} // namespace

//==============================================================================

template <typename Traits>
ResultValue<YdspNativeArtifact> YdspAsmJitCodegenImpl<Traits>::emit (const asmjit::Environment& environment, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, bool isEventHandler)
{
    asmjit::CodeHolder code;

    // Attach diagnostics so asmjit logs are captured for error reporting
    AsmJitErrorCollector errorCollector;
    asmjit::StringLogger logger;
    code.set_logger (&logger);
    code.set_error_handler (&errorCollector);

    // Include binary machine code in the log alongside assembly
    logger.add_flags (asmjit::FormatFlags::kMachineCode);

    if (code.init (environment) != asmjit::kErrorOk)
    {
        diagnostics.addError ({}, "Failed to initialize the code holder");
        return makeResultValueFail ("Native kernel emission failed");
    }

    YdspAsm cc (&code);

    // Drop no-op register moves the codegen emitted (see RedundantMoveCleanupPass).
    cc.template add_pass<RedundantMoveCleanupPass<Traits::isX64>>();

    auto* funcNode = cc.add_func (asmjit::FuncSignature::build<void, void*>());
    if (funcNode == nullptr)
    {
        String msg = "Failed to create the kernel function";
        if (! errorCollector.lastMessage.isEmpty())
            msg += ": " + errorCollector.lastMessage;
        diagnostics.addError ({}, msg);
        return makeResultValueFail ("Native kernel emission failed");
    }

    if constexpr (Traits::isX64)
    {
        const bool hasFusedMultiplyAdd = std::any_of (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
                                                      {
            return std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst)
                                {
                return inst.op == YdspIrOp::fmaF || inst.op == YdspIrOp::fmsubF;
            });
        });

        // Marks the frame as AVX, or the register allocator moves, spills and
        // reloads a ymm register with the 128-bit SSE forms and loses its
        // upper lanes.
        if (fn.vectorWidth > YdspVectorizer::vectorWidth || fn.packedFmaAvailable || hasFusedMultiplyAdd)
        {
            funcNode->frame().set_avx_enabled();
            funcNode->frame().set_avx_cleanup();

            if (fn.vectorWidth == 16)
                funcNode->frame().set_avx512_enabled();
        }
    }

    this->cc = &cc;
    this->isEventHandler = isEventHandler;
    activeFn = &fn;
    computeLoopScopes (fn);
    streamBaseRegs.clear();
    loopStreamBases.clear();
    activeVectorWidth = fn.vectorWidth == 8 || fn.vectorWidth == 16 ? fn.vectorWidth : YdspVectorizer::vectorWidth;
    configureTarget (environment, fn);

    YdspGp ctxReg = cc.new_gp64 ("ctx");
    funcNode->set_arg (0, ctxReg);

    inputsReg = cc.new_gp64 ("inputs");
    outputsReg = cc.new_gp64 ("outputs");
    paramsReg = cc.new_gp64 ("params");
    paramOutReg = cc.new_gp64 ("paramOut");
    stateReg = cc.new_gp64 ("state");
    stateArraysReg = cc.new_gp64 ("stateArrays");
    numSamplesReg = cc.new_gp32 ("numSamples");
    sampleRateReg = newFp ("sampleRate");
    outputEventsReg = cc.new_gp64 ("outputEvents");
    traceQueueReg = cc.new_gp64 ("traceQueue");
    for (const auto& block : fn.blocks)
        if (std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst) { return inst.op == YdspIrOp::traceCommit; }))
        {
            loadGpFromMem (traceQueueReg, memPtr (ctxReg, isEventHandler ? 80 : 64));
            break;
        }

    if (isEventHandler)
    {
        // Event-handler ABI: one voice's state slice, the node's params, and
        // the dispatching event's payload. No inputs/outputs/meters or
        // block-size context. The payload is read on demand through
        // loadEventField*, which carries its byte offset, so the context
        // pointer stays live for the whole body.
        eventCtxReg = ctxReg;

        loadGpFromMem (stateReg, memPtr (ctxReg, 0));
        loadGpFromMem (stateArraysReg, memPtr (ctxReg, 8));
        loadGpFromMem (paramsReg, memPtr (ctxReg, 16));
        loadFloatFromMem (sampleRateReg, memPtr (ctxReg, 24));
        loadGpFromMem (outputEventsReg, memPtr (ctxReg, 72));
    }
    else
    {
        bool needsInputs = false;
        bool needsOutputs = false;
        bool needsParams = false;
        bool needsParamOut = false;
        bool needsState = false;
        bool needsStateArrays = false;
        bool needsBlockSize = false;
        bool needsSampleRate = false;
        bool needsOutputEvents = false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                needsParams = needsParams || inst.op == YdspIrOp::loadParam || inst.op == YdspIrOp::storeParam;
                needsParamOut = needsParamOut || inst.op == YdspIrOp::loadParamOut || inst.op == YdspIrOp::storeParamOut;
                needsInputs = needsInputs || inst.op == YdspIrOp::loadInput;
                needsOutputs = needsOutputs || inst.op == YdspIrOp::loadOutput || inst.op == YdspIrOp::storeOutput;
                needsState = needsState || inst.op == YdspIrOp::loadStateF || inst.op == YdspIrOp::storeStateF
                          || inst.op == YdspIrOp::loadStateI || inst.op == YdspIrOp::storeStateI;
                needsStateArrays = needsStateArrays || inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::storeStateArrayF
                                || inst.op == YdspIrOp::loadStateArrayI || inst.op == YdspIrOp::storeStateArrayI;
                needsBlockSize = needsBlockSize || inst.op == YdspIrOp::loadBlockSize;
                needsSampleRate = needsSampleRate || inst.op == YdspIrOp::loadSampleRate;
                needsOutputEvents = needsOutputEvents || inst.op == YdspIrOp::emitEvent
                                 || inst.op == YdspIrOp::storeEventFieldF || inst.op == YdspIrOp::storeEventFieldI;
            }
        }

        if (needsInputs)
            loadGpFromMem (inputsReg, memPtr (ctxReg, 0));
        if (needsOutputs)
            loadGpFromMem (outputsReg, memPtr (ctxReg, 8));
        if (needsParams)
            loadGpFromMem (paramsReg, memPtr (ctxReg, 16));
        if (needsParamOut)
            loadGpFromMem (paramOutReg, memPtr (ctxReg, 24));
        if (needsState)
            loadGpFromMem (stateReg, memPtr (ctxReg, 32));
        if (needsStateArrays)
            loadGpFromMem (stateArraysReg, memPtr (ctxReg, 40));
        if (needsBlockSize)
            loadGpFromMem (numSamplesReg, memPtr (ctxReg, 52));
        if (needsSampleRate)
            loadFloatFromMem (sampleRateReg, memPtr (ctxReg, 48));
        if (needsOutputEvents)
            loadGpFromMem (outputEventsReg, memPtr (ctxReg, 56));

        // Hoist the per-stream channel pointers out of the sample loop: they
        // are fixed for the whole call, so re-deriving them at every access
        // costs one dependent load per stream access per sample.
        //
        // Each outermost loop loads its own in its preheader rather than
        // sharing one set from the prologue: a pointer that is live for the
        // whole kernel sits across every call a block-rate section makes, and
        // is then read back from memory at each access of the sample loop.
        //
        // Only the streams this kernel actually touches may be dereferenced:
        // the init kernel declares the processor's endpoints but is invoked
        // with inputs/outputs set to null (runInitKernels), so hoisting a
        // stream it never reads would fault on the null channel array.
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            for (const auto& inst : fn.blocks[b].insts)
            {
                const bool isInput = inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadInputAddress;
                const bool isOutput = inst.op == YdspIrOp::loadOutput || inst.op == YdspIrOp::storeOutput
                                   || inst.op == YdspIrOp::loadOutputAddress;

                if (! isInput && ! isOutput)
                    continue;

                const auto scope = blockLoopScope[b];
                const StreamBaseKey key { scope, isInput, inst.memIndex };

                if (inst.memIndex < 0 || inst.memIndex >= (isInput ? fn.numInputs : fn.numOutputs) || streamBaseRegs.count (key) != 0)
                    continue;

                const auto base = cc.new_gp64 (isInput ? "inStream" : "outStream");
                streamBaseRegs.emplace (key, base);

                if (scope < 0)
                {
                    loadGpFromMem (base, memPtr (isInput ? inputsReg : outputsReg, inst.memIndex * 8));
                    continue;
                }

                loopStreamBases[fn.loops[static_cast<size_t> (scope)].headerBlock - 1].push_back (key);

                // Live across the whole loop: never worth spilling.
                if (auto* virt = cc.virt_reg_by_reg (base))
                    virt->set_weight (255);
            }
        }
    }

    // Cumulative byte offsets for the (heterogeneous) param/meter blocks. The
    // scalar/array segment layout is derived from YdspIrFunction's shared
    // helpers at every state access (see stateScalarBase/stateArrayBase).
    paramOffsets = ydspEndpointByteOffsets (fn.paramTypes);
    paramOutOffsets = ydspEndpointByteOffsets (fn.paramOutTypes);

    blockLabels.resize (fn.blocks.size());

    for (size_t i = 0; i < fn.blocks.size(); ++i)
        blockLabels[i] = cc.new_label();

    // Allocate a register for every value.
    int maxResult = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result > maxResult)
                maxResult = inst.result;

    regs.resize (static_cast<size_t> (maxResult) + 1);

    // Pre-compute value types to avoid O(N²×M) scanning. The optimiser
    // persists the exact per-value types; fall back to op-based inference
    // defensively.
    valueTypes = resolveValueTypes (fn);
    valueLanes = fn.valueLanes;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0 && ! regs[static_cast<size_t> (inst.result)].is_valid())
                regs[static_cast<size_t> (inst.result)] = newRegFor (valueTypes, inst);

    // Every value id referenced as an operand must have a register allocated
    // above. A miss used to surface much later as an AsmJit "InvalidInstruction"
    // whose operand was a kIdBad (255) register, with no hint which value or
    // instruction caused it - and it only ever fired on the largest kernels.
    // Report it directly instead of letting asmjit fail opaquely.
    for (size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex)
    {
        const auto& insts = fn.blocks[blockIndex].insts;

        for (size_t instIndex = 0; instIndex < insts.size(); ++instIndex)
        {
            const auto& inst = insts[instIndex];

            // Scalar loads carry their state/param *slot* in operand 0, not a
            // value id (the optimizer's isValueIdOperand); a slot index must
            // not be required to name an allocated value register.
            const auto isSlotOperand = [] (YdspIrOp op, int operand)
            {
                switch (op)
                {
                    case YdspIrOp::loadParam:
                    case YdspIrOp::loadParamOut:
                    case YdspIrOp::loadStateF:
                    case YdspIrOp::loadStateI:
                        return operand == 0;

                    default:
                        return false;
                }
            };

            for (int operandIndex = 0; operandIndex < 3; ++operandIndex)
            {
                if (isSlotOperand (inst.op, operandIndex))
                    continue;

                const int operand = operandIndex == 0 ? inst.a : (operandIndex == 1 ? inst.b : inst.c);

                if (operand < 0)
                    continue;

                const auto index = static_cast<size_t> (operand);

                if (index >= regs.size() || ! regs[index].is_valid())
                {
                    String msg = "codegen: operand value ";
                    msg += String (operand);
                    msg += " of instruction ";
                    msg += String (instIndex);
                    msg += " (op ";
                    msg += String (static_cast<int> (inst.op));
                    msg += ", result ";
                    msg += String (inst.result);
                    msg += ", operands ";
                    msg += String (inst.a);
                    msg += "/";
                    msg += String (inst.b);
                    msg += "/";
                    msg += String (inst.c);
                    msg += ") in block ";
                    msg += String (blockIndex);
                    msg += " has no allocated register; this value is never defined as an instruction result in any of the ";
                    msg += String (fn.blocks.size());
                    msg += " blocks (";
                    msg += String (fn.blocks[blockIndex].insts.size());
                    msg += " insts in this block)";

                    // Dump the whole block so the surrounding shape is visible,
                    // and report where every value involved in the instruction
                    // is actually defined (or read again).
                    msg += "\n  block insts (op result a/b/c):";

                    for (size_t i = 0; i < insts.size(); ++i)
                    {
                        const auto& other = insts[i];
                        msg += "\n    [";
                        msg += String (i);
                        msg += "] op ";
                        msg += String (static_cast<int> (other.op));
                        msg += " res ";
                        msg += String (other.result);
                        msg += " (";
                        msg += String (other.a);
                        msg += "/";
                        msg += String (other.b);
                        msg += "/";
                        msg += String (other.c);
                        msg += ")";
                    }

                    const int involved[] = { operand, inst.a, inst.b, inst.c, inst.result };

                    for (const int value : involved)
                    {
                        msg += "\n  value ";
                        msg += String (value);

                        bool defined = false;

                        for (const auto& other : insts)
                        {
                            if (other.result == value)
                            {
                                msg += " defined in this block at op ";
                                msg += String (static_cast<int> (other.op));
                                defined = true;
                                break;
                            }
                        }

                        if (! defined)
                            msg += " not defined in this block";
                    }

                    diagnostics.addError ({}, msg);
                    return makeResultValueFail ("Native kernel emission failed");
                }
            }
        }
    }

    // The allocator ranks a register by how densely it is referenced over its
    // live range, which knows nothing about loops: a value that is live for
    // the whole kernel and read once per sample ranks below one the prologue
    // touches twice. Rank each value by where its references are instead.
    blockLoopDepth.assign (fn.blocks.size(), 0);

    for (const auto& loop : fn.loops)
        if (! loop.unrolled)
            for (int b = std::max (0, loop.headerBlock); b < loop.exitBlock && b < static_cast<int> (fn.blocks.size()); ++b)
                ++blockLoopDepth[static_cast<size_t> (b)];

    {
        std::vector<double> references (regs.size(), 0.0);

        for (size_t b = 0; b < fn.blocks.size(); ++b)
            for (const auto& inst : fn.blocks[b].insts)
                if (inst.result >= 0)
                    references[static_cast<size_t> (inst.result)] += referenceCost (static_cast<int> (b));

        forEachValueUse (fn, [&] (int block, int, int, int value)
        {
            if (static_cast<size_t> (value) < references.size())
                references[static_cast<size_t> (value)] += referenceCost (block);
        });

        const auto setWeight = [&cc] (const asmjit::Reg& reg, uint32_t weight)
        {
            if (reg.is_valid())
                if (auto* virt = cc.virt_reg_by_reg (reg))
                    virt->set_weight (weight);
        };

        // Loop induction variables (live across the back edge) and every
        // widened value (a spilled 4/8-lane vector costs several loads/stores
        // per sample) are kept in registers outright.
        std::vector<bool> pinned (regs.size(), false);

        for (const auto& loop : fn.loops)
            if (loop.induction >= 0 && static_cast<size_t> (loop.induction) < regs.size())
                pinned[static_cast<size_t> (loop.induction)] = true;

        for (size_t i = 0; i < valueLanes.size() && i < regs.size(); ++i)
            pinned[i] = pinned[i] || valueLanes[i] > 1;

        for (size_t i = 0; i < regs.size(); ++i)
            setWeight (regs[i], pinned[i] ? 255u : weightFor (references[i]));

    }

    // Last chance to set up addressing that would otherwise be re-derived at
    // every access. Must come after valueTypes and the segment offsets, and
    // before any block is emitted, so the addresses dominate every use.
    planCompareFusion (fn);
    planOperandLowering (fn);
    planFlagReuse (fn);
    planEndRelativeLoops (fn);
    planLoopPointers (fn);
    prepareStateAddressing (fn);
    prepareWrapBounds (fn);
    prepareRampConsts (fn);
    materializeMathCallTargets (fn);

    // Emit the blocks.
    for (size_t i = 0; i < fn.blocks.size(); ++i)
    {
        cc.bind (blockLabels[i]);
        currentBlock = static_cast<int> (i);
        beginBlock (static_cast<int> (i));

        for (size_t j = 0; j < fn.blocks[i].insts.size(); ++j)
        {
            const auto& inst = fn.blocks[i].insts[j];

            emitInstruction (fn, inst, static_cast<int> (i), static_cast<int> (j));

            // The IR is not SSA, so a value id can be written more than once
            // inside a single block - a fully unrolled loop rewrites its
            // induction variable once per copy. Anything a backend derived from
            // that id and memoized for the block is stale from here on.
            if (inst.result >= 0)
                onValueRedefined (inst.result);
        }

        if (const auto bases = loopStreamBases.find (static_cast<int> (i)); bases != loopStreamBases.end())
            for (const auto& key : bases->second)
                loadGpFromMem (streamBaseRegs.at (key), memPtr (std::get<1> (key) ? inputsReg : outputsReg, std::get<2> (key) * 8));

        if (const auto ramps = loopRampConsts.find (static_cast<int> (i)); ramps != loopRampConsts.end())
        {
            const auto mem = rampConstMem();

            for (const auto& ramp : ramps->second)
                loadVectorFromMem (ramp, mem);
        }

        endBlock (static_cast<int> (i));
        emitEndRelativeLoopSetup (static_cast<int> (i));
        emitLoopPointerSetup (static_cast<int> (i));
        emitLoopPointerAdvance (static_cast<int> (i));
        emitTerminator (fn.blocks[i], static_cast<int> (i));
    }

    cc.end_func();

    for (const auto& [name, label] : externalSymbols)
    {
        cc.align (asmjit::AlignMode::kData, 8);
        cc.bind (label);
        cc.embed_uint64 (0);
    }

    const auto asmError = cc.finalize();
    if (asmError != asmjit::kErrorOk)
    {
        String msg = "AsmJit failed to assemble the kernel: ";
        msg += asmjit::stringify_error (asmError);

        if (! errorCollector.lastMessage.isEmpty())
        {
            msg += " (";
            msg += errorCollector.lastMessage;
            msg += ")";
        }

        // Append the asmjit assembly log for offline inspection
        if (logger.data_size() > 0)
        {
            msg += "\n\n--- AsmJit assembly log ---\n";
            msg += String (logger.data(), logger.data_size());
        }

        diagnostics.addError ({}, msg);
        return makeResultValueFail ("Native kernel emission failed");
    }

    if (code.flatten() != asmjit::kErrorOk || code.resolve_cross_section_fixups() != asmjit::kErrorOk
        || code.has_unresolved_fixups() || code.has_reloc_entries())
    {
        diagnostics.addError ({}, "Native kernel contains unresolved or absolute relocations");
        return makeResultValueFail ("Native kernel is not position-independent");
    }

    YdspNativeArtifact artifact;
    artifact.code.resize (code.code_size());
    if (code.copy_flattened_data (artifact.code.data(), artifact.code.size()) != asmjit::kErrorOk)
        return makeResultValueFail ("Failed to copy native kernel");

    for (const auto& [name, label] : externalSymbols)
        artifact.symbols.push_back ({ name, code.label_offset_from_base (label) });

    // Always emit the generated assembly as an info diagnostic
    if (logger.data_size() > 0)
    {
        const String info (logger.data(), logger.data_size());
        diagnostics.addInfo ({}, info);
    }

    return makeResultValueOk (std::move (artifact));
}

//==============================================================================

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::planOperandLowering (const YdspIrFunction& fn)
{
    integerImmediates.clear();
    arrayDisplacements.clear();
    integerCompareImmediates.clear();
    elidedIntegerConstants.clear();
    elidedFloatConstants.clear();
    zeroVectorSplats.clear();
    fusedProductSubtracts.clear();
    elidedNegations.clear();

    const YdspValueUseCounts counts (fn);
    auto remainingUses = counts.uses;
    std::unordered_map<int, int64_t> literals;
    std::unordered_set<int> floatZeroLiterals;
    std::vector<YdspInstPosition> definitions (fn.valueTypes.size(), { -1, -1 });
    for (size_t b = 0; b < fn.blocks.size(); ++b)
        for (size_t i = 0; i < fn.blocks[b].insts.size(); ++i)
        {
            const auto& inst = fn.blocks[b].insts[i];
            if (! counts.definedOnce (inst.result))
                continue;
            definitions[static_cast<size_t> (inst.result)] = { static_cast<int> (b), static_cast<int> (i) };
            if (inst.op == YdspIrOp::constI)
                literals.emplace (inst.result, inst.ivalue);
            else if (inst.op == YdspIrOp::constF
                     && static_cast<size_t> (inst.result) < valueTypes.size()
                     && valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type
                     && floatBits (inst.fvalue) == 0)
                floatZeroLiterals.insert (inst.result);
        }

    std::vector<bool> local (fn.valueTypes.size(), true);
    forEachValueUse (fn, [&] (int block, int instruction, int, int value)
    {
        const auto definition = definitions[static_cast<size_t> (value)];
        if (block != definition.first || instruction <= definition.second)
            local[static_cast<size_t> (value)] = false;
    });

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        const auto& insts = fn.blocks[b].insts;
        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& inst = insts[i];
            const YdspInstPosition position { static_cast<int> (b), static_cast<int> (i) };

            const bool arrayLoad = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI;
            const bool arrayStore = inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeStateArrayI;
            if (arrayLoad || arrayStore)
            {
                const int valueId = arrayLoad ? inst.result : inst.b;
                const auto type = valueTypes[static_cast<size_t> (valueId)];
                const int bytes = is64BitValueType (type) ? 8 : 4;
                const int accessBytes = bytes * fn.laneCountOf (valueId);
                if (const auto index = literals.find (inst.a); index != literals.end()
                    && index->second >= 0 && index->second <= std::numeric_limits<int32_t>::max() / bytes)
                {
                    const int64_t offset = static_cast<int64_t> (stateArrayBase (type, inst.memIndex)) + index->second * bytes;
                    const bool encodable = Traits::isX64 ? offset >= 0 && offset <= std::numeric_limits<int32_t>::max()
                                                        : offset >= 0 && offset <= 4095 * accessBytes && offset % accessBytes == 0;
                    if (encodable)
                    {
                        arrayDisplacements.emplace (position, static_cast<int32_t> (offset));
                        --remainingUses[static_cast<size_t> (inst.a)];
                    }
                }
            }
            if (const auto literal = literals.find (inst.b); literal != literals.end())
            {
                const auto value = literal->second;
                const bool fusedCompare = isIntComparison (inst.op)
                    && counts.definedOnce (inst.result) && elidedCompares.count (position) != 0;
                bool encodable = false;
                if (! isVectorValue (inst.result)
                    && (fusedCompare || (! inst.saturatingIntegerArithmetic && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI)) || inst.op == YdspIrOp::andI))
                {
                    const bool wide = valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::int64Type;
                    if constexpr (Traits::isX64)
                    {
                        // A 64-bit operation sign-extends its 32-bit immediate.
                        encodable = ! wide || (value >= std::numeric_limits<int32_t>::min()
                                               && value <= std::numeric_limits<int32_t>::max());
                    }
                    else
                    {
                        const auto bits = static_cast<uint64_t> (value);
                        encodable = inst.op == YdspIrOp::andI
                                      ? value > 0 && (bits & (bits + 1)) == 0 && (wide || bits < 0xffffffffu)
                                      : value >= 0 && value <= 4095;
                    }
                }
                if (encodable)
                {
                    if (fusedCompare)
                        integerCompareImmediates.emplace (inst.result, value);
                    else
                        integerImmediates.emplace (position, value);
                    --remainingUses[static_cast<size_t> (inst.b)];
                }
            }

            bool isFusedProductSubtract = false;
            if (inst.op == YdspIrOp::fmaF && ! isVectorValue (inst.result)
                && counts.definedOnce (inst.c) && counts.useCount (inst.c) == 1)
            {
                for (size_t j = i; j-- > 0;)
                {
                    const auto& negation = insts[j];
                    if (negation.result != inst.c)
                        continue;
                    if (negation.op != YdspIrOp::negF)
                        break;

                    bool stable = true;
                    for (size_t k = j + 1; k < i; ++k)
                        if (insts[k].result == negation.a)
                            stable = false;

                    if (stable)
                    {
                        fusedProductSubtracts.emplace (position, negation.a);
                        elidedNegations.emplace (static_cast<int> (b), static_cast<int> (j));
                        isFusedProductSubtract = true;
                    }
                    break;
                }
            }

            if (supportsZeroVector()
                && inst.op == YdspIrOp::vsplat
                && inst.result >= 0
                && inst.a >= 0
                && static_cast<size_t> (inst.a) < remainingUses.size())
            {
                const bool isFloat = isFloatValueType (valueTypes[static_cast<size_t> (inst.result)]);
                const bool isZero = isFloat
                                  ? floatZeroLiterals.count (inst.a) != 0
                                  : (literals.find (inst.a) != literals.end() && literals.at (inst.a) == 0);
                if (isZero)
                {
                    zeroVectorSplats.insert (position);
                    --remainingUses[static_cast<size_t> (inst.a)];
                }
            }

            const auto hasEnclosingBackEdge = [&] (int defBlock, int useBlock) -> bool
            {
                for (size_t k = static_cast<size_t> (defBlock); k < fn.blocks.size(); ++k)
                {
                    const auto& blk = fn.blocks[k];
                    const int targets[] { blk.termTarget, blk.termTarget2 };

                    for (const int target : targets)
                        if (target > defBlock && target <= useBlock && target <= static_cast<int> (k))
                            return true;
                }

                return false;
            };

            const auto canCoalesce = [&] (int operand) -> bool
            {
                if (operand < 0
                    || ! counts.definedOnce (inst.result)
                    || ! counts.definedOnce (operand)
                    || counts.useCount (operand) != 1
                    || ! local[static_cast<size_t> (inst.result)]
                    || fn.laneCountOf (inst.result) != fn.laneCountOf (operand)
                    || valueTypes[static_cast<size_t> (inst.result)] != valueTypes[static_cast<size_t> (operand)])
                    return false;

                const auto def = definitions[static_cast<size_t> (operand)];

                if (def.first == static_cast<int> (b))
                    return def.second < static_cast<int> (i);

                return def.first >= 0
                    && def.first < static_cast<int> (b)
                    && blockLoopScope[static_cast<size_t> (def.first)] == blockLoopScope[b]
                    && blockLoopDepth[static_cast<size_t> (def.first)] == blockLoopDepth[b]
                    && ! hasEnclosingBackEdge (def.first, static_cast<int> (b));
            };

            const auto canCoalesceCarriedAddend = [&] (int initialResult, int addend) -> bool
            {
                if (initialResult < 0 || addend < 0 || initialResult == addend
                    || ! counts.definedOnce (initialResult)
                    || ! local[static_cast<size_t> (initialResult)]
                    || fn.laneCountOf (initialResult) != fn.laneCountOf (addend)
                    || valueTypes[static_cast<size_t> (initialResult)] != valueTypes[static_cast<size_t> (addend)])
                    return false;

                const auto usesOperand = [] (const YdspIrInst& candidate, int value) noexcept
                {
                    for (int opIndex = 0; opIndex < 3; ++opIndex)
                        if (isValueIdOperand (candidate.op, opIndex))
                        {
                            const int opVal = opIndex == 0 ? candidate.a : (opIndex == 1 ? candidate.b : candidate.c);
                            if (opVal == value)
                                return true;
                        }

                    return false;
                };

                int current = initialResult;
                size_t moveIndex = insts.size();

                for (size_t k = i + 1; k < insts.size(); ++k)
                {
                    const auto& next = insts[k];

                    if (next.op == YdspIrOp::movF && next.result == addend && next.a == current)
                    {
                        moveIndex = k;
                        break;
                    }

                    if (next.result == addend || next.result == current || usesOperand (next, addend))
                        return false;

                    if (usesOperand (next, current))
                    {
                        const bool isNextFma = (next.op == YdspIrOp::fmaF || next.op == YdspIrOp::fmsubF);

                        if (isNextFma
                            && next.c == current
                            && next.a != current
                            && next.b != current
                            && counts.definedOnce (current)
                            && local[static_cast<size_t> (current)]
                            && counts.useCount (current) == 1
                            && next.result >= 0
                            && counts.definedOnce (next.result)
                            && local[static_cast<size_t> (next.result)]
                            && fn.laneCountOf (next.result) == fn.laneCountOf (addend)
                            && valueTypes[static_cast<size_t> (next.result)] == valueTypes[static_cast<size_t> (addend)])
                        {
                            current = next.result;
                            continue;
                        }

                        if (current == initialResult)
                            continue;

                        return false;
                    }
                }

                if (moveIndex >= insts.size())
                    return false;

                for (size_t k = moveIndex + 1; k < insts.size(); ++k)
                    if (usesOperand (insts[k], current))
                        return false;

                return true;
            };

            const auto coalesceWith = [&] (int operand)
            {
                if (auto* resVirt = cc->virt_reg_by_reg (regs[static_cast<size_t> (inst.result)]))
                    if (auto* opVirt = cc->virt_reg_by_reg (regs[static_cast<size_t> (operand)]))
                        opVirt->set_weight (std::max (opVirt->weight(), resVirt->weight()));

                regs[static_cast<size_t> (inst.result)] = regs[static_cast<size_t> (operand)];
            };

            if (inst.op == YdspIrOp::vinsertF)
            {
                if (inst.a >= 0 && canCoalesce (inst.a))
                    coalesceWith (inst.a);
            }
            else if ((inst.op == YdspIrOp::fmaF || inst.op == YdspIrOp::fmsubF) && ! isFusedProductSubtract)
            {
                if (canCoalesce (inst.c) || canCoalesceCarriedAddend (inst.result, inst.c))
                {
                    coalesceWith (inst.c);
                }
                else if constexpr (Traits::isX64)
                {
                    if (inst.op == YdspIrOp::fmaF)
                    {
                        if (canCoalesce (inst.b))
                            coalesceWith (inst.b);
                    }
                    else
                    {
                        if (canCoalesce (inst.a))
                            coalesceWith (inst.a);
                        else if (canCoalesce (inst.b))
                            coalesceWith (inst.b);
                    }
                }
            }
            else if constexpr (Traits::isX64)
            {
                const bool isVector = isVectorValue (inst.result);
                const bool isFloatBin = inst.op == YdspIrOp::addF || inst.op == YdspIrOp::subF
                                     || inst.op == YdspIrOp::mulF || inst.op == YdspIrOp::divF
                                     || inst.op == YdspIrOp::minF || inst.op == YdspIrOp::maxF;
                const bool isScalarIntBin = ! isVector && ! inst.saturatingIntegerArithmetic
                                         && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI
                                             || inst.op == YdspIrOp::mulI || inst.op == YdspIrOp::andI
                                             || inst.op == YdspIrOp::orI || inst.op == YdspIrOp::xorI
                                             || inst.op == YdspIrOp::shlI || inst.op == YdspIrOp::shrI
                                             || inst.op == YdspIrOp::andB || inst.op == YdspIrOp::orB);
                const bool isNonVexVectorIntBin = isVector && ! usesVexVectors() && ! inst.saturatingIntegerArithmetic
                                               && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI
                                                   || inst.op == YdspIrOp::andI || inst.op == YdspIrOp::orI
                                                   || inst.op == YdspIrOp::xorI || inst.op == YdspIrOp::andB
                                                   || inst.op == YdspIrOp::orB);
                const bool isNonVexVectorFloatBin = isVector && ! usesVexVectors() && isFloatBin;

                if (((! isVector && isFloatBin) || isScalarIntBin || isNonVexVectorFloatBin || isNonVexVectorIntBin)
                    && canCoalesce (inst.a))
                {
                    coalesceWith (inst.a);
                }
            }
        }
    }

    for (const auto& [value, literal] : literals)
        if (remainingUses[static_cast<size_t> (value)] == 0)
            elidedIntegerConstants.insert (value);

    for (const int value : floatZeroLiterals)
        if (static_cast<size_t> (value) < remainingUses.size() && remainingUses[static_cast<size_t> (value)] == 0)
            elidedFloatConstants.insert (value);
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitPlannedIntCompareToFlags (const YdspIrInst& compare)
{
    if (const auto immediate = integerCompareImmediates.find (compare.result); immediate != integerCompareImmediates.end())
    {
        if constexpr (Traits::isX64)
            cc->cmp (gp (compare.a), asmjit::Imm (static_cast<int32_t> (immediate->second)));
        else
            cc->cmp (gp (compare.a), asmjit::Imm (immediate->second));
    }
    else
    {
        emitIntCompareToFlags (gp (compare.a), gp (compare.b));
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::planCompareFusion (const YdspIrFunction& fn)
{
    fusedSelectCompares.clear();
    fusedBranchCompares.clear();
    elidedCompares.clear();

    // Definition and use counts over the whole function. Operand fields that
    // actually hold a slot index (loadParam's `a`, say) are counted as uses
    // too: that only ever over-counts, which costs a fusion rather than
    // permitting an unsound one.
    std::unordered_map<int, int> defCount;
    std::unordered_map<int, int> useCount;
    std::unordered_map<int, int> fusedSelectUseCount;
    std::map<int, YdspInstPosition> selectComparePositions;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result >= 0)
                ++defCount[inst.result];

            if (inst.a >= 0)
                ++useCount[inst.a];
            if (inst.b >= 0)
                ++useCount[inst.b];
            if (inst.c >= 0)
                ++useCount[inst.c];
        }

        if (block.termCond >= 0)
            ++useCount[block.termCond];
    }

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        const auto& block = fn.blocks[b];

        for (size_t j = 0; j < block.insts.size(); ++j)
        {
            const auto& select = block.insts[j];

            if (select.op != YdspIrOp::selectB || select.result < 0 || select.a < 0 || isVectorValue (select.result))
                continue;

            const auto condition = select.a;

            // Each consumer must independently prove the comparison operands
            // stable. Elide the boolean only when all uses can be fused.
            if (defCount[condition] != 1)
                continue;

            size_t producer = 0;
            bool foundProducer = false;

            for (size_t k = j; k-- > 0;)
            {
                if (block.insts[k].result == condition)
                {
                    producer = k;
                    foundProducer = true;
                    break;
                }
            }

            if (! foundProducer)
                continue;

            const auto& compare = block.insts[producer];

            if ((! isFloatComparison (compare.op) && ! isIntComparison (compare.op)) || isVectorValue (compare.result))
                continue;

            // Re-emitting the comparison next to the select is only equivalent
            // while its operands still hold the same values.
            bool operandsStable = true;

            for (size_t k = producer + 1; k < j && operandsStable; ++k)
            {
                const auto written = block.insts[k].result;
                operandsStable = written < 0 || (written != compare.a && written != compare.b);
            }

            if (! operandsStable)
                continue;

            // Only elide the comparison once the select is committed to
            // re-emitting it: dropping it otherwise would leave the select
            // reading a register nothing ever writes.
            const YdspInstPosition selectAt { static_cast<int> (b), static_cast<int> (j) };
            const YdspInstPosition compareAt { static_cast<int> (b), static_cast<int> (producer) };

            if (fusedSelectCompares.emplace (selectAt, compare).second)
            {
                ++fusedSelectUseCount[condition];
                selectComparePositions.emplace (condition, compareAt);
            }
        }
    }

    for (const auto& [condition, position] : selectComparePositions)
        if (fusedSelectUseCount[condition] == useCount[condition])
            elidedCompares.insert (position);

    std::erase_if (fusedSelectCompares, [&] (const auto& entry)
    {
        const auto condition = entry.second.result;
        return fusedSelectUseCount[condition] != useCount[condition];
    });

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        const auto& block = fn.blocks[b];

        if (block.term != YdspIrTerm::branchIf || block.termCond < 0)
            continue;

        const auto condition = block.termCond;

        if (defCount[condition] != 1 || useCount[condition] != 1)
            continue;

        int producer = -1;

        for (size_t k = block.insts.size(); k-- > 0;)
        {
            if (block.insts[k].result == condition)
            {
                producer = static_cast<int> (k);
                break;
            }
        }

        if (producer < 0)
            continue;

        const auto& compare = block.insts[static_cast<size_t> (producer)];

        if ((! isFloatComparison (compare.op) && ! isIntComparison (compare.op)) || isVectorValue (compare.result))
            continue;

        bool operandsStable = true;

        for (size_t k = static_cast<size_t> (producer) + 1; k < block.insts.size() && operandsStable; ++k)
        {
            const auto written = block.insts[k].result;
            operandsStable = written < 0 || (written != compare.a && written != compare.b);
        }

        if (! operandsStable)
            continue;

        if (fusedBranchCompares.emplace (static_cast<int> (b), compare).second)
            elidedCompares.insert ({ static_cast<int> (b), producer });
    }
}

//==============================================================================

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::computeLoopScopes (const YdspIrFunction& fn)
{
    const auto numBlocks = static_cast<int> (fn.blocks.size());
    blockLoopScope.assign (fn.blocks.size(), -1);

    for (size_t l = 0; l < fn.loops.size(); ++l)
    {
        const auto& loop = fn.loops[l];
        const int header = loop.headerBlock;
        const int exit = std::min (loop.exitBlock, numBlocks);

        if (loop.unrolled || header < 1 || exit <= header)
            continue;

        const auto& preheader = fn.blocks[static_cast<size_t> (header) - 1];
        bool entersThroughPreheader = preheader.term == YdspIrTerm::fallthrough
                                   || (preheader.term == YdspIrTerm::branch && preheader.termTarget == header);

        for (int b = 0; b < numBlocks && entersThroughPreheader; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];
            const bool inside = b >= header && b < exit;

            for (const int target : { block.termTarget, block.termTarget2, block.term == YdspIrTerm::fallthrough ? b + 1 : -1 })
                if (! inside && b != header - 1 && target >= header && target < exit)
                    entersThroughPreheader = false;
        }

        if (! entersThroughPreheader)
            continue;

        // The outermost loop wins: an inner loop's blocks are already claimed.
        for (int b = header; b < exit; ++b)
        {
            auto& scope = blockLoopScope[static_cast<size_t> (b)];

            if (scope < 0 || fn.loops[static_cast<size_t> (scope)].headerBlock > header)
                scope = static_cast<int> (l);
        }
    }
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspGp YdspAsmJitCodegenImpl<Traits>::streamBase (int blockIndex, bool isInput, int slot) const
{
    const auto base = streamBaseRegs.find ({ blockLoopScope[static_cast<size_t> (blockIndex)], isInput, slot });

    jassert (base != streamBaseRegs.end());
    return base != streamBaseRegs.end() ? base->second : YdspGp {};
}

template <typename Traits>
double YdspAsmJitCodegenImpl<Traits>::referenceCost (int blockIndex) const noexcept
{
    const auto depth = static_cast<size_t> (blockIndex) < blockLoopDepth.size() ? blockLoopDepth[static_cast<size_t> (blockIndex)] : 0;
    return std::pow (10.0, std::min (depth, 4));
}

template <typename Traits>
uint32_t YdspAsmJitCodegenImpl<Traits>::weightFor (double references) noexcept
{
    // One reference in a loop nested four deep is 10^4, about 13 doublings.
    return static_cast<uint32_t> (jlimit (1.0, 240.0, 16.0 * std::log2 (1.0 + references)));
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::planFlagReuse (const YdspIrFunction& fn)
{
    flagSettingArithmetic.clear();
    flagPreservingArithmetic.clear();
    flagReusingSelects.clear();

    const YdspValueUseCounts counts (fn);
    std::unordered_set<int> zeros;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::constI && inst.ivalue == 0 && counts.definedOnce (inst.result))
                zeros.insert (inst.result);

    const auto isFlagPreservingPureOp = [this] (const YdspIrInst& between, YdspInstPosition at) -> bool
    {
        if (elidedCompares.count (at) != 0
            || elidedNegations.count (at) != 0
            || flagReusingSelects.count (at) != 0)
            return true;

        if ((between.result >= 0 && isVectorValue (between.result))
            || (between.b >= 0 && isVectorValue (between.b)))
            return false;

        switch (between.op)
        {
            case YdspIrOp::constI:
            case YdspIrOp::constB:
            case YdspIrOp::constF:
            case YdspIrOp::movI:
            case YdspIrOp::movF:
            case YdspIrOp::movB:
            case YdspIrOp::loadBlockSize:
            case YdspIrOp::loadSampleRate:
            case YdspIrOp::loadParam:
            case YdspIrOp::storeParam:
            case YdspIrOp::loadParamOut:
            case YdspIrOp::storeParamOut:
            case YdspIrOp::loadStateF:
            case YdspIrOp::storeStateF:
            case YdspIrOp::loadStateI:
            case YdspIrOp::storeStateI:
            case YdspIrOp::loadStateArrayF:
            case YdspIrOp::storeStateArrayF:
            case YdspIrOp::loadStateArrayI:
            case YdspIrOp::storeStateArrayI:
            case YdspIrOp::loadInput:
            case YdspIrOp::loadOutput:
            case YdspIrOp::storeOutput:
            case YdspIrOp::loadEventFieldF:
            case YdspIrOp::loadEventFieldI:
            case YdspIrOp::storeEventFieldF:
            case YdspIrOp::storeEventFieldI:
            case YdspIrOp::addF:
            case YdspIrOp::subF:
            case YdspIrOp::mulF:
            case YdspIrOp::divF:
            case YdspIrOp::minF:
            case YdspIrOp::maxF:
            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::floorF:
            case YdspIrOp::ceilF:
            case YdspIrOp::rintF:
            case YdspIrOp::clampF:
            case YdspIrOp::lerpF:
            case YdspIrOp::fmaF:
            case YdspIrOp::fmsubF:
            case YdspIrOp::itof:
            case YdspIrOp::extI:
            case YdspIrOp::truncI:
            case YdspIrOp::extF:
            case YdspIrOp::truncF:
            case YdspIrOp::bitcastFtoI:
            case YdspIrOp::bitcastItoF:
                return true;

            default:
                return false;
        }
    };

    // In position order, so a select that sits between a later select and its
    // subtraction is already known when that later one is checked.
    for (const auto& [selectAt, compare] : fusedSelectCompares)
    {
        int testedValue = -1;
        bool isLtZero = false;

        if ((compare.op == YdspIrOp::ltI || compare.op == YdspIrOp::geI) && zeros.count (compare.b) != 0)
        {
            testedValue = compare.a;
            isLtZero = (compare.op == YdspIrOp::ltI);
        }
        else if ((compare.op == YdspIrOp::gtI || compare.op == YdspIrOp::leI) && zeros.count (compare.a) != 0)
        {
            testedValue = compare.b;
            isLtZero = (compare.op == YdspIrOp::gtI);
        }
        else
        {
            continue;
        }

        const auto& insts = fn.blocks[static_cast<size_t> (selectAt.first)].insts;
        int producer = -1;

        for (int k = selectAt.second; k-- > 0 && producer < 0;)
            if (insts[static_cast<size_t> (k)].result == testedValue)
                producer = k;

        if (producer < 0)
            continue;

        const YdspInstPosition producerPos { selectAt.first, producer };
        if (flagPreservingArithmetic.count (producerPos) != 0)
            continue;

        const auto& arithmetic = insts[static_cast<size_t> (producer)];

        if ((arithmetic.op != YdspIrOp::addI && arithmetic.op != YdspIrOp::subI) || arithmetic.saturatingIntegerArithmetic)
            continue;

        // Everything emitted between the two must leave the flags alone.
        // Note: on x86-64, add/sub clobber flags so intervening addI/subI
        // (with an encodable immediate for subI) are emitted via lea instead,
        // and constI/constB/movI stay on mov (never xor).
        bool flagsSurvive = true;
        std::vector<YdspInstPosition> preserved;

        for (int k = producer + 1; k < selectAt.second && flagsSurvive; ++k)
        {
            const YdspInstPosition at { selectAt.first, k };
            const auto& between = insts[static_cast<size_t> (k)];

            bool plainArithmetic = false;
            if (! between.saturatingIntegerArithmetic && flagSettingArithmetic.count (at) == 0)
            {
                if constexpr (Traits::isX64)
                {
                    if (between.op == YdspIrOp::addI)
                    {
                        const auto imm = integerImmediates.find (at);
                        plainArithmetic = imm == integerImmediates.end()
                                       || (imm->second >= std::numeric_limits<int32_t>::min()
                                           && imm->second <= std::numeric_limits<int32_t>::max());
                    }
                    else if (between.op == YdspIrOp::subI)
                    {
                        const auto imm = integerImmediates.find (at);
                        plainArithmetic = imm != integerImmediates.end()
                                       && imm->second > std::numeric_limits<int32_t>::min()
                                       && imm->second <= std::numeric_limits<int32_t>::max();
                    }

                    if (plainArithmetic)
                        preserved.push_back (at);
                }
                else
                {
                    plainArithmetic = between.op == YdspIrOp::addI || between.op == YdspIrOp::subI;
                }
            }

            flagsSurvive = plainArithmetic || isFlagPreservingPureOp (between, at);
        }

        if (! flagsSurvive)
            continue;

        for (const auto& at : preserved)
            flagPreservingArithmetic.insert (at);

        flagSettingArithmetic.insert (producerPos);
        flagReusingSelects.emplace (selectAt, isLtZero);
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitFlagSettingArithmetic (const YdspIrInst& inst, YdspInstPosition position)
{
    const auto dst = gp (inst.result);
    const auto source = gp (inst.a);
    const auto immediate = integerImmediates.find (position);

    if constexpr (Traits::isX64)
    {
        if (immediate != integerImmediates.end())
        {
            if (dst != source)
                moveGp (dst, source);

            const auto value = asmjit::Imm (static_cast<int32_t> (immediate->second));
            if (inst.op == YdspIrOp::addI)
                cc->add (dst, value);
            else
                cc->sub (dst, value);
        }
        else
        {
            const auto rhs = gp (inst.b);
            if (dst == rhs && dst != source)
            {
                const auto tmp = dst.is_gp64() ? cc->new_gp64 ("flagArith") : cc->new_gp32 ("flagArith");
                moveGp (tmp, source);
                if (inst.op == YdspIrOp::addI)
                    cc->add (tmp, rhs);
                else
                    cc->sub (tmp, rhs);
                moveGp (dst, tmp);
            }
            else
            {
                if (dst != source)
                    moveGp (dst, source);
                if (inst.op == YdspIrOp::addI)
                    cc->add (dst, rhs);
                else
                    cc->sub (dst, rhs);
            }
        }
    }
    else
    {
        if (immediate != integerImmediates.end() && inst.op == YdspIrOp::addI)
            cc->adds (dst, source, asmjit::Imm (immediate->second));
        else if (immediate != integerImmediates.end())
            cc->subs (dst, source, asmjit::Imm (immediate->second));
        else if (inst.op == YdspIrOp::addI)
            cc->adds (dst, source, gp (inst.b));
        else
            cc->subs (dst, source, gp (inst.b));
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::planEndRelativeLoops (const YdspIrFunction& fn)
{
    endRelativeLoops.clear();

    const YdspValueUseCounts counts (fn);
    const auto numBlocks = static_cast<int> (fn.blocks.size());

    const auto isZero = [&] (int value)
    {
        if (! counts.definedOnce (value))
            return false;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.result == value)
                    return inst.op == YdspIrOp::constI && inst.ivalue == 0;

        return false;
    };

    for (const auto& loop : fn.loops)
    {
        EndRelativeLoop plan;
        plan.headerBlock = loop.headerBlock;
        plan.latchBlock = loop.exitBlock - 1;
        plan.induction = loop.induction;

        if (loop.unrolled || plan.headerBlock < 1 || plan.latchBlock <= plan.headerBlock || plan.latchBlock >= numBlocks
            || plan.induction < 0 || valueTypes[static_cast<size_t> (plan.induction)] != YdspValueType::int32Type)
            continue;

        // The shape emitTerminator() rotates: a compare-only header, tested
        // again at the latch.
        const auto& header = fn.blocks[static_cast<size_t> (plan.headerBlock)];
        const auto& latch = fn.blocks[static_cast<size_t> (plan.latchBlock)];
        const auto& preheader = fn.blocks[static_cast<size_t> (plan.headerBlock) - 1];
        const auto test = fusedBranchCompares.find (plan.headerBlock);

        if (test == fusedBranchCompares.end() || header.insts.size() != 1 || header.term != YdspIrTerm::branchIf
            || header.termTarget != plan.headerBlock + 1 || header.termTarget2 != plan.latchBlock + 1
            || latch.term != YdspIrTerm::branch || latch.termTarget != plan.headerBlock || latch.insts.empty()
            || ! (preheader.term == YdspIrTerm::fallthrough
                  || (preheader.term == YdspIrTerm::branch && preheader.termTarget == plan.headerBlock)))
            continue;

        plan.bound = test->second.b;

        if (test->second.op != YdspIrOp::ltI || test->second.a != plan.induction
            || integerCompareImmediates.count (test->second.result) != 0
            || ! counts.definedOnce (plan.bound)
            || valueTypes[static_cast<size_t> (plan.bound)] != YdspValueType::int32Type)
            continue;

        const YdspInstPosition step { plan.latchBlock, static_cast<int> (latch.insts.size()) - 1 };
        const auto& increment = latch.insts.back();
        const auto one = integerImmediates.find (step);

        if (increment.op != YdspIrOp::addI || increment.result != plan.induction || increment.a != plan.induction
            || increment.saturatingIntegerArithmetic || one == integerImmediates.end() || one->second != 1)
            continue;

        // Entered from the preheader and the latch only, with the offset
        // seeded to zero in the preheader and the bound known by then.
        bool eligible = true;
        int definitions = 0;

        for (int b = 0; b < numBlocks && eligible; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];

            if (b != plan.latchBlock && b != plan.headerBlock - 1
                && (block.termTarget == plan.headerBlock || block.termTarget2 == plan.headerBlock))
                eligible = false;

            for (int i = 0; i < static_cast<int> (block.insts.size()); ++i)
            {
                const auto& inst = block.insts[static_cast<size_t> (i)];

                if (inst.result == plan.bound && b >= plan.headerBlock)
                    eligible = false;

                if (inst.result != plan.induction)
                    continue;

                ++definitions;

                if (YdspInstPosition { b, i } != step)
                    plan.seed = { b, i };
            }
        }

        if (! eligible || definitions != 2 || plan.seed.first != plan.headerBlock - 1)
            continue;

        const auto& seed = preheader.insts[static_cast<size_t> (plan.seed.second)];

        if (! ((seed.op == YdspIrOp::constI && seed.ivalue == 0) || (seed.op == YdspIrOp::movI && isZero (seed.a))))
            continue;

        // Besides the loop test and the increment, the induction may only
        // index a scalar stream access inside the loop.
        forEachValueUse (fn, [&] (int block, int instruction, int operand, int value)
        {
            if (value != plan.induction || ! eligible)
                return;

            const YdspInstPosition at { block, instruction };

            if (at == step || at == YdspInstPosition { plan.headerBlock, 0 })
                return;

            if (instruction < 0 || operand != 0 || block <= plan.headerBlock || block > plan.latchBlock)
            {
                eligible = false;
                return;
            }

            const auto& inst = fn.blocks[static_cast<size_t> (block)].insts[static_cast<size_t> (instruction)];
            const bool isLoad = inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadOutput;
            const int accessed = isLoad ? inst.result : inst.b;

            if ((! isLoad && inst.op != YdspIrOp::storeOutput) || isVectorValue (accessed))
            {
                eligible = false;
                return;
            }

            const uint32_t scale = is64BitValueType (valueTypes[static_cast<size_t> (accessed)]) ? 3u : 2u;
            const int slot = inst.op == YdspIrOp::loadInput ? inst.memIndex : fn.numInputs + inst.memIndex;

            if (const auto [known, inserted] = plan.streamScales.emplace (slot, scale); ! inserted && known->second != scale)
                eligible = false;
        });

        if (! eligible)
            continue;

        const auto pinned = [this] (const char* name)
        {
            YdspGp reg = cc->new_gp64 (name);

            if (auto* virt = cc->virt_reg_by_reg (reg))
                virt->set_weight (255);

            return reg;
        };

        regs[static_cast<size_t> (plan.induction)] = pinned ("sampleOffset");

        for (const auto& [slot, scale] : plan.streamScales)
            plan.streamEnds.emplace (slot, pinned ("streamEnd"));

        flagSettingArithmetic.insert (step);
        endRelativeLoops.push_back (std::move (plan));
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitEndRelativeLoopSetup (int preheaderBlock)
{
    for (const auto& loop : endRelativeLoops)
    {
        if (loop.headerBlock - 1 != preheaderBlock)
            continue;

        YdspGp count = cc->new_gp64 ("sampleCount");

        if constexpr (Traits::isX64)
        {
            emitExtendInt (count, gp (loop.bound));

            for (const auto& [slot, end] : loop.streamEnds)
            {
                const bool isInput = slot < activeFn->numInputs;
                const auto base = streamBase (loop.headerBlock, isInput, isInput ? slot : slot - activeFn->numInputs);

                cc->lea (end, memPtrIndexed (base, count, loop.streamScales.at (slot), 0));
            }

            moveGp (gp (loop.induction), count);
            cc->neg (gp (loop.induction));
        }
        else
        {
            cc->sxtw (count, gp (loop.bound));

            for (const auto& [slot, end] : loop.streamEnds)
            {
                const bool isInput = slot < activeFn->numInputs;
                const auto base = streamBase (loop.headerBlock, isInput, isInput ? slot : slot - activeFn->numInputs);

                cc->add (end, base, count, asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, loop.streamScales.at (slot)));
            }

            cc->neg (gp (loop.induction), count);
        }
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::planLoopPointers (const YdspIrFunction& fn)
{
    advancingPointerLoops.clear();
    loopPointerAccesses.clear();
    elidedLoopOffsetAdds.clear();

    const YdspValueUseCounts counts (fn);
    const int numBlocks = static_cast<int> (fn.blocks.size());

    const auto constantIntOf = [&] (int value) -> std::optional<int64_t>
    {
        if (! counts.definedOnce (value))
            return std::nullopt;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.result == value)
                    return inst.op == YdspIrOp::constI ? std::optional<int64_t> (inst.ivalue) : std::nullopt;

        return std::nullopt;
    };

    const auto isCallInLoop = [] (const YdspIrInst& inst) noexcept
    {
        switch (inst.op)
        {
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
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
            case YdspIrOp::roundF:
            case YdspIrOp::copysignF:
            case YdspIrOp::emitEvent:
            case YdspIrOp::traceInt:
            case YdspIrOp::traceFloat:
            case YdspIrOp::traceCommit:
                return true;

            default:
                return false;
        }
    };

    for (const auto& loop : fn.loops)
    {
        const int header = loop.headerBlock;
        const int preheader = header - 1;
        const int latch = loop.exitBlock - 1;

        if (loop.unrolled || header < 1 || latch <= header || latch >= numBlocks
            || loop.induction < 0 || static_cast<size_t> (loop.induction) >= valueTypes.size()
            || valueTypes[static_cast<size_t> (loop.induction)] != YdspValueType::int32Type)
            continue;

        if (std::any_of (endRelativeLoops.begin(), endRelativeLoops.end(), [header] (const EndRelativeLoop& er) { return er.headerBlock == header; }))
            continue;

        const auto& preheaderBlock = fn.blocks[static_cast<size_t> (preheader)];
        const auto& latchBlock = fn.blocks[static_cast<size_t> (latch)];

        const bool validPreheader = preheaderBlock.term == YdspIrTerm::fallthrough
                                 || (preheaderBlock.term == YdspIrTerm::branch && preheaderBlock.termTarget == header);

        if (! validPreheader || latchBlock.term != YdspIrTerm::branch || latchBlock.termTarget != header || latchBlock.insts.empty())
            continue;

        bool validCfg = true;

        for (int b = 0; b < numBlocks && validCfg; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];
            const bool inside = b >= header && b <= latch;
            const int fallthroughTarget = block.term == YdspIrTerm::fallthrough ? b + 1 : -1;

            for (const int target : { block.termTarget, block.termTarget2, fallthroughTarget })
            {
                if (target < 0)
                    continue;

                if (! inside && b != preheader && target >= header && target <= latch)
                    validCfg = false;

                if (inside && b != latch && target == header)
                    validCfg = false;
            }
        }

        if (! validCfg)
            continue;

        // Identify the single constant-step induction update at the end of latch.
        int latchIncIndex = -1;
        int latchMoveIndex = -1;
        int64_t stepConst = 0;
        const auto latchSize = static_cast<int> (latchBlock.insts.size());

        {
            const auto& lastInst = latchBlock.insts.back();

            if (lastInst.op == YdspIrOp::addI && lastInst.result == loop.induction
                && lastInst.a == loop.induction && ! lastInst.saturatingIntegerArithmetic)
            {
                latchIncIndex = latchSize - 1;
                const auto imm = integerImmediates.find ({ latch, latchIncIndex });
                const auto c = imm != integerImmediates.end() ? std::optional<int64_t> (imm->second) : constantIntOf (lastInst.b);

                if (c.has_value())
                    stepConst = *c;
            }
            else if (latchSize >= 2 && lastInst.op == YdspIrOp::movI && lastInst.result == loop.induction)
            {
                const auto& prevInst = latchBlock.insts[static_cast<size_t> (latchSize - 2)];

                if (prevInst.op == YdspIrOp::addI && prevInst.result == lastInst.a
                    && prevInst.result != loop.induction && prevInst.a == loop.induction
                    && ! prevInst.saturatingIntegerArithmetic)
                {
                    latchIncIndex = latchSize - 2;
                    latchMoveIndex = latchSize - 1;
                    const auto imm = integerImmediates.find ({ latch, latchIncIndex });
                    const auto c = imm != integerImmediates.end() ? std::optional<int64_t> (imm->second) : constantIntOf (prevInst.b);

                    if (c.has_value())
                        stepConst = *c;
                }
            }
        }

        if (latchIncIndex < 0 || stepConst < 1 || stepConst > 256)
            continue;

        // Verify induction is seeded once in preheader, updated only at latch,
        // and never written elsewhere.
        bool validDefs = true;
        int seedIndex = -1;

        for (int b = 0; b < numBlocks && validDefs; ++b)
        {
            const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;

            for (int i = 0; i < static_cast<int> (insts.size()); ++i)
            {
                if (insts[static_cast<size_t> (i)].result != loop.induction)
                    continue;

                if (b == preheader)
                {
                    if (seedIndex >= 0)
                        validDefs = false;

                    seedIndex = i;
                }
                else if (b == latch && i == (latchMoveIndex >= 0 ? latchMoveIndex : latchIncIndex))
                {
                    continue;
                }
                else
                {
                    validDefs = false;
                }
            }
        }

        if (! validDefs || seedIndex < 0)
            continue;

        const auto& seedInst = preheaderBlock.insts[static_cast<size_t> (seedIndex)];
        std::optional<int64_t> startConst;
        int startValue = -1;

        if (seedInst.op == YdspIrOp::constI)
        {
            startConst = seedInst.ivalue;
        }
        else if (seedInst.op == YdspIrOp::movI && seedInst.a >= 0 && seedInst.a != loop.induction)
        {
            startConst = constantIntOf (seedInst.a);

            if (! startConst.has_value())
            {
                startValue = seedInst.a;

                for (int k = seedIndex + 1; k < static_cast<int> (preheaderBlock.insts.size()); ++k)
                    if (preheaderBlock.insts[static_cast<size_t> (k)].result == startValue)
                        validDefs = false;
            }
        }
        else
        {
            continue;
        }

        if (! validDefs)
            continue;

        // Find `streamIdx = addI(invariantBase, induction)` within blocks in [header+1, latch].
        struct StreamOffsetInfo
        {
            int block = -1;
            int instIndex = -1;
            int baseValue = -1;
        };

        std::unordered_map<int, StreamOffsetInfo> streamOffsets;

        const auto isInvariantInLoop = [&] (int value) -> bool
        {
            if (value < 0 || value == loop.induction
                || static_cast<size_t> (value) >= valueTypes.size()
                || valueTypes[static_cast<size_t> (value)] != YdspValueType::int32Type)
                return false;

            for (int b = header; b <= latch; ++b)
                for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
                    if (inst.result == value)
                        return false;

            return true;
        };

        for (int b = header + 1; b <= latch; ++b)
        {
            const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;
            const int limit = (b == latch) ? latchIncIndex : static_cast<int> (insts.size());

            for (int i = 0; i < limit; ++i)
            {
                const auto& inst = insts[static_cast<size_t> (i)];

                if (inst.op != YdspIrOp::addI || inst.saturatingIntegerArithmetic
                    || inst.result < 0 || isVectorValue (inst.result)
                    || ! counts.definedOnce (inst.result))
                    continue;

                if (valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::int32Type)
                    continue;

                int baseVal = -1;

                if (inst.a == loop.induction && isInvariantInLoop (inst.b))
                    baseVal = inst.b;
                else if (inst.b == loop.induction && isInvariantInLoop (inst.a))
                    baseVal = inst.a;

                if (baseVal >= 0)
                    streamOffsets.emplace (inst.result, StreamOffsetInfo { b, i, baseVal });
            }
        }

        // Collect candidate state-array and stream accesses indexed by loop.induction
        // (or by a loop-invariant offset + loop.induction for streams).
        // Key: (kindInt, baseOrSlot, isInput, offsetValue)
        using PtrKey = std::tuple<int, int, bool, int>;

        struct PtrCandidate
        {
            LoopPointer pointer;
            std::vector<YdspInstPosition> positions;
            std::vector<int> replacedOffsetValues;
            double refs = 0.0;
            bool valid = true;
        };

        std::map<PtrKey, PtrCandidate> candidates;
        bool hasCall = false;

        for (int b = header; b <= latch; ++b)
        {
            const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;
            const int limit = (b == latch) ? latchIncIndex : static_cast<int> (insts.size());

            for (int i = 0; i < limit; ++i)
            {
                const auto& inst = insts[static_cast<size_t> (i)];

                if (isCallInLoop (inst))
                    hasCall = true;

                if (b == header)
                    continue;

                const YdspInstPosition pos { b, i };

                if (arrayDisplacements.count (pos) != 0)
                    continue;

                const bool isArrayLoad = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI;
                const bool isArrayStore = inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeStateArrayI;

                if (isArrayLoad || isArrayStore)
                {
                    if (inst.a != loop.induction)
                        continue;

                    const int valueId = isArrayLoad ? inst.result : inst.b;

                    if (valueId < 0 || static_cast<size_t> (valueId) >= valueTypes.size())
                        continue;

                    const auto type = valueTypes[static_cast<size_t> (valueId)];
                    const uint32_t elemBytes = is64BitValueType (type) ? 8u : 4u;
                    const int baseOffset = stateArrayBase (type, inst.memIndex);

                    const PtrKey key { 0, baseOffset, false, -1 };
                    auto& entry = candidates[key];

                    if (entry.positions.empty())
                    {
                        entry.pointer.kind = LoopPointer::Kind::stateArray;
                        entry.pointer.baseOffset = baseOffset;
                        entry.pointer.elemBytes = elemBytes;
                    }
                    else if (entry.pointer.elemBytes != elemBytes)
                    {
                        entry.valid = false;
                    }

                    entry.positions.push_back (pos);
                    entry.refs += referenceCost (b);
                    continue;
                }

                const bool isStreamLoad = inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadOutput;
                const bool isStreamStore = inst.op == YdspIrOp::storeOutput;

                if (isStreamLoad || isStreamStore)
                {
                    const bool isInput = inst.op == YdspIrOp::loadInput;
                    const int valueId = isStreamLoad ? inst.result : inst.b;

                    if (valueId < 0 || static_cast<size_t> (valueId) >= valueTypes.size())
                        continue;

                    if (! streamBase (b, isInput, inst.memIndex).is_valid())
                        continue;

                    int offsetVal = -1;
                    int replacedOffsetVal = -1;

                    if (inst.a != loop.induction)
                    {
                        const auto offIt = streamOffsets.find (inst.a);

                        if (offIt == streamOffsets.end() || offIt->second.block != b || offIt->second.instIndex >= i)
                            continue;

                        offsetVal = offIt->second.baseValue;
                        replacedOffsetVal = inst.a;
                    }

                    const auto type = valueTypes[static_cast<size_t> (valueId)];
                    const uint32_t elemBytes = is64BitValueType (type) ? 8u : 4u;

                    const PtrKey key { 1, inst.memIndex, isInput, offsetVal };
                    auto& entry = candidates[key];

                    if (entry.positions.empty())
                    {
                        entry.pointer.kind = LoopPointer::Kind::stream;
                        entry.pointer.isInput = isInput;
                        entry.pointer.streamSlot = inst.memIndex;
                        entry.pointer.streamAccessBlock = b;
                        entry.pointer.offsetValue = offsetVal;
                        entry.pointer.elemBytes = elemBytes;
                    }
                    else if (entry.pointer.elemBytes != elemBytes)
                    {
                        entry.valid = false;
                    }

                    entry.positions.push_back (pos);

                    if (replacedOffsetVal >= 0)
                        entry.replacedOffsetValues.push_back (replacedOffsetVal);

                    entry.refs += referenceCost (b);
                }
            }
        }

        std::vector<PtrCandidate> selected;

        for (auto& [key, cand] : candidates)
        {
            (void) key;
            if (cand.valid && ! cand.positions.empty())
                selected.push_back (std::move (cand));
        }

        if (selected.empty())
            continue;

        std::sort (selected.begin(), selected.end(), [] (const PtrCandidate& a, const PtrCandidate& b)
        {
            if (a.refs != b.refs)
                return a.refs > b.refs;

            return a.positions.size() > b.positions.size();
        });

        const size_t maxPointers = Traits::isX64 ? (hasCall ? 3u : 6u) : (hasCall ? 6u : 12u);

        if (selected.size() > maxPointers)
            selected.resize (maxPointers);

        AdvancingPointerLoop plan;
        plan.headerBlock = header;
        plan.latchBlock = latch;
        plan.induction = loop.induction;
        plan.step = static_cast<int> (stepConst);
        plan.startValue = startValue;
        plan.startConst = startConst;

        std::unordered_map<int, int> replacedOffsetUseCount;

        for (auto& cand : selected)
        {
            YdspGp ptr = cc->new_gp64 ("loopPtr");

            if (auto* virt = cc->virt_reg_by_reg (ptr))
                virt->set_weight (hasCall ? weightFor (cand.refs) : 255u);

            cand.pointer.ptrReg = ptr;

            for (const auto& pos : cand.positions)
                loopPointerAccesses.emplace (pos, ptr);

            for (const int offVal : cand.replacedOffsetValues)
                ++replacedOffsetUseCount[offVal];

            plan.pointers.push_back (cand.pointer);
        }

        for (const auto& [offVal, replacedCount] : replacedOffsetUseCount)
        {
            if (replacedCount == counts.useCount (offVal))
            {
                const auto& info = streamOffsets.at (offVal);
                elidedLoopOffsetAdds.emplace (info.block, info.instIndex);
            }
        }

        advancingPointerLoops.push_back (std::move (plan));
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitLoopPointerSetup (int preheaderBlock)
{
    for (const auto& loop : advancingPointerLoops)
    {
        if (loop.headerBlock - 1 != preheaderBlock)
            continue;

        for (const auto& lp : loop.pointers)
        {
            const uint32_t scaleLog2 = lp.elemBytes == 8 ? 3u : 2u;
            const YdspGp baseReg = lp.kind == LoopPointer::Kind::stateArray
                                     ? stateArraysReg
                                     : streamBase (lp.streamAccessBlock, lp.isInput, lp.streamSlot);
            const int32_t baseOffset = lp.kind == LoopPointer::Kind::stateArray ? lp.baseOffset : 0;

            if (lp.offsetValue < 0 && loop.startConst.has_value())
            {
                const int64_t totalOffset = static_cast<int64_t> (baseOffset)
                                          + (*loop.startConst) * static_cast<int64_t> (lp.elemBytes);

                if (totalOffset >= std::numeric_limits<int32_t>::min()
                    && totalOffset <= std::numeric_limits<int32_t>::max())
                {
                    emitAddPointerImm (lp.ptrReg, baseReg, static_cast<int32_t> (totalOffset));
                }
                else
                {
                    YdspGp off = cc->new_gp64 ("ptrOff");
                    cc->mov (off, asmjit::Imm (totalOffset));

                    if constexpr (Traits::isX64)
                        cc->lea (lp.ptrReg, asmjit::x86::ptr (baseReg, off));
                    else
                        cc->add (lp.ptrReg, baseReg, off);
                }

                continue;
            }

            YdspGp idx64 = cc->new_gp64 ("ptrIdx");

            if (lp.offsetValue >= 0)
            {
                emitExtendInt (idx64, gp (lp.offsetValue));

                if (loop.startConst.has_value() && *loop.startConst != 0)
                {
                    emitAddPointerImm (idx64, idx64, static_cast<int32_t> (*loop.startConst));
                }
                else if (! loop.startConst.has_value())
                {
                    YdspGp start64 = cc->new_gp64 ("ptrStart");
                    emitExtendInt (start64, gp (loop.startValue));

                    if constexpr (Traits::isX64)
                        cc->add (idx64, start64);
                    else
                        cc->add (idx64, idx64, start64);
                }
            }
            else
            {
                emitExtendInt (idx64, gp (loop.startValue));
            }

            if constexpr (Traits::isX64)
            {
                cc->lea (lp.ptrReg, memPtrIndexed (baseReg, idx64, scaleLog2, baseOffset));
            }
            else
            {
                if (baseOffset != 0)
                {
                    YdspGp basePlusOff = cc->new_gp64 ("ptrBase");
                    emitAddPointerImm (basePlusOff, baseReg, baseOffset);
                    cc->add (lp.ptrReg, basePlusOff, idx64, asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, static_cast<int> (scaleLog2)));
                }
                else
                {
                    cc->add (lp.ptrReg, baseReg, idx64, asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, static_cast<int> (scaleLog2)));
                }
            }
        }
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitLoopPointerAdvance (int latchBlock)
{
    for (const auto& loop : advancingPointerLoops)
    {
        if (loop.latchBlock != latchBlock)
            continue;

        for (const auto& lp : loop.pointers)
            emitAddPointerImm (lp.ptrReg, lp.ptrReg, static_cast<int32_t> (loop.step * static_cast<int> (lp.elemBytes)));
    }
}

//==============================================================================

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspMem YdspAsmJitCodegenImpl<Traits>::rawConstMem (uint64_t bits, bool is64)
{
    if (is64)
        return cc->new_const (asmjit::ConstPoolScope::kLocal, &bits, 8);

    const auto word = static_cast<uint32_t> (bits);
    return cc->new_const (asmjit::ConstPoolScope::kLocal, &word, 4);
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspMem YdspAsmJitCodegenImpl<Traits>::packedConstMem (uint64_t bits, bool is64)
{
    uint64_t lanes[2] = { bits, bits };

    if (! is64)
    {
        const auto word = bits & 0xffffffffull;
        lanes[0] = word | (word << 32);
        lanes[1] = lanes[0];
    }

    return cc->new_const (asmjit::ConstPoolScope::kLocal, lanes, 16);
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspMem YdspAsmJitCodegenImpl<Traits>::vectorConstMem (uint32_t bits)
{
    std::array<uint32_t, 16> lanes {};

    std::fill_n (lanes.begin(), static_cast<size_t> (activeVectorWidth), bits);
    return cc->new_const (asmjit::ConstPoolScope::kLocal, lanes.data(), static_cast<size_t> (activeVectorWidth) * sizeof (uint32_t));
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspMem YdspAsmJitCodegenImpl<Traits>::rampConstMem()
{
    std::array<float, 16> offsets {};

    for (size_t lane = 0; lane < offsets.size(); ++lane)
        offsets[lane] = static_cast<float> (lane);

    return cc->new_const (asmjit::ConstPoolScope::kLocal, offsets.data(), static_cast<size_t> (activeVectorWidth) * sizeof (float));
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::prepareRampConsts (const YdspIrFunction& fn)
{
    vrampRegs.clear();
    loopRampConsts.clear();

    const auto mayEmitCall = [] (const YdspIrInst& inst) noexcept
    {
        switch (inst.op)
        {
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
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
            case YdspIrOp::roundF:
            case YdspIrOp::copysignF:
            case YdspIrOp::emitEvent:
            case YdspIrOp::traceInt:
            case YdspIrOp::traceFloat:
            case YdspIrOp::traceCommit:
                return true;

            default:
                return false;
        }
    };

    const int numBlocks = static_cast<int> (fn.blocks.size());
    std::vector<bool> loopEligible (fn.loops.size(), false);
    std::vector<bool> loopHasCall (fn.loops.size(), false);

    for (size_t l = 0; l < fn.loops.size(); ++l)
    {
        const auto& loop = fn.loops[l];
        const int header = loop.headerBlock;
        const int exit = std::min (loop.exitBlock, numBlocks);

        if (loop.unrolled || header < 1 || exit <= header)
            continue;

        const auto& preheader = fn.blocks[static_cast<size_t> (header) - 1];
        bool entersThroughPreheader = preheader.term == YdspIrTerm::fallthrough
                                   || (preheader.term == YdspIrTerm::branch && preheader.termTarget == header);

        for (int b = 0; b < numBlocks && entersThroughPreheader; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];
            const bool inside = b >= header && b < exit;

            for (const int target : { block.termTarget, block.termTarget2, block.term == YdspIrTerm::fallthrough ? b + 1 : -1 })
                if (! inside && b != header - 1 && target >= header && target < exit)
                    entersThroughPreheader = false;
        }

        loopEligible[l] = entersThroughPreheader;

        if (! entersThroughPreheader)
            continue;

        for (int b = header; b < exit && ! loopHasCall[l]; ++b)
            for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
                if (mayEmitCall (inst))
                {
                    loopHasCall[l] = true;
                    break;
                }
    }

    std::unordered_map<int, YdspFp> loopRampReg;
    std::vector<YdspInstPosition> topLevelRamps;
    double topLevelReferences = 0.0;
    bool topLevelHasCall = false;

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        if (blockLoopScope[b] < 0 && ! topLevelHasCall)
            for (const auto& inst : fn.blocks[b].insts)
                if (mayEmitCall (inst))
                {
                    topLevelHasCall = true;
                    break;
                }

        for (size_t i = 0; i < fn.blocks[b].insts.size(); ++i)
        {
            if (fn.blocks[b].insts[i].op != YdspIrOp::vrampF)
                continue;

            int bestLoop = -1;

            for (size_t l = 0; l < fn.loops.size(); ++l)
            {
                if (! loopEligible[l] || loopHasCall[l])
                    continue;

                const auto& loop = fn.loops[l];

                if (static_cast<int> (b) >= loop.headerBlock && static_cast<int> (b) < std::min (loop.exitBlock, numBlocks))
                    if (bestLoop < 0 || loop.headerBlock < fn.loops[static_cast<size_t> (bestLoop)].headerBlock)
                        bestLoop = static_cast<int> (l);
            }

            const YdspInstPosition position { static_cast<int> (b), static_cast<int> (i) };

            if (bestLoop >= 0)
            {
                auto it = loopRampReg.find (bestLoop);

                if (it == loopRampReg.end())
                {
                    YdspFp ramp = newFpVector ("ramp");

                    if (auto* virt = cc->virt_reg_by_reg (ramp))
                        virt->set_weight (255);

                    it = loopRampReg.emplace (bestLoop, ramp).first;
                    loopRampConsts[fn.loops[static_cast<size_t> (bestLoop)].headerBlock - 1].push_back (ramp);
                }

                vrampRegs.emplace (position, it->second);
            }
            else if (blockLoopScope[b] < 0)
            {
                topLevelRamps.push_back (position);
                topLevelReferences += referenceCost (static_cast<int> (b));
            }
        }
    }

    if (topLevelRamps.size() >= 2 && ! topLevelHasCall)
    {
        YdspFp ramp = newFpVector ("ramp");

        if (auto* virt = cc->virt_reg_by_reg (ramp))
            virt->set_weight (weightFor (topLevelReferences));

        loadVectorFromMem (ramp, rampConstMem());

        for (const auto& position : topLevelRamps)
            vrampRegs.emplace (position, ramp);
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::loadFloatConst (const YdspFp& dst, double value, YdspValueType type)
{
    // Read the literal straight out of the constant pool. Materializing it via
    // an integer register instead would cost two instructions plus a
    // domain-crossing move, and would tie up a GP register for as long as the
    // constant is live - which, after LICM hoists every constant into the entry
    // block, is the whole kernel.
    const bool is64 = (type == YdspValueType::float64Type);
    const uint64_t bits = is64 ? doubleBits (value) : floatBits (value);

    if (bits == 0 && emitZeroFloatConst (dst, is64))
        return;

    loadFloatFromMem (dst, rawConstMem (bits, is64));
}

//==============================================================================
// Memory addressing

// Emits a load or store of a state float/int slot, handling the AArch64
// indexed-address-with-offset limitation by materializing the base. The
// element type drives the scale (4 or 8 bytes) and the load/store width.
template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitStateAccess (YdspValueType type, bool isLoad, const asmjit::Reg& reg, int base, int indexValue)
{
    const bool isFloat = isFloatValueType (type);
    const YdspMem mem = emitStateMem (type, base, indexValue);

    if (isFloat)
    {
        if (isLoad)
            loadFloatFromMem (reg.template as<YdspFp>(), mem);
        else
            storeFloatToMem (mem, reg.template as<YdspFp>());
    }
    else
    {
        if (isLoad)
            loadGpFromMem (reg.template as<YdspGp>(), mem);
        else
            storeGpToMem (mem, reg.template as<YdspGp>());
    }
}

//==============================================================================
// Instruction emission

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspGp YdspAsmJitCodegenImpl<Traits>::integerSaturationLimit (const YdspGp& a, const YdspGp& b, bool multiply)
{
    const bool wide = a.is_gp64();
    const auto sign = wide ? cc->new_gp64 ("overflowSign") : cc->new_gp32 ("overflowSign");
    const auto shift = wide ? cc->new_gp64 ("overflowShift") : cc->new_gp32 ("overflowShift");
    const auto maximum = wide ? cc->new_gp64 ("overflowMax") : cc->new_gp32 ("overflowMax");
    const auto limit = wide ? cc->new_gp64 ("overflowLimit") : cc->new_gp32 ("overflowLimit");
    if (multiply)
        intBinary (YdspIrOp::xorI, sign, a, b);
    else
        moveGp (sign, a);
    cc->mov (shift, asmjit::Imm (wide ? 63 : 31));
    intBinary (YdspIrOp::shrI, sign, sign, shift);
    cc->mov (maximum, asmjit::Imm (wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max()));
    intBinary (YdspIrOp::xorI, limit, sign, maximum);
    return limit;
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitSaturatingIntNeg (const YdspGp& dst, const YdspGp& src)
{
    const bool is64 = dst.is_gp64();
    const auto negated = is64 ? cc->new_gp64 ("negated") : cc->new_gp32 ("negated");
    const auto minimum = is64 ? cc->new_gp64 ("negMin") : cc->new_gp32 ("negMin");
    const auto maximum = is64 ? cc->new_gp64 ("negMax") : cc->new_gp32 ("negMax");
    intUnaryNeg (negated, src);
    cc->mov (minimum, asmjit::Imm (is64 ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min()));
    cc->mov (maximum, asmjit::Imm (is64 ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max()));
    emitIntCompareToFlags (negated, minimum);
    emitSelectIntOnFlags (YdspCond::kEqual, dst, maximum, negated);
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitInstruction (const YdspIrFunction& fn, const YdspIrInst& inst, int blockIndex, int instIndex)
{
    const YdspInstPosition position { blockIndex, instIndex };
    if ((inst.op == YdspIrOp::constI && elidedIntegerConstants.count (inst.result) != 0)
        || (inst.op == YdspIrOp::constF && elidedFloatConstants.count (inst.result) != 0)
        || elidedNegations.count (position) != 0
        || elidedLoopOffsetAdds.count (position) != 0)
        return;

    // The preheader seeds an end-relative loop's offset itself.
    if (std::any_of (endRelativeLoops.begin(), endRelativeLoops.end(), [&] (const EndRelativeLoop& loop) { return loop.seed == position; }))
        return;

    if (flagSettingArithmetic.count (position) != 0)
    {
        emitFlagSettingArithmetic (inst, position);
        return;
    }

    if constexpr (Traits::isX64)
    {
        if (flagPreservingArithmetic.count (position) != 0)
        {
            const auto immediate = integerImmediates.find (position);

            if (immediate != integerImmediates.end())
            {
                const auto imm = static_cast<int32_t> (immediate->second);
                const auto offset = inst.op == YdspIrOp::addI ? imm : -imm;
                cc->lea (gp (inst.result), asmjit::x86::ptr (gp (inst.a).r64(), offset));
            }
            else
            {
                cc->lea (gp (inst.result), asmjit::x86::ptr (gp (inst.a).r64(), gp (inst.b).r64()));
            }

            return;
        }
    }

    if (const auto displacement = arrayDisplacements.find (position); displacement != arrayDisplacements.end())
    {
        const bool load = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI;
        const int value = load ? inst.result : inst.b;
        const auto mem = memPtr (stateArraysReg, displacement->second);
        if (isVectorValue (value))
        {
            if (load)
                loadVectorFromMem (fp (value), mem);
            else
                storeVectorToMem (mem, fp (value));
        }
        else if (isFloatValueType (valueTypes[static_cast<size_t> (value)]))
        {
            if (load)
                loadFloatFromMem (fp (value), mem);
            else
                storeFloatToMem (mem, fp (value));
        }
        else
        {
            if (load)
                loadGpFromMem (gp (value), mem);
            else
                storeGpToMem (mem, gp (value));
        }
        return;
    }

    if (const auto immediate = integerImmediates.find (position); immediate != integerImmediates.end())
    {
        if constexpr (Traits::isX64)
        {
            if (gp (inst.result) != gp (inst.a))
                moveGp (gp (inst.result), gp (inst.a));

            const auto value = asmjit::Imm (static_cast<int32_t> (immediate->second));
            if (inst.op == YdspIrOp::addI)
                cc->add (gp (inst.result), value);
            else if (inst.op == YdspIrOp::subI)
                cc->sub (gp (inst.result), value);
            else
                cc->and_ (gp (inst.result), value);
        }
        else
        {
            if (inst.op == YdspIrOp::addI)
                cc->add (gp (inst.result), gp (inst.a), asmjit::Imm (immediate->second));
            else if (inst.op == YdspIrOp::subI)
                cc->sub (gp (inst.result), gp (inst.a), asmjit::Imm (immediate->second));
            else
                cc->and_ (gp (inst.result), gp (inst.a), asmjit::Imm (immediate->second));
        }
        return;
    }

    if (const auto product = fusedProductSubtracts.find (position); product != fusedProductSubtracts.end())
    {
        if constexpr (Traits::isX64)
        {
            const auto result = newFpOfType (valueTypes[static_cast<size_t> (inst.result)], "productDifference");
            moveFloat (result, fp (inst.a));
            if (valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float64Type)
                cc->vfmsub213sd (result, fp (inst.b), fp (product->second));
            else
                cc->vfmsub213ss (result, fp (inst.b), fp (product->second));
            moveFloat (fp (inst.result), result);
        }
        else
        {
            cc->fnmsub (fp (inst.result), fp (inst.a), fp (inst.b), fp (product->second));
        }
        return;
    }

    switch (inst.op)
    {
        case YdspIrOp::constF:
            // A packed constant would read 16 bytes from a 4-byte pool entry:
            // the vectoriser keeps constants scalar and splats them instead.
            jassert (! isVectorValue (inst.result));

            loadFloatConst (fp (inst.result), inst.fvalue, valueTypes[static_cast<size_t> (inst.result)]);
            return;
        case YdspIrOp::constI:
            cc->mov (gp (inst.result), asmjit::Imm (inst.ivalue));
            return;
        case YdspIrOp::constB:
            cc->mov (gp (inst.result), asmjit::Imm (inst.bvalue ? 1 : 0));
            return;

        case YdspIrOp::loadBlockSize:
            moveGp (gp (inst.result), numSamplesReg);
            return;
        case YdspIrOp::loadSampleRate:
            moveFloat (fp (inst.result), sampleRateReg);
            return;
        case YdspIrOp::loadEventFieldF:
            loadFloatFromMem (fp (inst.result), memPtr (eventCtxReg, nativeEventPayloadOffset (inst.memIndex)));
            return;
        case YdspIrOp::loadEventFieldI:
            loadGpFromMem (gp (inst.result), memPtr (eventCtxReg, nativeEventPayloadOffset (inst.memIndex)));
            return;
        case YdspIrOp::storeEventFieldF:
            storeFloatToMem (memPtr (outputEventsReg, nativeEventPayloadOffset (inst.memIndex)), fp (inst.a));
            return;
        case YdspIrOp::storeEventFieldI:
            storeGpToMem (memPtr (outputEventsReg, nativeEventPayloadOffset (inst.memIndex)), gp (inst.a));
            return;
        case YdspIrOp::traceInt:
        case YdspIrOp::traceFloat:
        case YdspIrOp::traceCommit:
        {
            auto target = cc->new_gp64 ("traceFn");
            const bool integer = inst.op == YdspIrOp::traceInt;
            const bool commit = inst.op == YdspIrOp::traceCommit;
            loadExternalSymbol (target, commit ? reinterpret_cast<const void*> (&ydspTraceCommit)
                                       : integer ? reinterpret_cast<const void*> (&ydspTraceInt)
                                                 : reinterpret_cast<const void*> (&ydspTraceFloat));
            auto index = cc->new_gp32 ("traceIndex");
            cc->mov (index, asmjit::Imm (inst.memIndex));
            YdspGp integerValue;
            if (integer)
            {
                integerValue = gp (inst.a);
                if (integerValue.size() != 8)
                {
                    auto widened = cc->new_gp64 ("traceValue");
                    emitExtendInt (widened, integerValue);
                    integerValue = widened;
                }
            }
            asmjit::InvokeNode* node = nullptr;
            const auto signature = commit ? asmjit::FuncSignature::build<void, void*, int32_t>()
                                 : integer ? asmjit::FuncSignature::build<void, void*, int32_t, int64_t>()
                                           : asmjit::FuncSignature::build<void, void*, int32_t, double>();
            if (cc->invoke (asmjit::Out (node), target, signature) == asmjit::kErrorOk && node != nullptr)
            {
                node->set_arg (0, traceQueueReg);
                node->set_arg (1, index);
                if (integer)
                    node->set_arg (2, integerValue);
                else if (! commit)
                    node->set_arg (2, fp (inst.a));
            }
            return;
        }
        case YdspIrOp::emitEvent:
            emitEmitEvent (inst);
            return;

        case YdspIrOp::loadParam:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];
            const auto mem = memPtr (paramsReg, paramOffsets[static_cast<size_t> (inst.a)]);

            if (isFloatValueType (type))
                loadFloatFromMem (fp (inst.result), mem);
            else
                loadGpFromMem (gp (inst.result), mem);

            return;
        }
        case YdspIrOp::storeParam:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.a)];
            const auto mem = memPtr (paramsReg, paramOffsets[static_cast<size_t> (inst.memIndex)]);

            if (isFloatValueType (type))
                storeFloatToMem (mem, fp (inst.a));
            else
                storeGpToMem (mem, gp (inst.a));

            return;
        }

        case YdspIrOp::loadParamOut:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];
            const auto mem = memPtr (paramOutReg, paramOutOffsets[static_cast<size_t> (inst.a)]);

            if (isFloatValueType (type))
                loadFloatFromMem (fp (inst.result), mem);
            else
                loadGpFromMem (gp (inst.result), mem);

            return;
        }
        case YdspIrOp::storeParamOut:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.a)];
            const auto mem = memPtr (paramOutReg, paramOutOffsets[static_cast<size_t> (inst.memIndex)]);

            if (isFloatValueType (type))
                storeFloatToMem (mem, fp (inst.a));
            else
                storeGpToMem (mem, gp (inst.a));

            return;
        }

        case YdspIrOp::loadStateF:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];
            emitStateAccess (type, true, fp (inst.result), stateScalarBase (type, inst.a), -1);
            return;
        }
        case YdspIrOp::storeStateF:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.a)];
            emitStateAccess (type, false, fp (inst.a), stateScalarBase (type, inst.memIndex), -1);
            return;
        }

        case YdspIrOp::loadStateI:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];
            emitStateAccess (type, true, gp (inst.result), stateScalarBase (type, inst.a), -1);
            return;
        }
        case YdspIrOp::storeStateI:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.a)];
            emitStateAccess (type, false, gp (inst.a), stateScalarBase (type, inst.memIndex), -1);
            return;
        }

        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::storeStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::storeStateArrayI:
        {
            const bool isLoad = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI;
            const int valueId = isLoad ? inst.result : inst.b;
            const auto type = valueTypes[static_cast<size_t> (valueId)];

            if (const auto ptrIt = loopPointerAccesses.find (position); ptrIt != loopPointerAccesses.end())
            {
                const auto mem = memPtr (ptrIt->second, 0);

                if (isVectorValue (valueId))
                {
                    if (isLoad)
                        loadVectorFromMem (fp (valueId), mem);
                    else
                        storeVectorToMem (mem, fp (valueId));
                }
                else if (isFloatValueType (type))
                {
                    if (isLoad)
                        loadFloatFromMem (fp (valueId), mem);
                    else
                        storeFloatToMem (mem, fp (valueId));
                }
                else
                {
                    if (isLoad)
                        loadGpFromMem (gp (valueId), mem);
                    else
                        storeGpToMem (mem, gp (valueId));
                }

                return;
            }

            if (isVectorValue (valueId))
            {
                const auto mem = emitVectorStateMem (stateArrayBase (type, inst.memIndex), inst.a);

                if (isLoad)
                    loadVectorFromMem (fp (valueId), mem);
                else
                    storeVectorToMem (mem, fp (valueId));

                return;
            }

            const asmjit::Reg reg = isFloatValueType (type) ? asmjit::Reg (fp (valueId)) : asmjit::Reg (gp (valueId));
            emitStateAccess (type, isLoad, reg, stateArrayBase (type, inst.memIndex), inst.a);
            return;
        }

        case YdspIrOp::loadInputAddress:
        case YdspIrOp::loadOutputAddress:
        {
            const auto base = streamBase (blockIndex, inst.op == YdspIrOp::loadInputAddress, inst.memIndex);

            if (base.is_valid())
                moveGp (gp (inst.result), base);

            return;
        }

        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::storeOutput:
        {
            const bool isStore = (inst.op == YdspIrOp::storeOutput);
            const bool isInput = (inst.op == YdspIrOp::loadInput);
            const int valueId = isStore ? inst.b : inst.result;

            const auto type = valueTypes[static_cast<size_t> (valueId)];

            if (const auto ptrIt = loopPointerAccesses.find (position); ptrIt != loopPointerAccesses.end())
            {
                const auto mem = memPtr (ptrIt->second, 0);

                if (isVectorValue (valueId))
                {
                    if (isStore)
                        storeVectorToMem (mem, fp (valueId));
                    else
                        loadVectorFromMem (fp (valueId), mem);
                }
                else if (isFloatValueType (type))
                {
                    if (isStore)
                        storeFloatToMem (mem, fp (valueId));
                    else
                        loadFloatFromMem (fp (valueId), mem);
                }
                else
                {
                    if (isStore)
                        storeGpToMem (mem, gp (valueId));
                    else
                        loadGpFromMem (gp (valueId), mem);
                }

                return;
            }

            // compile() planned a base for every stream slot referenced by
            // the IR, so a miss here means the two scans disagree.
            YdspGp base = streamBase (blockIndex, isInput, inst.memIndex);

            if (! base.is_valid())
                return;

            for (const auto& loop : endRelativeLoops)
                if (loop.induction == inst.a)
                    base = loop.streamEnds.at (isInput ? inst.memIndex : fn.numInputs + inst.memIndex);

            if (isVectorValue (valueId))
            {
                const auto mem = emitVectorStreamMem (base, inst.a);

                if (isStore)
                    storeVectorToMem (mem, fp (inst.b));
                else
                    loadVectorFromMem (fp (inst.result), mem);

                return;
            }

            const uint32_t scale = is64BitValueType (type) ? 3u : 2u;
            const YdspMem mem = memPtrIndexed (base, gp (inst.a), scale, 0);

            if (isFloatValueType (type))
            {
                if (isStore)
                    storeFloatToMem (mem, fp (inst.b));
                else
                    loadFloatFromMem (fp (inst.result), mem);
            }
            else
            {
                if (isStore)
                    storeGpToMem (mem, gp (inst.b));
                else
                    loadGpFromMem (gp (inst.result), mem);
            }

            return;
        }

        // ---- lane movement ----
        case YdspIrOp::vsplat:
            if (zeroVectorSplats.count (position) != 0 && emitZeroVector (fp (inst.result)))
                return;

            if (isFloatValueType (valueTypes[static_cast<size_t> (inst.result)]))
                emitSplatFloat (fp (inst.result), fp (inst.a));
            else
                emitSplatInt (fp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::vrampF:
        {
            YdspFp ramp;

            if (const auto hoisted = vrampRegs.find (position); hoisted != vrampRegs.end())
            {
                ramp = hoisted->second;
            }
            else
            {
                ramp = newFpVector ("ramp");
                loadVectorFromMem (ramp, rampConstMem());
            }

            emitSplatFloat (fp (inst.result), fp (inst.a));
            vectorBinary (YdspIrOp::addF, fp (inst.result), fp (inst.result), ramp);
            return;
        }

        case YdspIrOp::vreduceAddF:
            emitReduceAddFloat (fp (inst.result), fp (inst.a));
            return;

        case YdspIrOp::vinsertF:
            emitInsertFloat (fp (inst.result), inst.a >= 0 ? fp (inst.a) : YdspFp {}, fp (inst.b), inst.memIndex);
            return;

        case YdspIrOp::vextractF:
            emitExtractFloat (fp (inst.result), fp (inst.a), inst.memIndex);
            return;

        // ---- float arithmetic ----
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
            if (isVectorValue (inst.result))
                vectorBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b));
            else
                floatBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b));

            return;

        case YdspIrOp::modF:
        {
            // a % b = a - trunc (a / b) * b
            floatBinary (YdspIrOp::divF, fp (inst.result), fp (inst.a), fp (inst.b));
            roundFloat (fp (inst.result), 3);
            floatBinary (YdspIrOp::mulF, fp (inst.result), fp (inst.result), fp (inst.b));
            floatBinary (YdspIrOp::subF, fp (inst.result), fp (inst.a), fp (inst.result));
            return;
        }

        case YdspIrOp::negF:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
            if (isVectorValue (inst.result))
                vectorUnary (inst.op, fp (inst.result), fp (inst.a));
            else
                floatUnary (inst.op, fp (inst.result), fp (inst.a), valueTypes[static_cast<size_t> (inst.a)]);

            return;

        case YdspIrOp::clampF:
        {
            // The clamped value goes second in both steps: an x86 min/max
            // returns its second operand when either one is a NaN, so a NaN is
            // passed through there too, as AArch64 and wasm do.
            if (isVectorValue (inst.result))
            {
                YdspFp raised = newFpVector ("clampMax");
                vectorBinary (YdspIrOp::maxF, raised, fp (inst.b), fp (inst.a));
                vectorBinary (YdspIrOp::minF, fp (inst.result), fp (inst.c), raised);
                return;
            }

            YdspFp raised = is64BitValueType (valueTypes[static_cast<size_t> (inst.result)]) ? newFp64 ("clampMax") : newFp ("clampMax");
            floatBinary (YdspIrOp::maxF, raised, fp (inst.b), fp (inst.a));
            floatBinary (YdspIrOp::minF, fp (inst.result), fp (inst.c), raised);
            return;
        }

        case YdspIrOp::fmaF:
            if (isVectorValue (inst.result))
                emitVectorFusedMultiplyAdd (fp (inst.result), fp (inst.a), fp (inst.b), fp (inst.c));
            else
                emitFusedMultiplyAdd (fp (inst.result), fp (inst.a), fp (inst.b), fp (inst.c));

            return;
        case YdspIrOp::fmsubF:
            if (isVectorValue (inst.result))
                emitVectorFusedMultiplySubtract (fp (inst.result), fp (inst.a), fp (inst.b), fp (inst.c));
            else
                emitFusedMultiplySubtract (fp (inst.result), fp (inst.a), fp (inst.b), fp (inst.c));
            return;

        case YdspIrOp::lerpF:
            // a + (b - a) * t
            if (isVectorValue (inst.result))
            {
                vectorBinary (YdspIrOp::subF, fp (inst.result), fp (inst.b), fp (inst.a));
                vectorBinary (YdspIrOp::mulF, fp (inst.result), fp (inst.result), fp (inst.c));
                vectorBinary (YdspIrOp::addF, fp (inst.result), fp (inst.result), fp (inst.a));
                return;
            }

            floatBinary (YdspIrOp::subF, fp (inst.result), fp (inst.b), fp (inst.a));
            floatBinary (YdspIrOp::mulF, fp (inst.result), fp (inst.result), fp (inst.c));
            floatBinary (YdspIrOp::addF, fp (inst.result), fp (inst.result), fp (inst.a));
            return;

        // ---- int arithmetic ----
        case YdspIrOp::addI:
        case YdspIrOp::subI:
        case YdspIrOp::mulI:
        case YdspIrOp::andI:
        case YdspIrOp::orI:
        case YdspIrOp::xorI:
        case YdspIrOp::shlI:
        case YdspIrOp::shrI:
            if (isVectorValue (inst.result))
            {
                if ((inst.op == YdspIrOp::shlI || inst.op == YdspIrOp::shrI) && ! isVectorValue (inst.b))
                    vectorIntShift (inst.op, fp (inst.result), fp (inst.a), gp (inst.b));
                else
                    vectorIntBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b), inst.saturatingIntegerArithmetic);
                return;
            }

            if (inst.saturatingIntegerArithmetic && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI || inst.op == YdspIrOp::mulI))
                emitSaturatingIntBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            else
                intBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::negI:
            if (isVectorValue (inst.result))
            {
                vectorIntUnary (inst.op, fp (inst.result), fp (inst.a));
                return;
            }

            emitSaturatingIntNeg (gp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::divI:
        case YdspIrOp::modI:
            emitIntDivision (inst.op, gp (inst.result), gp (inst.a), gp (inst.b), valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int64Type);
            return;

        case YdspIrOp::wrapI:
            emitWrapInt (gp (inst.result), gp (inst.a), gp (inst.b));
            return;
        case YdspIrOp::advanceWrapI:
            emitAdvanceWrapInt (gp (inst.result), gp (inst.a), static_cast<int32_t> (inst.ivalue));
            return;

        case YdspIrOp::minI:
            if (isVectorValue (inst.result))
            {
                vectorIntBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b), false);
                return;
            }

            emitIntCompareToFlags (gp (inst.a), gp (inst.b));
            emitSelectIntOnFlags (YdspCond::kSignedLT, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::maxI:
            if (isVectorValue (inst.result))
            {
                vectorIntBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b), false);
                return;
            }

            emitIntCompareToFlags (gp (inst.a), gp (inst.b));
            emitSelectIntOnFlags (YdspCond::kSignedGT, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::clampI:
        {
            if (isVectorValue (inst.result))
            {
                vectorIntClamp (fp (inst.result), fp (inst.a), fp (inst.b), fp (inst.c));
                return;
            }

            const auto dst = gp (inst.result);
            YdspGp maxed = dst.is_gp64() ? cc->new_gp64 ("clampMax") : cc->new_gp32 ("clampMax");

            emitIntCompareToFlags (gp (inst.a), gp (inst.b));
            emitSelectIntOnFlags (YdspCond::kSignedGT, maxed, gp (inst.a), gp (inst.b));
            emitIntCompareToFlags (maxed, gp (inst.c));
            emitSelectIntOnFlags (YdspCond::kSignedLT, dst, maxed, gp (inst.c));
            return;
        }

        case YdspIrOp::absI:
        {
            if (isVectorValue (inst.result))
            {
                vectorIntUnary (inst.op, fp (inst.result), fp (inst.a));
                return;
            }

            const auto dst = gp (inst.result);
            const auto src = gp (inst.a);
            YdspGp negated = dst.is_gp64() ? cc->new_gp64 ("absNeg") : cc->new_gp32 ("absNeg");
            YdspGp zero = dst.is_gp64() ? cc->new_gp64 ("absZero") : cc->new_gp32 ("absZero");

            emitSaturatingIntNeg (negated, src);
            cc->mov (zero, asmjit::Imm (0));

            emitIntCompareToFlags (src, zero);
            emitSelectIntOnFlags (YdspCond::kSignedLT, dst, negated, src);
            return;
        }

        case YdspIrOp::signI:
        {
            const auto dst = gp (inst.result);
            const auto src = gp (inst.a);
            const bool is64 = dst.is_gp64();
            YdspGp zero = is64 ? cc->new_gp64 ("signZero") : cc->new_gp32 ("signZero");
            YdspGp one = is64 ? cc->new_gp64 ("signOne") : cc->new_gp32 ("signOne");
            YdspGp negOne = is64 ? cc->new_gp64 ("signNegOne") : cc->new_gp32 ("signNegOne");

            cc->mov (zero, asmjit::Imm (0));
            cc->mov (one, asmjit::Imm (1));
            cc->mov (negOne, asmjit::Imm (-1));

            emitIntCompareToFlags (src, zero);
            emitSelectIntOnFlags (YdspCond::kSignedGT, dst, one, zero);
            emitSelectIntOnFlags (YdspCond::kSignedLT, dst, negOne, dst);
            return;
        }

        // ---- conversions ----
        case YdspIrOp::itof:
            if (isVectorValue (inst.result))
                vectorIntToFloat (fp (inst.result), fp (inst.a));
            else
                emitIntToFloat (fp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::ftoi:
            if (isVectorValue (inst.result))
                vectorFloatToInt (fp (inst.result), fp (inst.a), inst.boundedFloatToInt);
            else
                emitFloatToInt (gp (inst.result), fp (inst.a), inst.boundedFloatToInt);
            return;

        case YdspIrOp::extI:
            emitExtendInt (gp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::truncI:
            emitTruncateInt (gp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::extF:
            emitExtendFloat (fp (inst.result), fp (inst.a));
            return;

        case YdspIrOp::truncF:
            emitTruncateFloat (fp (inst.result), fp (inst.a));
            return;

        case YdspIrOp::bitcastFtoI:
            if (isVectorValue (inst.result))
            {
                if (fp (inst.result) != fp (inst.a))
                    moveVector (fp (inst.result), fp (inst.a));
            }
            else
            {
                emitBitcastFloatToInt (gp (inst.result), fp (inst.a));
            }
            return;

        case YdspIrOp::bitcastItoF:
            if (isVectorValue (inst.result))
            {
                if (fp (inst.result) != fp (inst.a))
                    moveVector (fp (inst.result), fp (inst.a));
            }
            else
            {
                emitBitcastIntToFloat (fp (inst.result), gp (inst.a));
            }
            return;

        // ---- logic ----
        case YdspIrOp::andB:
        case YdspIrOp::orB:
            if (isVectorValue (inst.result))
            {
                vectorIntBinary (inst.op, fp (inst.result), fp (inst.a), fp (inst.b), false);
                return;
            }

            if (inst.saturatingIntegerArithmetic && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI || inst.op == YdspIrOp::mulI))
                emitSaturatingIntBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            else
                intBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::notB:
            if (isVectorValue (inst.result))
                vectorIntUnary (inst.op, fp (inst.result), fp (inst.a));
            else
                emitNotB (gp (inst.result), gp (inst.a));
            return;

        // ---- comparisons ----
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::ltF:
        case YdspIrOp::leF:
        case YdspIrOp::gtF:
        case YdspIrOp::geF:
            if (isVectorValue (inst.result))
            {
                vectorFloatCompare (inst.op, fp (inst.result), fp (inst.a), fp (inst.b));
                return;
            }

            if (elidedCompares.count ({ blockIndex, instIndex }) == 0)
                emitFloatCompare (conditionForComparison<YdspCond> (inst.op), fp (inst.a), fp (inst.b), gp (inst.result));

            return;

        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::ltI:
        case YdspIrOp::ltUI:
        case YdspIrOp::leI:
        case YdspIrOp::gtI:
        case YdspIrOp::geI:
            if (isVectorValue (inst.result))
            {
                vectorIntCompare (inst.op, fp (inst.result), fp (inst.a), fp (inst.b));
                return;
            }

            if (elidedCompares.count ({ blockIndex, instIndex }) == 0)
                emitIntCompare (conditionForComparison<YdspCond> (inst.op), gp (inst.a), gp (inst.b), gp (inst.result));

            return;

        // ---- moves ----
        case YdspIrOp::movF:
            if (isVectorValue (inst.result))
                moveVector (fp (inst.result), fp (inst.a));
            else
                moveFloat (fp (inst.result), fp (inst.a));

            return;
        case YdspIrOp::movI:
        case YdspIrOp::movB:
            if (isVectorValue (inst.result))
                moveVector (fp (inst.result), fp (inst.a));
            else
                moveGp (gp (inst.result), gp (inst.a));
            return;

        // ---- select ----
        case YdspIrOp::selectB:
        {
            if (isVectorValue (inst.result))
            {
                if (isFloatValueType (valueTypes[static_cast<size_t> (inst.result)]))
                    vectorSelectFloat (fp (inst.a), fp (inst.result), fp (inst.b), fp (inst.c));
                else
                    vectorSelectInt (fp (inst.a), fp (inst.result), fp (inst.b), fp (inst.c));
                return;
            }

            // Branchless on both targets: a select in a sample loop is almost
            // always data dependent, so a branch diamond here would be an
            // unpredictable branch per sample.
            const bool isFloatResult = isFloatValueType (valueTypes[static_cast<size_t> (inst.result)]);

            // The subtraction that produced the tested value left its sign in
            // the flags, and nothing since has touched them.
            if (const auto reused = flagReusingSelects.find ({ blockIndex, instIndex }); reused != flagReusingSelects.end())
            {
                const auto cond = reused->second ? YdspCond::kSign : YdspCond::kNotSign;

                if (isFloatResult)
                    emitSelectFloatOnFlags (cond, fp (inst.result), fp (inst.b), fp (inst.c));
                else
                    emitSelectIntOnFlags (cond, gp (inst.result), gp (inst.b), gp (inst.c));

                return;
            }

            // Consume the comparison's flags directly where that is legal,
            // rather than reading back the 0/1 register it would produce.
            if (const auto it = fusedSelectCompares.find ({ blockIndex, instIndex }); it != fusedSelectCompares.end())
            {
                const auto& compare = it->second;

                if (isFloatComparison (compare.op))
                    emitFloatCompareToFlags (fp (compare.a), fp (compare.b));
                else
                    emitPlannedIntCompareToFlags (compare);

                const auto cond = conditionForComparison<YdspCond> (compare.op);

                if (isFloatResult)
                    emitSelectFloatOnFlags (cond, fp (inst.result), fp (inst.b), fp (inst.c));
                else
                    emitSelectIntOnFlags (cond, gp (inst.result), gp (inst.b), gp (inst.c));

                return;
            }

            if (isFloatResult)
                emitSelectFloat (gp (inst.a), fp (inst.result), fp (inst.b), fp (inst.c));
            else
                emitSelectInt (gp (inst.a), gp (inst.result), gp (inst.b), gp (inst.c));

            return;
        }

        // ---- intrinsics (SLEEF/libm calls) ----
        // Scalar float32 values hit the SLEEF `u35` scalar function under
        // fastMath and libm otherwise; widened values call the 4-lane SLEEF set
        // (u35 under fastMath, u10 otherwise). See emitTranscendentalCall.
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
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
            emitTranscendentalCall (fn, inst, false);
            return;

        case YdspIrOp::powF:
        case YdspIrOp::atan2F:
        case YdspIrOp::fmodF:
            emitTranscendentalCall (fn, inst, true);
            return;

        case YdspIrOp::roundF:
            emitLibmUnary (&::roundf, &::round, inst);
            return;

        // copysign is a bit-twiddle, but lowering it branch-free needs a
        // float bitwise-op hook neither backend exposes yet; it stays on the
        // scalar libm path for now (see the plan's Phase 4 note).
        case YdspIrOp::copysignF:
            emitLibmBinary (&::copysignf, &::copysign, inst);
            return;

        case YdspIrOp::signF:
        {
            // sign(x) = (float) ((x > 0) - (x < 0))
            YdspGp gt = cc->new_gp32 ("gt");
            YdspGp lt = cc->new_gp32 ("lt");
            YdspGp diff = cc->new_gp32 ("diff");

            const auto operandType = valueTypes[static_cast<size_t> (inst.a)];

            YdspFp zero = newFpOfType (operandType, "zero");
            loadFloatConst (zero, 0.0, operandType);

            emitFloatCompareToReg (YdspCond::kUnsignedGT, fp (inst.a), zero, gt);
            emitFloatCompareToReg (YdspCond::kUnsignedLT, fp (inst.a), zero, lt);

            intBinary (YdspIrOp::subI, diff, gt, lt);
            emitIntToFloat (fp (inst.result), diff);
            return;
        }

        default:
            return;
    }
}

//==============================================================================

template <typename Traits>
bool YdspAsmJitCodegenImpl<Traits>::isVectorValue (int value) const noexcept
{
    return value >= 0
        && static_cast<size_t> (value) < valueLanes.size()
        && valueLanes[static_cast<size_t> (value)] > 1;
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspFp YdspAsmJitCodegenImpl<Traits>::newFpOfType (YdspValueType type, const char* name)
{
    return type == YdspValueType::float64Type ? newFp64 (name) : newFp (name);
}

template <typename Traits>
asmjit::Reg YdspAsmJitCodegenImpl<Traits>::newRegFor (const std::vector<YdspValueType>& types, const YdspIrInst& inst)
{
    const auto type = types[static_cast<size_t> (inst.result)];

    if (isVectorValue (inst.result))
    {
        if (type == YdspValueType::int32Type || type == YdspValueType::boolType)
            return newIntVector ("v");

        return newFpVector ("v");
    }

    if (type == YdspValueType::float64Type)
        return newFp64 ("v");

    if (type == YdspValueType::float32Type)
        return newFp ("v");

    if (type == YdspValueType::int64Type)
        return cc->new_gp64 ("v");

    return cc->new_gp32 ("v");
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspGp YdspAsmJitCodegenImpl<Traits>::gp (int value) const
{
    return regs[static_cast<size_t> (value)].template as<YdspGp>();
}

template <typename Traits>
typename YdspAsmJitCodegenImpl<Traits>::YdspFp YdspAsmJitCodegenImpl<Traits>::fp (int value) const
{
    return regs[static_cast<size_t> (value)].template as<YdspFp>();
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::moveGp (const YdspGp& dst, const YdspGp& src)
{
    cc->mov (dst, src);
}

//==============================================================================
// transcendental calls (libm, or SLEEF scalar under fastMath)

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitLibmUnary (float (*f32fn) (float), double (*f64fn) (double), const YdspIrInst& inst)
{
    const bool is64 = valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;

    // Strict (fastMath off) promises 1-ULP behavior, but a float32 libm call is
    // only ~1 ULP on some platforms (MSVC's tanhf() is ~1.1). Evaluate the
    // float32 case in float64 and round once - <=0.5 ULP everywhere - while the
    // default fastMath path keeps the cheaper float32 libm call.
    const bool promoteToDouble = ! is64 && activeFn != nullptr && ! activeFn->fastMath;
    const bool useDouble = is64 || promoteToDouble;

    YdspFp wideIn;

    if (promoteToDouble)
    {
        wideIn = newFp64 ("libmIn64");
        emitExtendFloat (wideIn, fp (inst.a));
    }

    asmjit::InvokeNode* node = nullptr;

    const auto* fnPtr = useDouble ? reinterpret_cast<const void*> (f64fn) : reinterpret_cast<const void*> (f32fn);

    YdspGp target;

    if (const auto it = libmTargetRegs.find (fnPtr); it != libmTargetRegs.end())
        target = it->second;
    else
    {
        target = cc->new_gp64 ("fn");
        loadExternalSymbol (target, fnPtr);
    }

    auto err = cc->invoke (asmjit::Out (node), target, useDouble ? asmjit::FuncSignature::build<double, double>() : asmjit::FuncSignature::build<float, float>());

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        if (promoteToDouble)
        {
            const auto wideOut = newFp64 ("libmOut64");
            node->set_arg (0, wideIn);
            node->set_ret (0, wideOut);
            emitTruncateFloat (fp (inst.result), wideOut);
        }
        else
        {
            node->set_arg (0, fp (inst.a));
            node->set_ret (0, fp (inst.result));
        }
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitLibmBinary (float (*f32fn) (float, float), double (*f64fn) (double, double), const YdspIrInst& inst)
{
    const bool is64 = valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;

    // See emitLibmUnary: strict float32 math is evaluated in float64 so the
    // result is correctly rounded rather than trusting the platform tanhf() etc.
    const bool promoteToDouble = ! is64 && activeFn != nullptr && ! activeFn->fastMath;
    const bool useDouble = is64 || promoteToDouble;

    YdspFp wideInA;
    YdspFp wideInB;

    if (promoteToDouble)
    {
        wideInA = newFp64 ("libmInA64");
        wideInB = newFp64 ("libmInB64");
        emitExtendFloat (wideInA, fp (inst.a));
        emitExtendFloat (wideInB, fp (inst.b));
    }

    asmjit::InvokeNode* node = nullptr;

    const auto* fnPtr = useDouble ? reinterpret_cast<const void*> (f64fn) : reinterpret_cast<const void*> (f32fn);

    YdspGp target;

    if (const auto it = libmTargetRegs.find (fnPtr); it != libmTargetRegs.end())
        target = it->second;
    else
    {
        target = cc->new_gp64 ("fn");
        loadExternalSymbol (target, fnPtr);
    }

    auto err = cc->invoke (asmjit::Out (node), target, useDouble ? asmjit::FuncSignature::build<double, double, double>() : asmjit::FuncSignature::build<float, float, float>());

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        if (promoteToDouble)
        {
            const auto wideOut = newFp64 ("libmOut64");
            node->set_arg (0, wideInA);
            node->set_arg (1, wideInB);
            node->set_ret (0, wideOut);
            emitTruncateFloat (fp (inst.result), wideOut);
        }
        else
        {
            node->set_arg (0, fp (inst.a));
            node->set_arg (1, fp (inst.b));
            node->set_ret (0, fp (inst.result));
        }
    }
}

//==============================================================================
// SLEEF float32 math lowering
//
// Native builds link sleef_library, whose canonical names the module exports
// (4-lane Sleef_<name>f4_u10/u35). Only the *widened* path uses SLEEF: a
// transcendental on a widened float32 value calls the 4-lane set - u35 under
// fastMath, u10 otherwise, so a strict (fastMath-off) vectorized loop keeps
// ~1 ULP parity with libm. Scalar transcendentals use libm except for bounded
// ARM64 fastMath exp lowering below. Measured SLEEF scalar calls (exp
// on Apple silicon) regressed against platform libm.
// float64 operands never leave libm either.

namespace
{

#if ! YUP_WASM
using SleefVectorUnaryFn = Sleef_float32x4 (*) (Sleef_float32x4);
using SleefVectorBinaryFn = Sleef_float32x4 (*) (Sleef_float32x4, Sleef_float32x4);
#define YDSP_SLEEF_ADDRESS(name) &name
#else
using SleefVectorUnaryFn = void (*)();
using SleefVectorBinaryFn = void (*)();
#define YDSP_SLEEF_ADDRESS(name) nullptr
#endif

struct SleefUnaryMathEntry
{
    const char* name;
    YdspIrOp op;
    float (*libmF32) (float);
    double (*libmF64) (double);
    SleefVectorUnaryFn sleefVectorU10;
    SleefVectorUnaryFn sleefVectorU35;
    bool hasU35;
};

const SleefUnaryMathEntry sleefUnaryMathTable[] = {
    { "sin", YdspIrOp::sinF, &::sinf, &::sin, YDSP_SLEEF_ADDRESS (Sleef_sinf4_u10), YDSP_SLEEF_ADDRESS (Sleef_sinf4_u35), true },
    { "cos", YdspIrOp::cosF, &::cosf, &::cos, YDSP_SLEEF_ADDRESS (Sleef_cosf4_u10), YDSP_SLEEF_ADDRESS (Sleef_cosf4_u35), true },
    { "tan", YdspIrOp::tanF, &::tanf, &::tan, YDSP_SLEEF_ADDRESS (Sleef_tanf4_u10), YDSP_SLEEF_ADDRESS (Sleef_tanf4_u35), true },
    { "asin", YdspIrOp::asinF, &::asinf, &::asin, YDSP_SLEEF_ADDRESS (Sleef_asinf4_u10), YDSP_SLEEF_ADDRESS (Sleef_asinf4_u35), true },
    { "acos", YdspIrOp::acosF, &::acosf, &::acos, YDSP_SLEEF_ADDRESS (Sleef_acosf4_u10), YDSP_SLEEF_ADDRESS (Sleef_acosf4_u35), true },
    { "atan", YdspIrOp::atanF, &::atanf, &::atan, YDSP_SLEEF_ADDRESS (Sleef_atanf4_u10), YDSP_SLEEF_ADDRESS (Sleef_atanf4_u35), true },
    { "sinh", YdspIrOp::sinhF, &::sinhf, &::sinh, YDSP_SLEEF_ADDRESS (Sleef_sinhf4_u10), YDSP_SLEEF_ADDRESS (Sleef_sinhf4_u35), true },
    { "cosh", YdspIrOp::coshF, &::coshf, &::cosh, YDSP_SLEEF_ADDRESS (Sleef_coshf4_u10), YDSP_SLEEF_ADDRESS (Sleef_coshf4_u35), true },
    { "tanh", YdspIrOp::tanhF, &::tanhf, &::tanh, YDSP_SLEEF_ADDRESS (Sleef_tanhf4_u10), YDSP_SLEEF_ADDRESS (Sleef_tanhf4_u35), true },
    { "asinh", YdspIrOp::asinhF, &::asinhf, &::asinh, YDSP_SLEEF_ADDRESS (Sleef_asinhf4_u10), nullptr, false },
    { "acosh", YdspIrOp::acoshF, &::acoshf, &::acosh, YDSP_SLEEF_ADDRESS (Sleef_acoshf4_u10), nullptr, false },
    { "atanh", YdspIrOp::atanhF, &::atanhf, &::atanh, YDSP_SLEEF_ADDRESS (Sleef_atanhf4_u10), nullptr, false },
    { "exp", YdspIrOp::expF, &::expf, &::exp, YDSP_SLEEF_ADDRESS (Sleef_expf4_u10), nullptr, false },
    { "log", YdspIrOp::logF, &::logf, &::log, YDSP_SLEEF_ADDRESS (Sleef_logf4_u10), YDSP_SLEEF_ADDRESS (Sleef_logf4_u35), true },
    { "log10", YdspIrOp::log10F, &::log10f, &::log10, YDSP_SLEEF_ADDRESS (Sleef_log10f4_u10), nullptr, false },
};

const SleefUnaryMathEntry* sleefUnaryEntry (YdspIrOp op) noexcept
{
    for (const auto& entry : sleefUnaryMathTable)
        if (entry.op == op)
            return &entry;

    return nullptr;
}

struct SleefBinaryMathEntry
{
    const char* name;
    YdspIrOp op;
    float (*libmF32) (float, float);
    double (*libmF64) (double, double);
    SleefVectorBinaryFn sleefVectorU10;
    SleefVectorBinaryFn sleefVectorU35;
    bool hasU35;
};

const SleefBinaryMathEntry sleefBinaryMathTable[] = {
    { "pow", YdspIrOp::powF, &::powf, &::pow, YDSP_SLEEF_ADDRESS (Sleef_powf4_u10), nullptr, false },
    { "atan2", YdspIrOp::atan2F, &::atan2f, &::atan2, YDSP_SLEEF_ADDRESS (Sleef_atan2f4_u10), YDSP_SLEEF_ADDRESS (Sleef_atan2f4_u35), true },
    { "fmod", YdspIrOp::fmodF, &::fmodf, &::fmod, YDSP_SLEEF_ADDRESS (Sleef_fmodf4), nullptr, false },
};

#undef YDSP_SLEEF_ADDRESS

const SleefBinaryMathEntry* sleefBinaryEntry (YdspIrOp op) noexcept
{
    for (const auto& entry : sleefBinaryMathTable)
        if (entry.op == op)
            return &entry;

    return nullptr;
}

String vectorMathSymbol (YdspIrOp op, bool fastMath)
{
    if (const auto* entry = sleefUnaryEntry (op))
        return String (entry->name) + (fastMath && entry->hasU35 ? ".v4.u35" : ".v4.u10");
    if (const auto* entry = sleefBinaryEntry (op))
        return String (entry->name) + (fastMath && entry->hasU35 ? ".v4.u35" : ".v4.u10");
    return {};
}

template <typename Visitor>
void visitNativeSymbols (Visitor&& visitor)
{
    const auto visit = [&] (const auto& entry)
    {
        visitor (String (entry.name) + ".f32", reinterpret_cast<const void*> (entry.libmF32));
        visitor (String (entry.name) + ".f64", reinterpret_cast<const void*> (entry.libmF64));
        visitor (String (entry.name) + ".v4.u10", reinterpret_cast<const void*> (entry.sleefVectorU10));
        if (entry.sleefVectorU35 != nullptr)
            visitor (String (entry.name) + ".v4.u35", reinterpret_cast<const void*> (entry.sleefVectorU35));
    };
    for (const auto& entry : sleefUnaryMathTable)
        visit (entry);
    for (const auto& entry : sleefBinaryMathTable)
        visit (entry);

    visitor ("round.f32", reinterpret_cast<const void*> (&::roundf));
    visitor ("round.f64", reinterpret_cast<const void*> (static_cast<double (*) (double)> (&::round)));
    visitor ("copysign.f32", reinterpret_cast<const void*> (&::copysignf));
    visitor ("copysign.f64", reinterpret_cast<const void*> (static_cast<double (*) (double, double)> (&::copysign)));
    visitor ("emit", reinterpret_cast<const void*> (&ydspCommitOutputEvent));
    visitor ("traceInt", reinterpret_cast<const void*> (&ydspTraceInt));
    visitor ("traceFloat", reinterpret_cast<const void*> (&ydspTraceFloat));
    visitor ("traceCommit", reinterpret_cast<const void*> (&ydspTraceCommit));
}

String nativeSymbolName (const void* address)
{
    String result;
    visitNativeSymbols ([&] (const String& name, const void* candidate)
                        {
        if (candidate == address && result.isEmpty())
            result = name;
    });
    return result;
}

#if ! YUP_WASM
const void* nativeSymbolAddress (const String& name)
{
    const void* result = nullptr;
    visitNativeSymbols ([&] (const String& candidate, const void* address)
                        {
        if (candidate == name)
            result = address;
    });
    return result;
}
#endif

} // namespace

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::loadExternalSymbol (const YdspGp& target, const void* address)
{
    loadExternalSymbol (target, nativeSymbolName (address));
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::loadExternalSymbol (const YdspGp& target, const String& name)
{
    jassert (name.isNotEmpty());
    auto [it, inserted] = externalSymbols.try_emplace (name);
    if (inserted)
        it->second = cc->new_label();

    if constexpr (Traits::isX64)
        loadGpFromMem (target, asmjit::x86::ptr (it->second));
    else
        loadGpFromMem (target, asmjit::a64::ptr (it->second));
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::materializeMathCallTargets (const YdspIrFunction& fn)
{
    std::unordered_set<void*> targets;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result < 0)
                continue;

            if (isVectorValue (inst.result))
            {
                const auto symbol = vectorMathSymbol (inst.op, fn.fastMath);
                if (symbol.isNotEmpty())
                    vectorMathTargetRegs.try_emplace (symbol);
                continue;
            }

            const auto is64 = inst.a >= 0 && valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;
            const bool useDouble = is64 || ! fn.fastMath;

            void* fnPtr = nullptr;

            switch (inst.op)
            {
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
                case YdspIrOp::expF:
                case YdspIrOp::logF:
                case YdspIrOp::log10F:
                {
                    if (const auto* entry = sleefUnaryEntry (inst.op))
                        fnPtr = useDouble ? reinterpret_cast<void*> (entry->libmF64) : reinterpret_cast<void*> (entry->libmF32);
                    break;
                }

                case YdspIrOp::powF:
                case YdspIrOp::atan2F:
                case YdspIrOp::fmodF:
                {
                    if (const auto* entry = sleefBinaryEntry (inst.op))
                        fnPtr = useDouble ? reinterpret_cast<void*> (entry->libmF64) : reinterpret_cast<void*> (entry->libmF32);
                    break;
                }

                case YdspIrOp::roundF:
                    fnPtr = useDouble ? reinterpret_cast<void*> (static_cast<double (*) (double)> (&::round))
                                      : reinterpret_cast<void*> (&::roundf);
                    break;

                case YdspIrOp::copysignF:
                    fnPtr = useDouble ? reinterpret_cast<void*> (static_cast<double (*) (double, double)> (&::copysign))
                                      : reinterpret_cast<void*> (&::copysignf);
                    break;

                default:
                    continue;
            }

            if (fnPtr != nullptr)
                targets.insert (fnPtr);
        }
    }

    for (auto& [symbol, reg] : vectorMathTargetRegs)
    {
        reg = cc->new_gp64 ("vectorMathFn");
        loadExternalSymbol (reg, symbol);
    }

    std::vector<const void*> orderedTargets (targets.begin(), targets.end());
    std::sort (orderedTargets.begin(), orderedTargets.end(), [] (const void* a, const void* b)
               {
        return nativeSymbolName (a) < nativeSymbolName (b);
    });

    for (auto* fnPtr : orderedTargets)
    {
        auto reg = cc->new_gp64 ("libmFn");
        loadExternalSymbol (reg, fnPtr);
        libmTargetRegs[fnPtr] = reg;
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitVectorMathInvoke (const String& symbol, bool isBinary, const YdspFp& ret, const YdspFp& a, const YdspFp& b)
{
    asmjit::InvokeNode* node = nullptr;

    const auto target = vectorMathTargetRegs.find (symbol);
    jassert (target != vectorMathTargetRegs.end());
    if (target == vectorMathTargetRegs.end())
        return;

    const auto sig = isBinary
                       ? asmjit::FuncSignature (asmjit::CallConvId::kCDecl,
                                                asmjit::FuncSignature::kNoVarArgs,
                                                asmjit::TypeId::kFloat32x4,
                                                asmjit::TypeId::kFloat32x4,
                                                asmjit::TypeId::kFloat32x4)
                       : asmjit::FuncSignature (asmjit::CallConvId::kCDecl,
                                                asmjit::FuncSignature::kNoVarArgs,
                                                asmjit::TypeId::kFloat32x4,
                                                asmjit::TypeId::kFloat32x4);

    const auto err = cc->invoke (asmjit::Out (node), target->second, sig);

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        node->set_arg (0, a);

        if (isBinary)
            node->set_arg (1, b);

        node->set_ret (0, ret);
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitInlineFastExp (const YdspIrInst& inst)
{
    if constexpr (! Traits::isX64)
    {
        constexpr auto type = YdspValueType::float32Type;

        // Minimax-style fit of e^x on [-1, 1] (degree 8, fp32-rounded
        // coefficients, ~1.2e-7 abs error). Clenshaw-free Estrin form keeps the
        // serial depth to four FMAs: two degree-3 chunks, then x^4 and x^8
        // corrections.
        const double coeffs[9] = { 1.0,
                                   0.9999998807907104,
                                   0.4999999701976776,
                                   0.16666798293590546,
                                   0.04166688770055771,
                                   0.008328596130013466,
                                   0.001388275995850563,
                                   0.00020469992887228727,
                                   2.5499197363387793e-05 };

        const auto loadConst = [&] (double value)
        {
            auto reg = newFpOfType (type, "expC");
            loadFloatConst (reg, value, type);
            return reg;
        };

        // |x| > 1 goes back to the libm call: the poly only models [-1, 1]. The
        // branch is cold for the envelope and modal-decay uses this form targets.
        YdspFp absX = newFpOfType (type, "expAbsX");
        floatUnary (YdspIrOp::absF, absX, fp (inst.a), type);

        YdspFp one = newFpOfType (type, "expOne");
        loadFloatConst (one, 1.0, type);

        YdspGp outOfRange = cc->new_gp32 ("expOutOfRange");
        emitFloatCompareToReg (YdspCond::kUnsignedGT, absX, one, outOfRange);

        asmjit::Label cold = cc->new_label();
        asmjit::Label join = cc->new_label();
        cc->cbnz (outOfRange, cold);

        const auto x = fp (inst.a);

        YdspFp x2 = newFpOfType (type, "expX2");
        YdspFp x4 = newFpOfType (type, "expX4");
        floatBinary (YdspIrOp::mulF, x2, x, x);
        floatBinary (YdspIrOp::mulF, x4, x2, x2);

        YdspFp p0 = newFpOfType (type, "expP0");
        YdspFp p1 = newFpOfType (type, "expP1");
        YdspFp p2 = newFpOfType (type, "expP2");
        YdspFp p3 = newFpOfType (type, "expP3");

        emitFusedMultiplyAdd (p0, x, loadConst (coeffs[1]), loadConst (coeffs[0]));
        emitFusedMultiplyAdd (p1, x, loadConst (coeffs[3]), loadConst (coeffs[2]));
        emitFusedMultiplyAdd (p2, x, loadConst (coeffs[5]), loadConst (coeffs[4]));
        emitFusedMultiplyAdd (p3, x, loadConst (coeffs[7]), loadConst (coeffs[6]));

        YdspFp q0 = newFpOfType (type, "expQ0");
        YdspFp q1 = newFpOfType (type, "expQ1");
        emitFusedMultiplyAdd (q0, x2, p1, p0);
        emitFusedMultiplyAdd (q1, x2, p3, p2);

        YdspFp t = newFpOfType (type, "expT");
        emitFusedMultiplyAdd (t, x4, loadConst (coeffs[8]), q1);
        emitFusedMultiplyAdd (fp (inst.result), x4, t, q0);

        cc->b (join);
        cc->bind (cold);
        emitLibmUnary (&::expf, &::exp, inst);
        cc->bind (join);
    }
    else
    {
        emitLibmUnary (&::expf, &::exp, inst);
    }
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitTranscendentalCall (const YdspIrFunction& fn, const YdspIrInst& inst, bool isBinary)
{
    if (! isVectorValue (inst.result))
    {
        if constexpr (! Traits::isX64)
        {
            // fastMath scalar float32 exp is inlined on AArch64 so the per-sample
            // libm call (and the register spill round-trip the allocator wraps it
            // in) can be measured against a straight-line form. See
            // emitInlineFastExp.
            if (fn.fastMath && ! isBinary && inst.op == YdspIrOp::expF
                && valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float32Type)
            {
                emitInlineFastExp (inst);
                return;
            }
        }

        // Remaining scalar calls use platform libm; measured SLEEF scalar
        // calls regressed against it (exp on Apple silicon).
        if (isBinary)
        {
            if (const auto* entry = sleefBinaryEntry (inst.op))
                emitLibmBinary (entry->libmF32, entry->libmF64, inst);
            else
                jassertfalse; // every scalar binary transcendental has a libm pair
        }
        else
        {
            if (const auto* entry = sleefUnaryEntry (inst.op))
                emitLibmUnary (entry->libmF32, entry->libmF64, inst);
            else
                jassertfalse; // every scalar unary transcendental has a libm pair
        }

        return;
    }

    // Widened path: the 4-lane SLEEF set, u35 under fastMath with a fallback
    // to u10 for the functions that only ship u10.
    const auto vectorSymbol = vectorMathSymbol (inst.op, fn.fastMath);

    if (vectorSymbol.isEmpty())
        return;

    const auto a = fp (inst.a);
    const auto b = isBinary ? fp (inst.b) : YdspFp();

    if (activeVectorWidth == 4)
    {
        emitVectorMathInvoke (vectorSymbol, isBinary, fp (inst.result), a, b);
        return;
    }

    if constexpr (Traits::isX64)
    {
        if (activeVectorWidth == 8)
        {
            // Split the 8-lane value into two 4-lane calls - SLEEF ships no
            // 8-lane single-precision set. Each call clobbers the caller-saved
            // vector registers, so an operand computed before the other call, or
            // a result that must outlive it, would be read back from a clobbered
            // register. Stage both operands and the first result through a small
            // stack buffer instead, so nothing vector-shaped is left live in a
            // register across a call.
            if (! sleefScratchReady)
            {
                sleefScratch = cc->new_stack (64, 16, "sleefScratch");
                sleefScratchReady = true;
            }

            const auto slot = [&] (int32_t byteOffset)
            {
                YdspMem mem = sleefScratch;
                mem.add_offset (byteOffset);
                return mem;
            };

            // Only the two operands that must outlive a call touch memory: the
            // second call's input (staged before the first call) and the first
            // call's result (staged before the second). Each half used by the
            // call that is about to run stays in a register, so the per-site
            // cost is one 16-byte store plus one 16-byte load.
            YdspFp loIn = newFp128 ("sleefLoIn");
            YdspFp hiIn = newFp128 ("sleefHiIn");
            cc->vextractf128 (loIn, a, asmjit::Imm (0));
            cc->vextractf128 (hiIn, a, asmjit::Imm (1));
            cc->vmovups (slot (0), hiIn);

            YdspFp loInB;
            YdspFp hiInB;
            if (isBinary)
            {
                loInB = newFp128 ("sleefLoInB");
                hiInB = newFp128 ("sleefHiInB");
                cc->vextractf128 (loInB, b, asmjit::Imm (0));
                cc->vextractf128 (hiInB, b, asmjit::Imm (1));
                cc->vmovups (slot (16), hiInB);
            }

            YdspFp loResult = newFp128 ("sleefLoRes");
            emitVectorMathInvoke (vectorSymbol, isBinary, loResult, loIn, loInB);
            cc->vmovups (slot (32), loResult);

            YdspFp hiInReload = newFp128 ("sleefHiInR");
            cc->vmovups (hiInReload, slot (0));

            YdspFp hiInBReload;
            if (isBinary)
            {
                hiInBReload = newFp128 ("sleefHiInBR");
                cc->vmovups (hiInBReload, slot (16));
            }

            YdspFp hiResult = newFp128 ("sleefHiRes");
            emitVectorMathInvoke (vectorSymbol, isBinary, hiResult, hiInReload, hiInBReload);

            YdspFp loReload = newFp128 ("sleefLoReload");
            cc->vmovups (loReload, slot (32));

            YdspFp dst = newFpVector ("sleefRes");
            cc->vxorps (dst, dst, dst);
            cc->vinsertf128 (dst, dst, loReload, asmjit::Imm (0));
            cc->vinsertf128 (dst, dst, hiResult, asmjit::Imm (1));
            moveVector (fp (inst.result), dst);
            return;
        }
    }

    jassertfalse; // an unsupported active vector width reached a transcendental
}

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitEmitEvent (const YdspIrInst& inst)
{
    // AArch64's BLR only takes a register operand, so the target address
    // must be materialized rather than passed as an immediate.
    YdspGp target = cc->new_gp64 ("fn");
    loadExternalSymbol (target, reinterpret_cast<const void*> (&ydspCommitOutputEvent));

    YdspGp shapeTag = cc->new_gp64 ("shapeTag");
    cc->mov (shapeTag, asmjit::Imm (inst.ivalue));

    YdspGp endpointIndex = cc->new_gp32 ("endpointIndex");
    cc->mov (endpointIndex, asmjit::Imm (inst.memIndex));

    asmjit::InvokeNode* node = nullptr;

    auto err = cc->invoke (asmjit::Out (node), target, asmjit::FuncSignature::build<void, void*, int64_t, int32_t, int32_t>());

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        node->set_arg (0, outputEventsReg);
        node->set_arg (1, shapeTag);
        node->set_arg (2, gp (inst.a));
        node->set_arg (3, endpointIndex);
    }
}

//==============================================================================
// Terminators

template <typename Traits>
void YdspAsmJitCodegenImpl<Traits>::emitTerminator (const YdspIrBlock& sourceBlock, int blockIndex)
{
    const auto* terminatorBlock = &sourceBlock;
    int compareBlockIndex = blockIndex;

    if (sourceBlock.term == YdspIrTerm::branch
        && sourceBlock.termTarget >= 0 && sourceBlock.termTarget < blockIndex)
    {
        const auto& header = activeFn->blocks[static_cast<size_t> (sourceBlock.termTarget)];
        if (header.insts.size() == 1 && header.term == YdspIrTerm::branchIf
            && header.termTarget == sourceBlock.termTarget + 1
            && header.termTarget2 == blockIndex + 1
            && fusedBranchCompares.count (sourceBlock.termTarget) != 0)
        {
            // A comparison-only header has no effects to replay. Test at the
            // latch and jump straight to the body; retain the entry guard.
            terminatorBlock = &header;
            compareBlockIndex = sourceBlock.termTarget;
        }
    }

    const auto& block = *terminatorBlock;

    // Blocks are emitted in index order, so a jump to blockIndex + 1 targets
    // the label bound on the very next line: skip it and let the code fall
    // through (asmjit's register allocator recovers the edge from the bound
    // label).
    const auto isNextBlock = [blockIndex] (int target)
    {
        return target == blockIndex + 1;
    };

    switch (block.term)
    {
        case YdspIrTerm::fallthrough:
            // By definition the target is blockIndex + 1, which is exactly the
            // label bound next - nothing to emit.
            return;

        case YdspIrTerm::branch:
            if (! isNextBlock (block.termTarget))
                jump (blockLabels[static_cast<size_t> (block.termTarget)]);

            return;

        case YdspIrTerm::branchIf:
        {
            if (block.termTarget < 0 || block.termTarget2 < 0)
                return;

            const auto thenLabel = blockLabels[static_cast<size_t> (block.termTarget)];
            const auto elseLabel = blockLabels[static_cast<size_t> (block.termTarget2)];

            if (const auto it = fusedBranchCompares.find (compareBlockIndex); it != fusedBranchCompares.end())
            {
                const auto& compare = it->second;

                const auto endRelative = std::find_if (endRelativeLoops.begin(), endRelativeLoops.end(), [&] (const EndRelativeLoop& loop)
                {
                    return loop.headerBlock == compareBlockIndex;
                });

                if (endRelative != endRelativeLoops.end() && blockIndex == endRelative->latchBlock)
                {
                    // The increment set the flags, and the offset is zero
                    // exactly when the last sample is done.
                    branchOnFlags (YdspCond::kNotEqual, true, thenLabel);
                    return;
                }

                if (endRelative != endRelativeLoops.end())
                    cc->cmp (gp (endRelative->induction), asmjit::Imm (0));
                else if (isFloatComparison (compare.op))
                    emitFloatCompareToFlags (fp (compare.a), fp (compare.b));
                else
                    emitPlannedIntCompareToFlags (compare);

                const auto cond = conditionForComparison<YdspCond> (compare.op);

                if (isNextBlock (block.termTarget))
                {
                    branchOnFlags (cond, false, elseLabel);
                    return;
                }

                branchOnFlags (cond, true, thenLabel);

                if (! isNextBlock (block.termTarget2))
                    jump (elseLabel);

                return;
            }

            // Falling through to whichever arm is physically next removes the
            // unconditional jump; inverting the test is what makes the common
            // "branch into the body, fall through to the exit" shape free.
            if (isNextBlock (block.termTarget))
            {
                branchIfZero (gp (block.termCond), elseLabel);
                return;
            }

            branchIfNotZero (gp (block.termCond), thenLabel);

            if (! isNextBlock (block.termTarget2))
                jump (elseLabel);

            return;
        }

        default:
            return;
    }
}

//==============================================================================

template <typename Traits>
int YdspAsmJitCodegenImpl<Traits>::stateScalarBase (YdspValueType type, int slot) const
{
    return activeFn->stateScalarByteOffset (type, slot);
}

template <typename Traits>
int YdspAsmJitCodegenImpl<Traits>::stateArrayBase (YdspValueType type, int element) const
{
    return activeFn->stateArrayByteOffset (type, element);
}

//==============================================================================

ResultValue<YdspNativeArtifact> YdspAsmJitCodegen::emit (const YdspTargetTriple& target, const YdspIrFunction& fn, YdspDiagnostics& diagnostics)
{
    if ((target.architecture != YdspTargetArchitecture::x64 && target.architecture != YdspTargetArchitecture::arm64)
        || (target.operatingSystem != YdspTargetOperatingSystem::windowsTarget
            && target.operatingSystem != YdspTargetOperatingSystem::macosTarget
            && target.operatingSystem != YdspTargetOperatingSystem::linuxTarget))
    {
        diagnostics.addError ({}, "Unsupported native target");
        return makeResultValueFail ("Unsupported native target");
    }

    const auto arch = target.architecture == YdspTargetArchitecture::x64 ? asmjit::Arch::kX64 : asmjit::Arch::kAArch64;
    const auto platform = target.operatingSystem == YdspTargetOperatingSystem::windowsTarget ? asmjit::Platform::kWindows
                        : target.operatingSystem == YdspTargetOperatingSystem::macosTarget   ? asmjit::Platform::kOSX
                                                                                             : asmjit::Platform::kLinux;
    const auto abi = target.operatingSystem == YdspTargetOperatingSystem::windowsTarget ? asmjit::PlatformABI::kMSVC
                   : target.operatingSystem == YdspTargetOperatingSystem::macosTarget   ? asmjit::PlatformABI::kDarwin
                                                                                        : asmjit::PlatformABI::kGNU;
    const asmjit::Environment environment (arch, asmjit::SubArch::kUnknown, asmjit::Vendor::kUnknown, platform, abi);
    auto result = target.architecture == YdspTargetArchitecture::x64
                    ? YdspAsmJitCodegenX64().emit (environment, fn, diagnostics, fn.isEventHandler)
                    : YdspAsmJitCodegenARM64().emit (environment, fn, diagnostics, fn.isEventHandler);
    if (! result && ! diagnostics.hasErrors())
        diagnostics.addError ({}, result.getErrorMessage());
    return result;
}

#if ! YUP_WASM
namespace
{
YdspTargetTriple nativeHostTriple()
{
    YdspTargetTriple target;
#if YUP_WINDOWS
    target.operatingSystem = YdspTargetOperatingSystem::windowsTarget;
#elif YUP_MAC
    target.operatingSystem = YdspTargetOperatingSystem::macosTarget;
#else
    target.operatingSystem = YdspTargetOperatingSystem::linuxTarget;
#endif
#if ASMJIT_ARCH_ARM == 64
    target.architecture = YdspTargetArchitecture::arm64;
#else
    target.architecture = YdspTargetArchitecture::x64;
#endif
    return target;
}
} // namespace

YdspKernelFn YdspAsmJitCodegen::load (asmjit::JitRuntime& runtime, const YdspTargetTriple& target, const YdspNativeArtifact& artifact, YdspDiagnostics& diagnostics)
{
    const auto fail = [&] (const String& message) -> YdspKernelFn
    {
        diagnostics.addError ({}, message);
        return nullptr;
    };

    if (target != nativeHostTriple() || sizeof (void*) != 8)
        return fail ("Native artifact target does not match the current process");
    if (artifact.code.empty())
        return fail ("Native artifact has no code");

    auto bytes = artifact.code;
    for (const auto& symbol : artifact.symbols)
    {
        if (symbol.offset > bytes.size() || bytes.size() - static_cast<size_t> (symbol.offset) < sizeof (uint64_t))
            return fail ("Native artifact has an invalid symbol offset");
        const auto* address = nativeSymbolAddress (symbol.name);
        if (address == nullptr)
            return fail ("Unknown native helper: " + symbol.name);
        const auto bits = static_cast<uint64_t> (reinterpret_cast<uintptr_t> (address));
        std::memcpy (bytes.data() + symbol.offset, &bits, sizeof (bits));
    }

    asmjit::CodeHolder code;
    if (code.init (runtime.environment(), runtime.cpu_features()) != asmjit::kErrorOk)
        return fail ("Failed to initialize native artifact loader");
    // An architecture assembler is only used to copy already-emitted bytes.
    asmjit::Error error;
    if (target.architecture == YdspTargetArchitecture::x64)
    {
        asmjit::x86::Assembler emitter (&code);
        error = emitter.embed (bytes.data(), bytes.size());
    }
    else
    {
        asmjit::a64::Assembler emitter (&code);
        error = emitter.embed (bytes.data(), bytes.size());
    }
    YdspKernelFn result = nullptr;
    if (error != asmjit::kErrorOk || runtime.add (&result, &code) != asmjit::kErrorOk)
        return fail ("Failed to install native artifact");
    return result;
}

YdspKernelFn YdspAsmJitCodegen::compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize)
{
    if (generatedCodeSize != nullptr)
        *generatedCodeSize = 0;
    auto artifact = runtime.environment().arch() == asmjit::Arch::kX64
                      ? YdspAsmJitCodegenX64().emit (runtime.environment(), fn, diagnostics, fn.isEventHandler)
                      : YdspAsmJitCodegenARM64().emit (runtime.environment(), fn, diagnostics, fn.isEventHandler);
    if (! artifact)
    {
        if (! diagnostics.hasErrors())
            diagnostics.addError ({}, artifact.getErrorMessage());
        return nullptr;
    }
    if (generatedCodeSize != nullptr)
        *generatedCodeSize = artifact.getReference().code.size();
    return load (runtime, nativeHostTriple(), artifact.getReference(), diagnostics);
}

YdspEventHandlerFn YdspAsmJitCodegen::compileEventHandler (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize)
{
    return ydspFnPtrCast<YdspEventHandlerFn> (compile (runtime, fn, diagnostics, generatedCodeSize));
}

#endif

} // namespace yup
