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

YdspStateLayout computeStateLayout (const YdspAnalyzedProcessor& processor, YdspDiagnostics& diagnostics)
{
    YdspStateLayout layout;

    std::unordered_map<String, const YdspStructDecl*> structDecls;

    for (const auto& structDecl : processor.decl->structs)
        structDecls[structDecl.name] = &structDecl;

    uint64_t elements = 0;
    // Leaves room for the padding between the array segment's typed regions.
    constexpr auto maxElements = static_cast<uint64_t> (std::numeric_limits<int32_t>::max()) / 8
                               - 4 * static_cast<uint64_t> (YdspIrFunction::stateAlignmentBytes);
    for (const auto* state : processor.states)
    {
        const auto instances = static_cast<uint64_t> (jmax (1, state->arraySize));
        uint64_t fields = 1;
        if (const auto it = structDecls.find (state->structName); it != structDecls.end())
        {
            fields = 0;
            for (const auto& field : it->second->fields)
            {
                const auto count = static_cast<uint64_t> (jmax (1, field.arraySize));
                if (count > maxElements - fields)
                {
                    diagnostics.addError (state->location, "State layout exceeds the supported address range");
                    return {};
                }
                fields += count;
            }
        }
        if (fields > 0 && instances > (maxElements - elements) / fields)
        {
            diagnostics.addError (state->location, "State layout exceeds the supported address range");
            return {};
        }
        elements += instances * fields;
    }

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

//==============================================================================

namespace
{

std::optional<double> literalFloatValueOf (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::intLiteral)
        return static_cast<double> (expr.integer);

    if (expr.kind == YdspExprKind::floatLiteral)
        return expr.number;

    if (expr.kind == YdspExprKind::unary && expr.op == YdspOperator::neg && expr.children.size() == 1)
        if (const auto inner = literalFloatValueOf (*expr.children[0]))
            return -*inner;

    return std::nullopt;
}

std::optional<int64_t> literalIntValueOf (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::intLiteral)
        return expr.integer;

    if (expr.kind == YdspExprKind::unary && expr.op == YdspOperator::neg && expr.children.size() == 1)
        if (const auto inner = literalIntValueOf (*expr.children[0]))
            return *inner == std::numeric_limits<int64_t>::min() ? std::numeric_limits<int64_t>::min() : -*inner;

    return std::nullopt;
}

bool assignsElementOf (const YdspStmt& stmt, const String& array)
{
    return stmt.kind == YdspStmtKind::assign && stmt.target != nullptr
        && stmt.target->kind == YdspExprKind::index && stmt.target->children.size() == 2
        && stmt.target->children[0]->kind == YdspExprKind::identifier && stmt.target->children[0]->text == array;
}

bool writesArray (const YdspStmt& stmt, const String& array)
{
    if (assignsElementOf (stmt, array))
        return true;

    bool found = false;

    ydspForEachSubStmt (stmt, [&] (const YdspStmt& child)
    {
        if (! found && writesArray (child, array))
            found = true;
    });

    return found;
}

bool writesArray (const std::vector<YdspStmtPtr>& body, const String& array)
{
    return std::any_of (body.begin(), body.end(), [&array] (const YdspStmtPtr& stmt) { return stmt != nullptr && writesArray (*stmt, array); });
}

bool bodyWritesArrayThroughCalls (const std::vector<YdspStmtPtr>& body,
                                  const String& array,
                                  const std::vector<YdspFuncDecl>& functions,
                                  std::unordered_set<String>& visited);

bool exprCallsWritingFunction (const YdspExpr& expr,
                                 const String& array,
                                 const std::vector<YdspFuncDecl>& functions,
                                 std::unordered_set<String>& visited)
{
    if (expr.kind == YdspExprKind::call)
    {
        for (const auto& fnDecl : functions)
        {
            if (fnDecl.name == expr.text && visited.insert (fnDecl.name).second)
            {
                if (bodyWritesArrayThroughCalls (fnDecl.body, array, functions, visited))
                    return true;
            }
        }
    }

    bool found = false;

    ydspForEachSubExpr (expr, [&] (const YdspExpr& child)
    {
        if (! found && exprCallsWritingFunction (child, array, functions, visited))
            found = true;
    });

    return found;
}

