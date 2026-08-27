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

namespace detail
{

// Flattens a struct-typed state into the existing scalar/array slots:
// scalar fields of a scalar instance become scalar slots; every other
// combination becomes an array region (elements * instances).
void layoutStructState (const YdspStateDecl* state, const YdspStructDecl* structDecl, YdspStateLayout& layout)
{
    const bool isArrayInstance = state->arraySize > 0;

    for (size_t f = 0; f < structDecl->fields.size(); ++f)
    {
        const auto& field = structDecl->fields[f];
        const auto type = toStorageType (field.type);
        const auto key = std::make_pair (state, static_cast<int> (f));

        if (field.arraySize > 0)
        {
            // Array field: `fieldSize * instances` elements.
            const int instances = isArrayInstance ? state->arraySize : 1;
            const int total = instances * field.arraySize;

            switch (type)
            {
                case YdspValueType::float32Type:
                    layout.structFieldBases[key] = layout.float32ArrayCursor;
                    layout.float32ArrayCursor += total;
                    break;
                case YdspValueType::float64Type:
                    layout.structFieldBases[key] = layout.float64ArrayCursor;
                    layout.float64ArrayCursor += total;
                    break;
                case YdspValueType::int32Type:
                    layout.structFieldBases[key] = layout.int32ArrayCursor;
                    layout.int32ArrayCursor += total;
                    break;
                case YdspValueType::int64Type:
                    layout.structFieldBases[key] = layout.int64ArrayCursor;
                    layout.int64ArrayCursor += total;
                    break;
                default:
                    break;
            }

            layout.structFieldStrides[key] = field.arraySize;
        }
        else if (isArrayInstance)
        {
            // Scalar field of an array instance: `instances` elements.
            switch (type)
            {
                case YdspValueType::float32Type:
                    layout.structFieldBases[key] = layout.float32ArrayCursor;
                    layout.float32ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::float64Type:
                    layout.structFieldBases[key] = layout.float64ArrayCursor;
                    layout.float64ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::int32Type:
                    layout.structFieldBases[key] = layout.int32ArrayCursor;
                    layout.int32ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::int64Type:
                    layout.structFieldBases[key] = layout.int64ArrayCursor;
                    layout.int64ArrayCursor += state->arraySize;
                    break;
                default:
                    break;
            }

            layout.structFieldStrides[key] = 1;
        }
        else
        {
            // Scalar field of a scalar instance: a single scalar slot.
            switch (type)
            {
                case YdspValueType::float32Type:
                    layout.structFieldBases[key] = layout.float32ScalarCount++;
                    break;
                case YdspValueType::float64Type:
                    layout.structFieldBases[key] = layout.float64ScalarCount++;
                    break;
                case YdspValueType::int32Type:
                    layout.structFieldBases[key] = layout.int32ScalarCount++;
                    break;
                case YdspValueType::int64Type:
                    layout.structFieldBases[key] = layout.int64ScalarCount++;
                    break;
                default:
                    break;
            }

            layout.structFieldStrides[key] = 0;
        }
    }
}

//==============================================================================

// Computes the shared state layout for a processor. This covers only the
// *declared* state: hidden slots allocated by ' / @ inside the kernel's
// sample loop grow the counts *after* this base (event handlers never
// allocate hidden slots, so they only ever read this shared layout).
YdspStateLayout computeStateLayout (const YdspAnalyzedProcessor& processor)
{
    YdspStateLayout layout;

    std::unordered_map<String, const YdspStructDecl*> structDecls;

    for (const auto& structDecl : processor.decl->structs)
        structDecls[structDecl.name] = &structDecl;

    for (const auto* state : processor.states)
    {
        if (! state->structName.isEmpty())
        {
            const auto it = structDecls.find (state->structName);

            if (it != structDecls.end())
                layoutStructState (state, it->second, layout);

            continue;
        }

        const auto type = toStorageType (state->type);

        if (state->arraySize > 0)
        {
            switch (type)
            {
                case YdspValueType::float32Type:
                    layout.float32ArrayBases[state] = layout.float32ArrayCursor;
                    layout.float32ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::float64Type:
                    layout.float64ArrayBases[state] = layout.float64ArrayCursor;
                    layout.float64ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::int32Type:
                    layout.int32ArrayBases[state] = layout.int32ArrayCursor;
                    layout.int32ArrayCursor += state->arraySize;
                    break;
                case YdspValueType::int64Type:
                    layout.int64ArrayBases[state] = layout.int64ArrayCursor;
                    layout.int64ArrayCursor += state->arraySize;
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch (type)
            {
                case YdspValueType::float32Type:
                    layout.float32ScalarSlots[state] = layout.float32ScalarCount++;
                    break;
                case YdspValueType::float64Type:
                    layout.float64ScalarSlots[state] = layout.float64ScalarCount++;
                    break;
                case YdspValueType::int32Type:
                    layout.int32ScalarSlots[state] = layout.int32ScalarCount++;
                    break;
                case YdspValueType::int64Type:
                    layout.int64ScalarSlots[state] = layout.int64ScalarCount++;
                    break;
                default:
                    break;
            }
        }
    }

    return layout;
}

} // namespace detail

//==============================================================================

YdspIrBuilder::YdspIrBuilder (YdspIrFunction& fn, const YdspAnalyzedProcessor& processor, DspJitDiagnostics& diagnostics, const detail::YdspStateLayout& layout, Config config)
    : fn (fn)
    , processor (processor)
    , diagnostics (diagnostics)
    , layout (layout)
    , isInit (config.isInit)
    , isEventHandler (config.isEventHandler)
    , eventHandler (config.eventHandler)
    , sharedLayout (config.sharedLayout)
    , programFunctions (config.programFunctions)
{
    // Index the processor's struct declarations for field resolution.
    for (const auto& structDecl : processor.decl->structs)
        structDecls[structDecl.name] = &structDecl;
}

//==============================================================================

void YdspIrBuilder::build()
{
    fn.isSampleMode = (processor.mode == YdspProcessMode::sample) && ! isInit && ! isEventHandler;
    fn.isInit = isInit;
    fn.isEventHandler = isEventHandler;

    if (isEventHandler && eventHandler != nullptr)
    {
        fn.eventShape = eventHandler->shape;
        fn.eventInputName = eventHandler->decl->endpointName;
        fn.ownerProcessorName = processor.decl->name;
    }

    fn.numParams = static_cast<int> (processor.inputValues.size());
    fn.numParamsOut = static_cast<int> (processor.outputValues.size());
    fn.numInputs = static_cast<int> (processor.inputStreams.size());
    fn.numOutputs = static_cast<int> (processor.outputStreams.size());

    // Record per-endpoint element types (drives codegen addressing and the
    // host runtime buffer sizing).
    for (const auto* endpoint : processor.inputStreams)
        fn.inputTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.outputStreams)
        fn.outputTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.inputValues)
        fn.paramTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.outputValues)
        fn.paramOutTypes.push_back (toStorageType (endpoint->type));

    currentBlock = newBlock(); // prologue

    // Parameters are sampled once per call: load them into registers up front.
    for (int i = 0; i < fn.numParams; ++i)
        paramValues.push_back (emitInst ({ YdspIrOp::loadParam, newValue (fn.paramTypes[static_cast<size_t> (i)]), i }));

    // Event handlers have no sample loop and no block-size context, so the
    // `blockSize` builtin is unavailable inside them.
    if (! isEventHandler)
        blockSizeValue = emitInst ({ YdspIrOp::loadBlockSize, newValue (YdspValueType::int32Type) });

    if (fn.isSampleMode)
    {
        buildSampleLoop();
    }
    else if (isInit)
    {
        lowerStatements (processor.decl->init->body);
    }
    else if (isEventHandler)
    {
        if (eventHandler != nullptr)
            lowerStatements (eventHandler->decl->body);
    }
    else
    {
        lowerStatements (processor.decl->process->body);

        // Defense in depth: flush any deferred ' writes (sema rejects
        // delay primitives in block mode, so this is normally empty).
        for (const auto& [slot, reg, captureExpr] : pendingPrevStores)
        {
            const auto resolved = coerceTo (captureValue (*captureExpr), YdspValueType::float32Type);
            writeHiddenSlot (slot, reg, resolved, YdspValueType::float32Type);
        }
    }

    // The kernel's final counts include the hidden ' / @ slots it allocated
    // past the shared base; handlers never allocate hidden slots.
    fn.float32Scalars = layout.float32ScalarCount + hiddenFloat32Scalars;
    fn.float64Scalars = layout.float64ScalarCount;
    fn.int32Scalars = layout.int32ScalarCount + hiddenInt32Scalars;
    fn.int64Scalars = layout.int64ScalarCount;
    fn.float32ArrayElements = layout.float32ArrayCursor + hiddenFloat32ArrayElements;
    fn.float64ArrayElements = layout.float64ArrayCursor;
    fn.int32ArrayElements = layout.int32ArrayCursor;
    fn.int64ArrayElements = layout.int64ArrayCursor;

    // The init and event-handler functions allocate no hidden state, but
    // must address the same memory as the main kernel - adopt its final
    // layout wholesale.
    if (sharedLayout != nullptr)
    {
        fn.float32Scalars = sharedLayout->float32Scalars;
        fn.float64Scalars = sharedLayout->float64Scalars;
        fn.int32Scalars = sharedLayout->int32Scalars;
        fn.int64Scalars = sharedLayout->int64Scalars;
        fn.float32ArrayElements = sharedLayout->float32ArrayElements;
        fn.float64ArrayElements = sharedLayout->float64ArrayElements;
        fn.int32ArrayElements = sharedLayout->int32ArrayElements;
        fn.int64ArrayElements = sharedLayout->int64ArrayElements;
    }

    fn.valueTypes = valueTypes;
}

