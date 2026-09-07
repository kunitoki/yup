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

bool isIntComparison (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::ltI:
        case YdspIrOp::leI:
        case YdspIrOp::gtI:
        case YdspIrOp::geI:
            return true;

        default:
            return false;
    }
}

// The condition each comparison opcode tests. Float comparisons use the
// unsigned codes: comiss/fcmp report an ordered result through the carry flag,
// so the unsigned forms are the ones that exclude NaN.
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
#if ASMJIT_ARCH_X86
        static constexpr asmjit::InstId moveIds[] =
        {
            asmjit::x86::Inst::kIdMov,
            asmjit::x86::Inst::kIdMovaps,
            asmjit::x86::Inst::kIdMovups,
            asmjit::x86::Inst::kIdMovapd,
            asmjit::x86::Inst::kIdMovupd,
            asmjit::x86::Inst::kIdMovdqa,
            asmjit::x86::Inst::kIdMovdqu,
            asmjit::x86::Inst::kIdMovq,
            asmjit::x86::Inst::kIdMovss,
            asmjit::x86::Inst::kIdMovsd,
        };
#elif ASMJIT_ARCH_ARM
        static constexpr asmjit::InstId moveIds[] =
        {
            asmjit::a64::Inst::kIdMov,
            asmjit::a64::Inst::kIdFmov_v,
            asmjit::a64::Inst::kIdMov_v,
        };
#else
        static constexpr asmjit::InstId moveIds[] = {};
#endif
        for (const auto id : moveIds)
            if (instId == id)
                return true;

        return false;
    }
};

} // namespace

//==============================================================================

YdspKernelFn YdspAsmJitCodegenImpl::compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, bool isEventHandler, size_t* generatedCodeSize)
{
    if (generatedCodeSize != nullptr)
        *generatedCodeSize = 0;

    asmjit::CodeHolder code;

    // Attach diagnostics so asmjit logs are captured for error reporting
    AsmJitErrorCollector errorCollector;
    asmjit::StringLogger logger;
    code.set_logger (&logger);
    code.set_error_handler (&errorCollector);

    // Include binary machine code in the log alongside assembly
    logger.add_flags (asmjit::FormatFlags::kMachineCode);

    if (code.init (runtime.environment(), runtime.cpu_features()) != asmjit::kErrorOk)
    {
        diagnostics.addError (0, 0, "Failed to initialize the code holder");
        return nullptr;
    }

    YdspAsm cc (&code);

    // Drop no-op register moves the codegen emitted (see RedundantMoveCleanupPass).
    cc.add_pass<RedundantMoveCleanupPass>();

    auto* funcNode = cc.add_func (asmjit::FuncSignature::build<void, void*>());
    if (funcNode == nullptr)
    {
        String msg = "Failed to create the kernel function";
        if (! errorCollector.lastMessage.isEmpty())
            msg += ": " + errorCollector.lastMessage;
        diagnostics.addError (0, 0, msg);
        return nullptr;
    }

#if ASMJIT_ARCH_X86
    const bool hasFusedMultiplyAdd = std::any_of (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst)
        {
            return inst.op == YdspIrOp::fmaF || inst.op == YdspIrOp::fmsubF;
        });
    });

    if (fn.vectorWidth > YdspVectorizer::vectorWidth || hasFusedMultiplyAdd)
        funcNode->frame().set_avx_cleanup();