bool stmtWritesArrayThroughCalls (const YdspStmt& stmt,
                                  const String& array,
                                  const std::vector<YdspFuncDecl>& functions,
                                  std::unordered_set<String>& visited)
{
    if (assignsElementOf (stmt, array))
        return true;

    bool found = false;

    ydspForEachSubExpr (stmt, [&] (const YdspExpr& expr)
    {
        if (! found && exprCallsWritingFunction (expr, array, functions, visited))
            found = true;
    });

    if (found)
        return true;

    ydspForEachSubStmt (stmt, [&] (const YdspStmt& child)
    {
        if (! found && stmtWritesArrayThroughCalls (child, array, functions, visited))
            found = true;
    });

    return found;
}

bool bodyWritesArrayThroughCalls (const std::vector<YdspStmtPtr>& body,
                                  const String& array,
                                  const std::vector<YdspFuncDecl>& functions,
                                  std::unordered_set<String>& visited)
{
    return std::any_of (body.begin(), body.end(), [&] (const YdspStmtPtr& stmt)
    {
        return stmt != nullptr && stmtWritesArrayThroughCalls (*stmt, array, functions, visited);
    });
}

/** Populates compile-time constant elements of a state array when only its
    initialiser list writes it (the semantic analyzer has lowered that list
    into literal assignments at the top of `init`), and nothing in `process`,
    processor functions, or event handlers assigns to it. */
void populateConstantElements (const YdspAnalyzedProcessor& processor,
                               const YdspStateDecl& state,
                               YdspIrFunction::StateArray& out)
{
    const auto& decl = *processor.decl;

    if ((decl.process != nullptr && writesArray (decl.process->body, state.name))
        || std::any_of (decl.functions.begin(), decl.functions.end(), [&state] (const auto& function) { return writesArray (function.body, state.name); })
        || std::any_of (decl.eventHandlers.begin(), decl.eventHandlers.end(), [&state] (const auto& handler) { return writesArray (handler.body, state.name); }))
        return;

    const bool isFloat = isFloatValueType (out.type);
    std::vector<double> floatElements;
    std::vector<int64_t> intElements;

    if (isFloat)
        floatElements.assign (static_cast<size_t> (state.arraySize), 0.0);
    else
        intElements.assign (static_cast<size_t> (state.arraySize), 0);

    if (decl.init != nullptr)
    {
        for (const auto& stmt : decl.init->body)
        {
            if (stmt == nullptr)
                continue;

            if (! assignsElementOf (*stmt, state.name))
            {
                if (writesArray (*stmt, state.name))
                    return;

                continue;
            }

            const auto& index = *stmt->target->children[1];

            if (index.kind != YdspExprKind::intLiteral || index.integer < 0 || index.integer >= state.arraySize
                || stmt->value == nullptr)
                return;

            if (isFloat)
            {
                const auto value = literalFloatValueOf (*stmt->value);

                if (! value.has_value())
                    return;

                floatElements[static_cast<size_t> (index.integer)] = out.type == YdspValueType::float32Type
                                                                       ? static_cast<double> (static_cast<float> (*value))
                                                                       : *value;
            }
            else
            {
                // An integer table takes integer literals only: what a fraction
                // is converted to is the lowering's business, not a guess here.
                const auto value = literalIntValueOf (*stmt->value);

                if (! value.has_value())
                    return;

                intElements[static_cast<size_t> (index.integer)] = out.type == YdspValueType::int32Type
                                                                     ? static_cast<int64_t> (static_cast<int32_t> (*value))
                                                                     : *value;
            }
        }
    }

    out.compileTimeConstant = true;
    out.constFloatValues = std::move (floatElements);
    out.constIntValues = std::move (intElements);
}

} // namespace

} // namespace detail

//==============================================================================