//==============================================================================

void YdspIrBuilder::buildSampleLoop()
{
    // Prologue -> loop header -> body start -> ... -> body end -> epilogue.
    // The epilogue block is created after the body so the CFG stays linear
    // in block order (the codegen falls through blockIndex + 1).
    //
    // Loop-invariant initialization lives in the prologue (current block) so
    // the back-edge to the header only re-checks the termination condition.

    sampleIndex = emitConstI (0);
    const auto one = emitConstI (1);

    // Load scalar state into registers once per call, not once per sample.
    //
    // This is legal because a kernel call owns its state exclusively: the
    // runtime dispatches event handlers and automation strictly *between*
    // calls, splitting the block at each offset and re-entering the kernel
    // (see processNodeWithSplits in yup_YdspGraphPimpl.cpp). Should the loop
    // body run zero times - a rate-divided node whose block is shorter than
    // its divider - the epilogue stores back exactly the bits the prologue
    // loaded, so the round trip is a no-op rather than a corruption.
    for (const auto* state : processor.states)
    {
        if (state->arraySize > 0 || ! state->structName.isEmpty())
            continue;

        const auto type = toStorageType (state->type);

        stateRegs[state] = emitInst ({ stateIsInt (state) ? YdspIrOp::loadStateI : YdspIrOp::loadStateF,
                                       newValue (type),
                                       stateScalarSlot (state) });
    }

    const int header = newBlock();
    currentBlock = header;

    const auto cond = emitInst ({ YdspIrOp::ltI, newValue (YdspValueType::boolType), sampleIndex, blockSizeValue });

    const int bodyStart = newBlock();

    setTerminator (YdspIrTerm::branchIf, cond, bodyStart, -1); // epilogue target fixed up below
    currentBlock = bodyStart;

    lowerStatements (processor.decl->process->body);

    // Deferred writes of the ' delay primitive. Capture resolution is deferred
    // here so that lastOutputValue (populated by output-stream assignments) can
    // be used. The hidden slot lives in a register for the duration of the
    // loop, so this updates the register - the epilogue writes it to memory.
    for (const auto& [slot, reg, captureExpr] : pendingPrevStores)
    {
        const auto resolved = coerceTo (captureValue (*captureExpr), YdspValueType::float32Type);
        writeHiddenSlot (slot, reg, resolved, YdspValueType::float32Type);
    }

    const auto next = emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), sampleIndex, one });
    emitInst ({ YdspIrOp::movI, sampleIndex, next });

    setTerminator (YdspIrTerm::branch, -1, header);

    const int epilogue = newBlock();
    fn.blocks[static_cast<size_t> (header)].termTarget2 = epilogue;

    currentBlock = epilogue;

    // Write every promoted register back to state memory, once per call.
    for (const auto* state : processor.states)
    {
        if (state->arraySize > 0 || ! state->structName.isEmpty())
            continue;

        emitInst ({ stateIsInt (state) ? YdspIrOp::storeStateI : YdspIrOp::storeStateF,
                    -1,
                    stateRegs[state],
                    -1,
                    -1,
                    stateScalarSlot (state) });
    }

    for (const auto& promoted : promotedHiddenSlots)
    {
        emitInst ({ promoted.isInt ? YdspIrOp::storeStateI : YdspIrOp::storeStateF,
                    -1,
                    promoted.reg,
                    -1,
                    -1,
                    promoted.slot });
    }

    YdspIrLoop loop;
    loop.id = static_cast<int> (fn.loops.size());
    loop.headerBlock = header;
    loop.exitBlock = epilogue;
    loop.induction = sampleIndex;
    loop.bound = { YdspLoopBoundKind::blockSize, 0 };
    fn.loops.push_back (loop);
}