#endif

    this->cc = &cc;
    this->isEventHandler = isEventHandler;
    activeFn = &fn;
    activeVectorWidth = fn.vectorWidth == 8 || fn.vectorWidth == 16 ? fn.vectorWidth : YdspVectorizer::vectorWidth;

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

    if (isEventHandler)
    {
        // Event-handler ABI: one voice's state slice, the node's params, and
        // the dispatching event's payload. No inputs/outputs/meters or
        // block-size context. The payload is read on demand through
        // loadEventField*, which carries its byte offset, so the context
        // pointer stays live for the whole body.
        eventCtxReg = ctxReg;

        loadGpFromMem (stateReg, memPtr (ctxReg, offsetof (YdspEventContext, state)));
        loadGpFromMem (stateArraysReg, memPtr (ctxReg, offsetof (YdspEventContext, stateArrays)));
        loadGpFromMem (paramsReg, memPtr (ctxReg, offsetof (YdspEventContext, params)));
        loadFloatFromMem (sampleRateReg, memPtr (ctxReg, offsetof (YdspEventContext, sampleRate)));
        loadGpFromMem (outputEventsReg, memPtr (ctxReg, offsetof (YdspEventContext, outputEvents)));
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
            loadGpFromMem (inputsReg, memPtr (ctxReg, offsetof (YdspKernelContext, inputs)));
        if (needsOutputs)
            loadGpFromMem (outputsReg, memPtr (ctxReg, offsetof (YdspKernelContext, outputs)));
        if (needsParams)
            loadGpFromMem (paramsReg, memPtr (ctxReg, offsetof (YdspKernelContext, params)));
        if (needsParamOut)
            loadGpFromMem (paramOutReg, memPtr (ctxReg, offsetof (YdspKernelContext, paramOut)));
        if (needsState)
            loadGpFromMem (stateReg, memPtr (ctxReg, offsetof (YdspKernelContext, state)));
        if (needsStateArrays)
            loadGpFromMem (stateArraysReg, memPtr (ctxReg, offsetof (YdspKernelContext, stateArrays)));
        if (needsBlockSize)
            loadGpFromMem (numSamplesReg, memPtr (ctxReg, offsetof (YdspKernelContext, numSamples)));
        if (needsSampleRate)
            loadFloatFromMem (sampleRateReg, memPtr (ctxReg, offsetof (YdspKernelContext, sampleRate)));
        if (needsOutputEvents)
            loadGpFromMem (outputEventsReg, memPtr (ctxReg, offsetof (YdspKernelContext, outputEvents)));

        // Hoist the per-stream channel pointers out of the sample loop: they
        // are fixed for the whole call, so re-deriving them at every access
        // costs one dependent load per stream access per sample.
        //
        // Only the streams this kernel actually touches may be dereferenced:
        // the init kernel declares the processor's endpoints but is invoked
        // with inputs/outputs set to null (runInitKernels), so hoisting a
        // stream it never reads would fault on the null channel array.
        inputBaseRegs.assign (static_cast<size_t> (fn.numInputs), YdspGp {});
        outputBaseRegs.assign (static_cast<size_t> (fn.numOutputs), YdspGp {});

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                const bool isInput = inst.op == YdspIrOp::loadInput;
                const bool isOutput = inst.op == YdspIrOp::loadOutput || inst.op == YdspIrOp::storeOutput;

                if (! isInput && ! isOutput)
                    continue;

                auto& bases = isInput ? inputBaseRegs : outputBaseRegs;
                const auto slot = static_cast<size_t> (inst.memIndex);

                if (slot >= bases.size() || bases[slot].is_valid())
                    continue;

                bases[slot] = cc.new_gp64 (isInput ? "inStream" : "outStream");
                loadGpFromMem (bases[slot], memPtr (isInput ? inputsReg : outputsReg, inst.memIndex * 8));

                // These base pointers are live across the whole sample loop;
                // give them top weight so the allocator never spills them.
                if (auto* virt = cc.virt_reg_by_reg (bases[slot]))
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

                    diagnostics.addError (0, 0, msg);
                    return nullptr;
                }
            }
        }
    }

    // Weight the registers the allocator must not spill: loop induction
    // variables (their value is live across the back edge) and every widened
    // value (a spilled 4/8-lane vector costs several loads/stores per sample).
    // asmjit's weight is an alloc/spill hint on the virtual register.
    for (const auto& loop : fn.loops)
        if (loop.induction >= 0 && static_cast<size_t> (loop.induction) < regs.size())
            if (regs[static_cast<size_t> (loop.induction)].is_valid())
                if (auto* virt = cc.virt_reg_by_reg (regs[static_cast<size_t> (loop.induction)]))
                    virt->set_weight (255);

    for (size_t i = 0; i < valueLanes.size() && i < regs.size(); ++i)
    {
        if (valueLanes[i] <= 1 || ! regs[i].is_valid())
            continue;

        if (auto* virt = cc.virt_reg_by_reg (regs[i]))
            virt->set_weight (255);
    }

    // Last chance to set up addressing that would otherwise be re-derived at
    // every access. Must come after valueTypes and the segment offsets, and
    // before any block is emitted, so the addresses dominate every use.
    prepareStateAddressing (fn);
    planCompareFusion (fn);
    materializeScalarLibmTargets (fn);

    // Emit the blocks.
    for (size_t i = 0; i < fn.blocks.size(); ++i)
    {
        cc.bind (blockLabels[i]);
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

        emitTerminator (fn.blocks[i], static_cast<int> (i));
    }

    cc.end_func();

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

        diagnostics.addError (0, 0, msg);
        return nullptr;
    }

    if (generatedCodeSize != nullptr)
        *generatedCodeSize = code.code_size();

    YdspKernelFn fnPtr = nullptr;

    auto addErr = runtime.add (&fnPtr, &code);
    if (addErr != asmjit::kErrorOk)
    {
        String msg = "AsmJit failed to allocate the kernel: ";
        msg += asmjit::stringify_error (addErr);
        diagnostics.addError (0, 0, msg);
        return nullptr;
    }

    // Always emit the generated assembly as an info diagnostic
    if (logger.data_size() > 0)
    {
        const String info (logger.data(), logger.data_size());
        diagnostics.addInfo (0, 0, info);
    }

    return fnPtr;
}