YdspIrBuilder::YdspIrBuilder (YdspIrFunction& fn, const YdspAnalyzedProcessor& processor, YdspDiagnostics& diagnostics, const detail::YdspStateLayout& layout, Config config)
    : fn (fn)
    , processor (processor)
    , diagnostics (diagnostics)
    , layout (layout)
    , provenArrayAccesses (config.provenArrayAccesses)
    , boundedFloatCasts (config.boundedFloatCasts)
    , boundedIntegerArithmetic (config.boundedIntegerArithmetic)
    , traceSites (config.traceSites)
    , isInit (config.isInit)
    , isEventHandler (config.isEventHandler)
    , eventHandler (config.eventHandler)
    , sharedLayout (config.sharedLayout)
    , programFunctions (config.programFunctions)
{
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

    for (const auto* endpoint : processor.inputStreams)
        fn.inputTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.outputStreams)
        fn.outputTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.inputValues)
        fn.paramTypes.push_back (toStorageType (endpoint->type));
    for (const auto* endpoint : processor.outputValues)
        fn.paramOutTypes.push_back (toStorageType (endpoint->type));

    currentBlock = newBlock();

    for (int i = 0; i < fn.numParams; ++i)
        paramValues.push_back (emitInst ({ YdspIrOp::loadParam, newValue (fn.paramTypes[static_cast<size_t> (i)]), i }));

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
        // A block process reads and writes its scalar state inside its own
        // loops: like the sample loop, it works on registers and writes the
        // state back once, at the end.
        loadScalarStateRegisters();
        lowerStatements (processor.decl->process->body);

        for (const auto& [slot, reg, captureExpr] : pendingPrevStores)
        {
            const auto resolved = coerceTo (captureValue (*captureExpr), YdspValueType::float32Type);
            writeHiddenSlot (slot, reg, resolved, YdspValueType::float32Type);
        }

        storeScalarStateRegisters();
    }

    fn.float32Scalars = layout.float32ScalarCount + hiddenFloat32Scalars;
    fn.float64Scalars = layout.float64ScalarCount;
    fn.int32Scalars = layout.int32ScalarCount + hiddenInt32Scalars;
    fn.int64Scalars = layout.int64ScalarCount;
    fn.float32ArrayElements = layout.float32ArrayCursor + hiddenFloat32ArrayElements;
    fn.float64ArrayElements = layout.float64ArrayCursor;
    fn.int32ArrayElements = layout.int32ArrayCursor;
    fn.int64ArrayElements = layout.int64ArrayCursor;

    // The init kernel is what fills a table: there the elements are still
    // ordinary writes and reads.
    for (const auto* state : processor.states)
    {
        if (state->arraySize <= 0 || ! state->structName.isEmpty())
            continue;

        const auto type = toStorageType (state->type);
        const int base = stateArrayBase (state);
        const bool isFloat = isFloatValueType (type);
        const bool is64 = is64BitValueType (type);

        bool readOnlyInProcess = ! isInit && ! isEventHandler;

        if (readOnlyInProcess)
        {
            for (const auto& block : fn.blocks)
            {
                for (const auto& inst : block.insts)
                {
                    if ((isFloat && inst.op != YdspIrOp::storeStateArrayF)
                        || (! isFloat && inst.op != YdspIrOp::storeStateArrayI))
                        continue;

                    if (inst.memIndex != base)
                        continue;

                    if (inst.b >= 0 && static_cast<size_t> (inst.b) < valueTypes.size()
                        && is64BitValueType (valueTypes[static_cast<size_t> (inst.b)]) != is64)
                        continue;

                    readOnlyInProcess = false;
                    break;
                }

                if (! readOnlyInProcess)
                    break;
            }
        }

        if (readOnlyInProcess)
        {
            std::unordered_set<String> visitedFuncs;

            for (const auto& handler : processor.eventHandlers)
            {
                if (handler.decl != nullptr
                    && detail::bodyWritesArrayThroughCalls (handler.decl->body, state->name, processor.decl->functions, visitedFuncs))
                {
                    readOnlyInProcess = false;
                    break;
                }
            }
        }

        YdspIrFunction::StateArray entry;
        entry.type = type;
        entry.base = base;
        entry.length = state->arraySize;
        entry.readOnlyInProcess = readOnlyInProcess;
        entry.privateToProcess = ! isInit && ! isEventHandler && processor.eventHandlers.empty();

        if (! isInit)
            detail::populateConstantElements (processor, *state, entry);

        fn.stateArrays.push_back (std::move (entry));
    }

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