//==============================================================================

int YdspIrBuilder::newValue (YdspValueType type)
{
    valueTypes.push_back (type);
    return static_cast<int> (valueTypes.size()) - 1;
}

//==============================================================================

int YdspIrBuilder::newBlock()
{
    fn.blocks.emplace_back();
    return static_cast<int> (fn.blocks.size()) - 1;
}

//==============================================================================

int YdspIrBuilder::emitInst (YdspIrInst inst)
{
    fn.blocks[static_cast<size_t> (currentBlock)].insts.push_back (inst);
    return inst.result;
}

//==============================================================================

int YdspIrBuilder::emitInstInPrologue (YdspIrInst inst)
{
    fn.blocks[0].insts.push_back (inst);
    return inst.result;
}

//==============================================================================

int YdspIrBuilder::promoteHiddenSlot (int slot, YdspValueType type)
{
    const auto loadOp = isFloatValueType (type) ? YdspIrOp::loadStateF : YdspIrOp::loadStateI;

    if (! fn.isSampleMode)
        return emitInst ({ loadOp, newValue (type), slot });

    const auto reg = emitInstInPrologue ({ loadOp, newValue (type), slot });

    promotedHiddenSlots.push_back ({ slot, reg, ! isFloatValueType (type) });

    return reg;
}