//==============================================================================

void YdspAsmJitCodegenImpl::planCompareFusion (const YdspIrFunction& fn)
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

            if (select.op != YdspIrOp::selectB || select.result < 0 || select.a < 0)
                continue;

            const auto condition = select.a;

            // The comparison must feed this select and nothing else, or eliding
            // it would drop a value something still reads.
            if (defCount[condition] != 1 || useCount[condition] != 1)
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

            if (! isFloatComparison (compare.op) && ! isIntComparison (compare.op))
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
                elidedCompares.insert (compareAt);
        }
    }

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

        if (! isFloatComparison (compare.op) && ! isIntComparison (compare.op))
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

YdspMem YdspAsmJitCodegenImpl::rawConstMem (uint64_t bits, bool is64)
{
    if (is64)
        return cc->new_const (asmjit::ConstPoolScope::kLocal, &bits, 8);

    const auto word = static_cast<uint32_t> (bits);
    return cc->new_const (asmjit::ConstPoolScope::kLocal, &word, 4);
}

YdspMem YdspAsmJitCodegenImpl::packedConstMem (uint64_t bits, bool is64)
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

YdspMem YdspAsmJitCodegenImpl::vectorConstMem (uint32_t bits)
{
    std::array<uint32_t, 16> lanes {};

    std::fill_n (lanes.begin(), static_cast<size_t> (activeVectorWidth), bits);
    return cc->new_const (asmjit::ConstPoolScope::kLocal, lanes.data(), static_cast<size_t> (activeVectorWidth) * sizeof (uint32_t));
}

void YdspAsmJitCodegenImpl::loadFloatConst (const YdspFp& dst, double value, YdspValueType type)
{
    // Read the literal straight out of the constant pool. Materializing it via
    // an integer register instead would cost two instructions plus a
    // domain-crossing move, and would tie up a GP register for as long as the
    // constant is live - which, after LICM hoists every constant into the entry
    // block, is the whole kernel.
    const bool is64 = (type == YdspValueType::float64Type);

    loadFloatFromMem (dst, rawConstMem (is64 ? doubleBits (value) : floatBits (value), is64));
}

//==============================================================================
// Memory addressing

// Emits a load or store of a state float/int slot, handling the AArch64
// indexed-address-with-offset limitation by materializing the base. The
// element type drives the scale (4 or 8 bytes) and the load/store width.
void YdspAsmJitCodegenImpl::emitStateAccess (YdspValueType type, bool isLoad, const asmjit::Reg& reg, int base, int indexValue)
{
    const bool isFloat = isFloatValueType (type);
    const YdspMem mem = emitStateMem (type, base, indexValue);

    if (isFloat)
    {
        if (isLoad)
            loadFloatFromMem (reg.as<YdspFp>(), mem);
        else
            storeFloatToMem (mem, reg.as<YdspFp>());
    }
    else
    {
        if (isLoad)
            loadGpFromMem (reg.as<YdspGp>(), mem);
        else
            storeGpToMem (mem, reg.as<YdspGp>());
    }
}

//==============================================================================
// Instruction emission