void YdspIrBuilder::loadScalarStateRegisters()
{
    for (const auto* state : processor.states)
    {
        if (state->arraySize > 0 || ! state->structName.isEmpty())
            continue;

        stateRegs[state] = emitInst ({ stateIsInt (state) ? YdspIrOp::loadStateI : YdspIrOp::loadStateF,
                                       newValue (toStorageType (state->type)),
                                       stateScalarSlot (state) });
    }
}

void YdspIrBuilder::storeScalarStateRegisters()
{
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
}

//==============================================================================

void YdspIrBuilder::buildSampleLoop()
{
    collectSharedInputDelays();
    sampleIndex = emitConstI (0);
    const auto one = emitConstI (1);

    loadScalarStateRegisters();

    const int header = newBlock();
    currentBlock = header;

    const auto cond = emitInst ({ YdspIrOp::ltI, newValue (YdspValueType::boolType), sampleIndex, blockSizeValue });

    const int bodyStart = newBlock();

    setTerminator (YdspIrTerm::branchIf, cond, bodyStart, -1);
    currentBlock = bodyStart;

    lowerStatements (processor.decl->process->body);

    for (const auto& [slot, reg, captureExpr] : pendingPrevStores)
    {
        const auto resolved = coerceTo (captureValue (*captureExpr), YdspValueType::float32Type);
        writeHiddenSlot (slot, reg, resolved, YdspValueType::float32Type);
    }

    for (const auto& history : sharedInputDelays)
    {
        if (history.writePosition < 0)
            continue;

        const auto nextPosition = emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), history.writePosition, one });
        emitInst ({ YdspIrOp::andI, history.writePosition, nextPosition, emitConstI (history.size - 1) });
    }

    const auto next = emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), sampleIndex, one });
    emitInst ({ YdspIrOp::movI, sampleIndex, next });

    setTerminator (YdspIrTerm::branch, -1, header);

    const int epilogue = newBlock();
    fn.blocks[static_cast<size_t> (header)].termTarget2 = epilogue;

    currentBlock = epilogue;

    storeScalarStateRegisters();

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

void YdspIrBuilder::collectSharedInputDelays()
{
    for (const auto* input : processor.inputStreams)
    {
        std::vector<const YdspExpr*> taps;
        int maxDelay = 0;

        for (const auto& statement : processor.decl->process->body)
        {
            if (statement->kind != YdspStmtKind::assign && statement->kind != YdspStmtKind::localDecl)
                break;

            if (statement->kind == YdspStmtKind::localDecl && statement->name == input->name)
                break;

            std::vector<const YdspExpr*> pending;
            if (statement->value != nullptr)
                pending.push_back (statement->value.get());

            while (! pending.empty())
            {
                const auto* expr = pending.back();
                pending.pop_back();

                if (expr->kind != YdspExprKind::unary && expr->kind != YdspExprKind::binary
                    && expr->kind != YdspExprKind::ternary && expr->kind != YdspExprKind::delay)
                    continue;

                if (expr->kind == YdspExprKind::delay
                    && expr->children[0]->kind == YdspExprKind::identifier
                    && expr->children[0]->text == input->name)
                {
                    const auto amount = expr->children[1]->integer;
                    if (amount > 0 && amount <= 65536)
                    {
                        taps.push_back (expr);
                        maxDelay = std::max (maxDelay, static_cast<int> (amount));
                    }
                }

                for (const auto& child : expr->children)
                    pending.push_back (child.get());
            }
        }

        if (taps.size() < 2)
            continue;

        const auto index = sharedInputDelays.size();
        sharedInputDelays.push_back ({ static_cast<int> (std::bit_ceil (static_cast<unsigned> (maxDelay + 1))) });
        for (const auto* tap : taps)
            sharedInputDelayTaps.emplace (tap, index);
    }
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
        emitInst ({ moveOpcodeFor (type), reg, value });
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