//==============================================================================

void YdspIrBuilder::writeHiddenSlot (int slot, int reg, int value, YdspValueType type)
{
    if (fn.isSampleMode)
    {
        // The register carries the slot across the back edge; buildSampleLoop's
        // epilogue is what writes it to memory, once per call.
        emitInst ({ movOpFor (type), reg, value });
        return;
    }

    emitInst ({ isFloatValueType (type) ? YdspIrOp::storeStateF : YdspIrOp::storeStateI, -1, value, -1, -1, slot });
}

//==============================================================================

void YdspIrBuilder::setTerminator (YdspIrTerm term, int cond, int target, int target2)
{
    auto& block = fn.blocks[static_cast<size_t> (currentBlock)];
    block.term = term;
    block.termCond = cond;
    block.termTarget = target;
    block.termTarget2 = target2;
}

//==============================================================================

int YdspIrBuilder::emitConstFFor (double value, YdspValueType type)
{
    const auto result = emitInst ({ YdspIrOp::constF, newValue (type), -1, -1, -1, -1, value });
    floatConstPayloads[result] = value;
    return result;
}

//==============================================================================

int YdspIrBuilder::emitConstF (double value) { return emitConstFFor (value, YdspValueType::float32Type); }

//==============================================================================

int YdspIrBuilder::emitConstF64 (double value) { return emitConstFFor (value, YdspValueType::float64Type); }

//==============================================================================

int YdspIrBuilder::emitConstIFor (int64_t value, YdspValueType type)
{
    const auto result = emitInst ({ YdspIrOp::constI, newValue (type), -1, -1, -1, -1, 0.0, value });
    intConstPayloads[result] = value;
    return result;
}

//==============================================================================

int YdspIrBuilder::emitConstI (int64_t value) { return emitConstIFor (value, YdspValueType::int32Type); }

//==============================================================================

int YdspIrBuilder::emitConstI64 (int64_t value) { return emitConstIFor (value, YdspValueType::int64Type); }

//==============================================================================

int YdspIrBuilder::emitConstB (bool value) { return emitInst ({ YdspIrOp::constB, newValue (YdspValueType::boolType), -1, -1, -1, -1, 0.0, 0, value }); }

} // namespace yup