void YdspAsmJitCodegenImpl::emitInstruction (const YdspIrFunction& fn, const YdspIrInst& inst, int blockIndex, int instIndex)
{
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
            loadFloatFromMem (fp (inst.result), memPtr (eventCtxReg, inst.memIndex));
            return;
        case YdspIrOp::loadEventFieldI:
            loadGpFromMem (gp (inst.result), memPtr (eventCtxReg, inst.memIndex));
            return;
        case YdspIrOp::storeEventFieldF:
            storeFloatToMem (memPtr (outputEventsReg, inst.memIndex), fp (inst.a));
            return;
        case YdspIrOp::storeEventFieldI:
            storeGpToMem (memPtr (outputEventsReg, inst.memIndex), gp (inst.a));
            return;
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
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];

            // A widened access reads a whole vector of consecutive elements
            // starting at the same element index, so only the width changes.
            if (isVectorValue (inst.result))
            {
                loadVectorFromMem (fp (inst.result), emitVectorStateMem (stateArrayBase (type, inst.memIndex), inst.a));
                return;
            }

            emitStateAccess (type, true, fp (inst.result), stateArrayBase (type, inst.memIndex), inst.a);
            return;
        }
        case YdspIrOp::storeStateArrayF:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.b)];

            if (isVectorValue (inst.b))
            {
                storeVectorToMem (emitVectorStateMem (stateArrayBase (type, inst.memIndex), inst.a), fp (inst.b));
                return;
            }

            emitStateAccess (type, false, fp (inst.b), stateArrayBase (type, inst.memIndex), inst.a);
            return;
        }

        case YdspIrOp::loadStateArrayI:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.result)];
            emitStateAccess (type, true, gp (inst.result), stateArrayBase (type, inst.memIndex), inst.a);
            return;
        }
        case YdspIrOp::storeStateArrayI:
        {
            const auto type = valueTypes[static_cast<size_t> (inst.b)];
            emitStateAccess (type, false, gp (inst.b), stateArrayBase (type, inst.memIndex), inst.a);
            return;
        }

        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::storeOutput:
        {
            const bool isStore = (inst.op == YdspIrOp::storeOutput);
            const bool isInput = (inst.op == YdspIrOp::loadInput);

            const auto type = isStore ? valueTypes[static_cast<size_t> (inst.b)]
                                      : valueTypes[static_cast<size_t> (inst.result)];

            const auto& bases = isInput ? inputBaseRegs : outputBaseRegs;
            const auto slot = static_cast<size_t> (inst.memIndex);

            // compile() pre-loaded a base for every stream slot referenced by
            // the IR, so a miss here means the two scans disagree.
            jassert (slot < bases.size() && bases[slot].is_valid());

            if (slot >= bases.size() || ! bases[slot].is_valid())
                return;

            const YdspGp base = bases[slot];

            if (isVectorValue (isStore ? inst.b : inst.result))
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
            emitSplatFloat (fp (inst.result), fp (inst.a));
            return;

        case YdspIrOp::vreduceAddF:
            emitReduceAddFloat (fp (inst.result), fp (inst.a));
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
            if (isVectorValue (inst.result))
            {
                vectorBinary (YdspIrOp::maxF, fp (inst.result), fp (inst.a), fp (inst.b));
                vectorBinary (YdspIrOp::minF, fp (inst.result), fp (inst.result), fp (inst.c));
                return;
            }

            floatBinary (YdspIrOp::maxF, fp (inst.result), fp (inst.a), fp (inst.b));
            floatBinary (YdspIrOp::minF, fp (inst.result), fp (inst.result), fp (inst.c));
            return;

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
            intBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::negI:
            intUnaryNeg (gp (inst.result), gp (inst.a));
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
            emitIntCompareToFlags (gp (inst.a), gp (inst.b));
            emitSelectIntOnFlags (YdspCond::kSignedLT, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::maxI:
            emitIntCompareToFlags (gp (inst.a), gp (inst.b));
            emitSelectIntOnFlags (YdspCond::kSignedGT, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::clampI:
        {
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
            const auto dst = gp (inst.result);
            const auto src = gp (inst.a);
            YdspGp negated = dst.is_gp64() ? cc->new_gp64 ("absNeg") : cc->new_gp32 ("absNeg");
            YdspGp zero = dst.is_gp64() ? cc->new_gp64 ("absZero") : cc->new_gp32 ("absZero");

            intUnaryNeg (negated, src);
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
            emitIntToFloat (fp (inst.result), gp (inst.a));
            return;

        case YdspIrOp::ftoi:
            emitFloatToInt (gp (inst.result), fp (inst.a));
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

        // ---- logic ----
        case YdspIrOp::andB:
        case YdspIrOp::orB:
            intBinary (inst.op, gp (inst.result), gp (inst.a), gp (inst.b));
            return;

        case YdspIrOp::notB:
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
                emitFloatCompare (conditionForComparison (inst.op), fp (inst.a), fp (inst.b), gp (inst.result));

            return;

        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::ltI:
        case YdspIrOp::leI:
        case YdspIrOp::gtI:
        case YdspIrOp::geI:
            if (elidedCompares.count ({ blockIndex, instIndex }) == 0)
                emitIntCompare (conditionForComparison (inst.op), gp (inst.a), gp (inst.b), gp (inst.result));

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
            moveGp (gp (inst.result), gp (inst.a));
            return;

        // ---- select ----
        case YdspIrOp::selectB:
        {
            if (isVectorValue (inst.result))
            {
                vectorSelectFloat (fp (inst.a), fp (inst.result), fp (inst.b), fp (inst.c));
                return;
            }

            // Branchless on both targets: a select in a sample loop is almost
            // always data dependent, so a branch diamond here would be an
            // unpredictable branch per sample.
            const bool isFloatResult = isFloatValueType (valueTypes[static_cast<size_t> (inst.result)]);

            // Consume the comparison's flags directly where that is legal,
            // rather than reading back the 0/1 register it would produce.
            if (const auto it = fusedSelectCompares.find ({ blockIndex, instIndex }); it != fusedSelectCompares.end())
            {
                const auto& compare = it->second;

                if (isFloatComparison (compare.op))
                    emitFloatCompareToFlags (fp (compare.a), fp (compare.b));
                else
                    emitIntCompareToFlags (gp (compare.a), gp (compare.b));

                const auto cond = conditionForComparison (compare.op);

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

bool YdspAsmJitCodegenImpl::isVectorValue (int value) const noexcept
{
    return value >= 0
        && static_cast<size_t> (value) < valueLanes.size()
        && valueLanes[static_cast<size_t> (value)] > 1;
}

YdspFp YdspAsmJitCodegenImpl::newFpOfType (YdspValueType type, const char* name)
{
    return type == YdspValueType::float64Type ? newFp64 (name) : newFp (name);
}

asmjit::Reg YdspAsmJitCodegenImpl::newRegFor (const std::vector<YdspValueType>& types, const YdspIrInst& inst)
{
    const auto type = types[static_cast<size_t> (inst.result)];

    if (isVectorValue (inst.result))
        return newFpVector ("v");

    if (type == YdspValueType::float64Type)
        return newFp64 ("v");

    if (type == YdspValueType::float32Type)
        return newFp ("v");

    if (type == YdspValueType::int64Type)
        return cc->new_gp64 ("v");

    return cc->new_gp32 ("v");
}

YdspGp YdspAsmJitCodegenImpl::gp (int value) const
{
    return regs[static_cast<size_t> (value)].as<YdspGp>();
}

YdspFp YdspAsmJitCodegenImpl::fp (int value) const
{
    return regs[static_cast<size_t> (value)].as<YdspFp>();
}

void YdspAsmJitCodegenImpl::moveGp (const YdspGp& dst, const YdspGp& src)
{
    cc->mov (dst, src);
}

//==============================================================================
// transcendental calls (libm, or SLEEF scalar under fastMath)

void YdspAsmJitCodegenImpl::emitLibmUnary (float (*f32fn) (float), double (*f64fn) (double), const YdspIrInst& inst)
{
    const bool is64 = valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;

    asmjit::InvokeNode* node = nullptr;

    const auto* fnPtr = is64 ? reinterpret_cast<const void*> (f64fn) : reinterpret_cast<const void*> (f32fn);

    YdspGp target;

    if (const auto it = libmTargetRegs.find (fnPtr); it != libmTargetRegs.end())
        target = it->second;
    else
    {
        target = cc->new_gp64 ("fn");
        cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (fnPtr)));
    }

    auto err = cc->invoke (asmjit::Out (node), target, is64 ? asmjit::FuncSignature::build<double, double>() : asmjit::FuncSignature::build<float, float>());

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        node->set_arg (0, fp (inst.a));
        node->set_ret (0, fp (inst.result));
    }
}

void YdspAsmJitCodegenImpl::emitLibmBinary (float (*f32fn) (float, float), double (*f64fn) (double, double), const YdspIrInst& inst)
{
    const bool is64 = valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;

    asmjit::InvokeNode* node = nullptr;

    const auto* fnPtr = is64 ? reinterpret_cast<const void*> (f64fn) : reinterpret_cast<const void*> (f32fn);

    YdspGp target;

    if (const auto it = libmTargetRegs.find (fnPtr); it != libmTargetRegs.end())
        target = it->second;
    else
    {
        target = cc->new_gp64 ("fn");
        cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (fnPtr)));
    }

    auto err = cc->invoke (asmjit::Out (node), target, is64 ? asmjit::FuncSignature::build<double, double, double>() : asmjit::FuncSignature::build<float, float, float>());

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        node->set_arg (0, fp (inst.a));
        node->set_arg (1, fp (inst.b));
        node->set_ret (0, fp (inst.result));
    }
}

//==============================================================================
// SLEEF float32 math lowering
//
// Native builds link sleef_library, whose canonical names the module exports
// (4-lane Sleef_<name>f4_u10/u35). Only the *widened* path uses SLEEF: a
// transcendental on a widened float32 value calls the 4-lane set - u35 under
// fastMath, u10 otherwise, so a strict (fastMath-off) vectorized loop keeps
// ~1 ULP parity with libm. Scalar transcendentals never leave libm: the
// platform libm scalar is already tuned per target, and measured SLEEF scalar
// calls (exp on Apple silicon) regressed against it, so the scalar fastMath
// benefit is contraction alone (see Phase 4 for a per-function enable list).
// float64 operands never leave libm either.

namespace
{

using SleefVectorUnaryFn = Sleef_float32x4 (*) (Sleef_float32x4);
using SleefVectorBinaryFn = Sleef_float32x4 (*) (Sleef_float32x4, Sleef_float32x4);

struct SleefUnaryMathEntry
{
    YdspIrOp op;
    float (*libmF32) (float);
    double (*libmF64) (double);
    SleefVectorUnaryFn sleefVectorU10;
    SleefVectorUnaryFn sleefVectorU35;
};

const SleefUnaryMathEntry sleefUnaryMathTable[] =
{
    { YdspIrOp::sinF, &::sinf, &::sin, &Sleef_sinf4_u10, &Sleef_sinf4_u35 },
    { YdspIrOp::cosF, &::cosf, &::cos, &Sleef_cosf4_u10, &Sleef_cosf4_u35 },
    { YdspIrOp::tanF, &::tanf, &::tan, &Sleef_tanf4_u10, &Sleef_tanf4_u35 },
    { YdspIrOp::asinF, &::asinf, &::asin, &Sleef_asinf4_u10, &Sleef_asinf4_u35 },
    { YdspIrOp::acosF, &::acosf, &::acos, &Sleef_acosf4_u10, &Sleef_acosf4_u35 },
    { YdspIrOp::atanF, &::atanf, &::atan, &Sleef_atanf4_u10, &Sleef_atanf4_u35 },
    { YdspIrOp::sinhF, &::sinhf, &::sinh, &Sleef_sinhf4_u10, &Sleef_sinhf4_u35 },
    { YdspIrOp::coshF, &::coshf, &::cosh, &Sleef_coshf4_u10, &Sleef_coshf4_u35 },
    { YdspIrOp::tanhF, &::tanhf, &::tanh, &Sleef_tanhf4_u10, &Sleef_tanhf4_u35 },
    { YdspIrOp::asinhF, &::asinhf, &::asinh, &Sleef_asinhf4_u10, nullptr },
    { YdspIrOp::acoshF, &::acoshf, &::acosh, &Sleef_acoshf4_u10, nullptr },
    { YdspIrOp::atanhF, &::atanhf, &::atanh, &Sleef_atanhf4_u10, nullptr },
    { YdspIrOp::expF, &::expf, &::exp, &Sleef_expf4_u10, nullptr },
    { YdspIrOp::logF, &::logf, &::log, &Sleef_logf4_u10, &Sleef_logf4_u35 },
    { YdspIrOp::log10F, &::log10f, &::log10, &Sleef_log10f4_u10, nullptr },
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
    YdspIrOp op;
    float (*libmF32) (float, float);
    double (*libmF64) (double, double);
    SleefVectorBinaryFn sleefVectorU10;
    SleefVectorBinaryFn sleefVectorU35;
};

const SleefBinaryMathEntry sleefBinaryMathTable[] =
{
    { YdspIrOp::powF, &::powf, &::pow, &Sleef_powf4_u10, nullptr },
    { YdspIrOp::atan2F, &::atan2f, &::atan2, &Sleef_atan2f4_u10, &Sleef_atan2f4_u35 },
    { YdspIrOp::fmodF, &::fmodf, &::fmod, &Sleef_fmodf4, nullptr },
};

const SleefBinaryMathEntry* sleefBinaryEntry (YdspIrOp op) noexcept
{
    for (const auto& entry : sleefBinaryMathTable)
        if (entry.op == op)
            return &entry;

    return nullptr;
}

} // namespace

void YdspAsmJitCodegenImpl::materializeScalarLibmTargets (const YdspIrFunction& fn)
{
    std::unordered_set<void*> targets;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result < 0 || isVectorValue (inst.result))
                continue;

            const auto is64 = inst.a >= 0 && valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float64Type;

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
                        fnPtr = is64 ? reinterpret_cast<void*> (entry->libmF64) : reinterpret_cast<void*> (entry->libmF32);
                    break;
                }

                case YdspIrOp::powF:
                case YdspIrOp::atan2F:
                case YdspIrOp::fmodF:
                {
                    if (const auto* entry = sleefBinaryEntry (inst.op))
                        fnPtr = is64 ? reinterpret_cast<void*> (entry->libmF64) : reinterpret_cast<void*> (entry->libmF32);
                    break;
                }

                case YdspIrOp::roundF:
                    fnPtr = is64 ? reinterpret_cast<void*> (static_cast<double (*) (double)> (&::round))
                                 : reinterpret_cast<void*> (&::roundf);
                    break;

                case YdspIrOp::copysignF:
                    fnPtr = is64 ? reinterpret_cast<void*> (static_cast<double (*) (double, double)> (&::copysign))
                                 : reinterpret_cast<void*> (&::copysignf);
                    break;

                default:
                    continue;
            }

            if (fnPtr != nullptr)
                targets.insert (fnPtr);
        }
    }

    for (auto* fnPtr : targets)
    {
        auto reg = cc->new_gp64 ("libmFn");
        cc->mov (reg, asmjit::Imm (ydspFnPtrToInt64 (fnPtr)));
        libmTargetRegs[fnPtr] = reg;
    }
}

void YdspAsmJitCodegenImpl::emitVectorMathInvoke (void* fnPtr, bool isBinary, const YdspFp& ret, const YdspFp& a, const YdspFp& b)
{
    asmjit::InvokeNode* node = nullptr;

    // Materialize the target in a GP register, as the scalar libm calls do.
    YdspGp target = cc->new_gp64 ("fn");
    cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (fnPtr)));

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

    const auto err = cc->invoke (asmjit::Out (node), target, sig);

    if (err == asmjit::kErrorOk && node != nullptr)
    {
        node->set_arg (0, a);

        if (isBinary)
            node->set_arg (1, b);

        node->set_ret (0, ret);
    }
}

void YdspAsmJitCodegenImpl::emitInlineFastExp (const YdspIrInst& inst)
{
#if ASMJIT_ARCH_ARM == 64
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
#else
    emitLibmUnary (&::expf, &::exp, inst);
#endif
}

void YdspAsmJitCodegenImpl::emitTranscendentalCall (const YdspIrFunction& fn, const YdspIrInst& inst, bool isBinary)
{
    if (! isVectorValue (inst.result))
    {
#if ASMJIT_ARCH_ARM == 64
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
#endif

        // Scalar path: always libm. The platform libm scalar is tuned per
        // target and measured SLEEF scalar calls regressed against it (exp on
        // Apple silicon), so fastMath's scalar benefit is contraction only.
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
    void* vectorFn = nullptr;

    if (isBinary)
    {
        if (const auto* entry = sleefBinaryEntry (inst.op))
            vectorFn = reinterpret_cast<void*> (fn.fastMath && entry->sleefVectorU35 != nullptr ? entry->sleefVectorU35 : entry->sleefVectorU10);
    }
    else
    {
        if (const auto* entry = sleefUnaryEntry (inst.op))
            vectorFn = reinterpret_cast<void*> (fn.fastMath && entry->sleefVectorU35 != nullptr ? entry->sleefVectorU35 : entry->sleefVectorU10);
    }

    if (vectorFn == nullptr)
        return;

    const auto a = fp (inst.a);
    const auto b = isBinary ? fp (inst.b) : YdspFp();

    if (activeVectorWidth == 4)
    {
        emitVectorMathInvoke (vectorFn, isBinary, fp (inst.result), a, b);
        return;
    }

#if ASMJIT_ARCH_X86
    if (activeVectorWidth == 8)
    {
        // Split the 8-lane value into two 4-lane calls. Everything chunk-sized
        // is a fresh 128-bit register, so the register allocator can never put
        // a chunk on a ymm whose xmm half does not exist.
        YdspFp loA = newFp128 ("sleefLoA");
        YdspFp hiA = newFp128 ("sleefHiA");
        cc->vextractf128 (loA, a, asmjit::Imm (0));
        cc->vextractf128 (hiA, a, asmjit::Imm (1));

        YdspFp loB;
        YdspFp hiB;

        if (isBinary)
        {
            loB = newFp128 ("sleefLoB");
            hiB = newFp128 ("sleefHiB");
            cc->vextractf128 (loB, b, asmjit::Imm (0));
            cc->vextractf128 (hiB, b, asmjit::Imm (1));
        }

        YdspFp loResult = newFp128 ("sleefLoRes");
        YdspFp hiResult = newFp128 ("sleefHiRes");
        emitVectorMathInvoke (vectorFn, isBinary, loResult, loA, loB);
        emitVectorMathInvoke (vectorFn, isBinary, hiResult, hiA, hiB);

        YdspFp dst = newFpVector ("sleefRes");
        cc->vxorps (dst, dst, dst);
        cc->vinsertf128 (dst, dst, loResult, asmjit::Imm (0));
        cc->vinsertf128 (dst, dst, hiResult, asmjit::Imm (1));
        moveVector (fp (inst.result), dst);
        return;
    }
#endif

    jassertfalse; // an unsupported active vector width reached a transcendental
}

void YdspAsmJitCodegenImpl::emitEmitEvent (const YdspIrInst& inst)
{
    // AArch64's BLR only takes a register operand, so the target address
    // must be materialized rather than passed as an immediate.
    YdspGp target = cc->new_gp64 ("fn");
    cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (reinterpret_cast<void*> (&ydspCommitOutputEvent))));

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

void YdspAsmJitCodegenImpl::emitTerminator (const YdspIrBlock& block, int blockIndex)
{
    // Blocks are emitted in index order, so a jump to blockIndex + 1 targets
    // the label bound on the very next line: skip it and let the code fall
    // through (asmjit's register allocator recovers the edge from the bound
    // label). The sample-loop header takes this path on every iteration.
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

            if (const auto it = fusedBranchCompares.find (blockIndex); it != fusedBranchCompares.end())
            {
                const auto& compare = it->second;

                if (isFloatComparison (compare.op))
                    emitFloatCompareToFlags (fp (compare.a), fp (compare.b));
                else
                    emitIntCompareToFlags (gp (compare.a), gp (compare.b));

                const auto cond = conditionForComparison (compare.op);

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

int YdspAsmJitCodegenImpl::stateScalarBase (YdspValueType type, int slot) const
{
    return activeFn->stateScalarByteOffset (type, slot);
}

int YdspAsmJitCodegenImpl::stateArrayBase (YdspValueType type, int element) const
{
    return activeFn->stateArrayByteOffset (type, element);
}

//==============================================================================

YdspKernelFn YdspAsmJitCodegen::compile (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize)
{
#if ASMJIT_ARCH_X86
    YdspAsmJitCodegenX64 impl;
#elif ASMJIT_ARCH_ARM
    YdspAsmJitCodegenARM64 impl;
#else
#error "yup_dsp_jit requires an x86-64 or AArch64 backend"
#endif

    return impl.compile (runtime, fn, diagnostics, false, generatedCodeSize);
}

YdspEventHandlerFn YdspAsmJitCodegen::compileEventHandler (asmjit::JitRuntime& runtime, const YdspIrFunction& fn, YdspDiagnostics& diagnostics, size_t* generatedCodeSize)
{
#if ASMJIT_ARCH_X86
    YdspAsmJitCodegenX64 impl;
#elif ASMJIT_ARCH_ARM
    YdspAsmJitCodegenARM64 impl;
#else
#error "yup_dsp_jit requires an x86-64 or AArch64 backend"
#endif

    // Both ABIs are `void (*) (void*)` at the machine level; the impl returns
    // the kernel-typed pointer and the caller casts to the handler ABI.
    return ydspFnPtrCast<YdspEventHandlerFn> (impl.compile (runtime, fn, diagnostics, true, generatedCodeSize));
}

} // namespace yup
