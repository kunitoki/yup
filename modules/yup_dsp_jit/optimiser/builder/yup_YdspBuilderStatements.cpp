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

void YdspIrBuilder::lowerStatements (const std::vector<std::unique_ptr<YdspStmt>>& statements)
{
    for (const auto& statement : statements)
        lowerStatement (*statement);
}

void YdspIrBuilder::lowerStatement (const YdspStmt& stmt)
{
    switch (stmt.kind)
    {
        case YdspStmtKind::block:
            lowerStatements (stmt.children);
            break;

        case YdspStmtKind::localDecl:
        {
            if (stmt.value != nullptr)
            {
                const auto value = lowerExpr (*stmt.value);
                const auto type = stmt.hasDeclType ? toStorageType (stmt.declType) : valueTypes[static_cast<size_t> (value)];
                locals[stmt.name] = stmt.hasDeclType ? coerceTo (value, type) : value;
            }
            else
            {
                const auto type = toStorageType (stmt.declType);

                locals[stmt.name] = type == YdspValueType::int32Type   ? emitConstI (0)
                                  : type == YdspValueType::int64Type   ? emitConstI64 (0)
                                  : type == YdspValueType::float64Type ? emitConstF64 (0.0)
                                  : type == YdspValueType::boolType    ? emitConstB (false)
                                                                       : emitConstF (0.0);
            }

            break;
        }

        case YdspStmtKind::assign:
            lowerAssignment (stmt);
            break;

        case YdspStmtKind::returnStmt:
            if (stmt.returnExpr != nullptr)
                returnValue = lowerExpr (*stmt.returnExpr);
            break;

        case YdspStmtKind::ifStmt:
        {
            const auto cond = lowerExpr (*stmt.cond);

            const bool hasElse = (stmt.elseStmt != nullptr);

            // Create blocks in CFG-linear order: then, [else], join. The join is
            // always created *after* both regions have been lowered, so every
            // block they allocate stays inside the region and the join is the
            // region's last block — the codegen falls through blockIndex + 1 and
            // the wasm backend recovers the region's extent from the join index,
            // so a join allocated up front truncates the region. (An `else if`
            // chain had its inner blocks pushed past the join, leaving the outer
            // join falling into the inner then; a `for` inside an else-less `if`
            // had its preheader fall into the join and the join fall into the
            // loop header, whose induction variable was never initialised — an
            // infinite loop in the generated kernel.)
            const int condBlock = currentBlock;

            const int thenBlock = newBlock();

            currentBlock = thenBlock;
            lowerStatement (*stmt.thenStmt);

            const int thenTail = currentBlock;

            int elseBlock = -1;
            int elseTail = -1;

            if (hasElse)
            {
                elseBlock = newBlock();

                currentBlock = elseBlock;
                lowerStatement (*stmt.elseStmt);

                elseTail = currentBlock;
            }

            const int join = newBlock();

            fn.blocks[static_cast<size_t> (condBlock)].term = YdspIrTerm::branchIf;
            fn.blocks[static_cast<size_t> (condBlock)].termCond = cond;
            fn.blocks[static_cast<size_t> (condBlock)].termTarget = thenBlock;
            fn.blocks[static_cast<size_t> (condBlock)].termTarget2 = hasElse ? elseBlock : join;

            fn.blocks[static_cast<size_t> (thenTail)].term = YdspIrTerm::branch;
            fn.blocks[static_cast<size_t> (thenTail)].termTarget = join;

            if (elseTail >= 0)
            {
                fn.blocks[static_cast<size_t> (elseTail)].term = YdspIrTerm::branch;
                fn.blocks[static_cast<size_t> (elseTail)].termTarget = join;
            }

            currentBlock = join;

            break;
        }

        case YdspStmtKind::forStmt:
        {
            // preheader (current block): initialize the induction variable
            const auto startValue = lowerExpr (*stmt.startExpr);
            const auto induction = newValue (YdspValueType::int32Type);
            emitInst ({ YdspIrOp::movI, induction, startValue });

            // The preheader falls through into the header, so the header must be
            // the block that physically follows it - every enclosing construct
            // has to leave the region's blocks contiguous (see the ifStmt case).
            const int header = newBlock();
            const int body = newBlock();

            // header: compute the bound and compare. The exit block is
            // created after the body so nested if/for blocks stay in
            // CFG-linear order (the codegen falls through blockIndex+1).
            currentBlock = header;

            // Capture the resolved bound immediately: lowering the body below
            // may lower a *nested* loop, which resolves its own bound.
            YdspLoopBound resolvedBound;
            const auto boundValue = emitLoopBound (stmt, resolvedBound);

            const auto cond = emitInst ({ YdspIrOp::ltI, newValue (YdspValueType::boolType), induction, boundValue });
            setTerminator (YdspIrTerm::branchIf, cond, body, -1); // exit patched below

            currentBlock = body;

            // The induction variable is scoped to the body: restore whatever
            // the name meant outside on the way out (the analyzer enforces the
            // same scoping).
            const auto shadowed = locals.find (stmt.name);
            const auto hadShadowed = shadowed != locals.end();
            const auto shadowedValue = hadShadowed ? shadowed->second : -1;

            locals[stmt.name] = induction;

            lowerStatement (*stmt.body);

            const auto one = emitConstI (1);
            const auto next = emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), induction, one });
            emitInst ({ YdspIrOp::movI, induction, next });
            setTerminator (YdspIrTerm::branch, -1, header);

            const int exit = newBlock();
            fn.blocks[static_cast<size_t> (header)].termTarget2 = exit;

            YdspIrLoop loop;
            loop.id = static_cast<int> (fn.loops.size());
            loop.headerBlock = header;
            loop.exitBlock = exit;
            loop.induction = induction;
            loop.bound = resolvedBound;
            fn.loops.push_back (loop);

            if (hadShadowed)
                locals[stmt.name] = shadowedValue;
            else
                locals.erase (stmt.name);

            currentBlock = exit;
            break;
        }

        case YdspStmtKind::emitStmt:
        {
            const auto* shape = findEventShape (stmt.shapeName);

            if (shape == nullptr)
            {
                diagnostics.addError (stmt.location.line, stmt.location.column, "Unknown event shape '" + stmt.shapeName + "'");
                break;
            }

            if (! isEventHandler && sampleIndex < 0)
            {
                diagnostics.addError (stmt.location.line, stmt.location.column, "emit is not allowed in init or block-mode process (only in the per-sample process body or an event handler)");
                break;
            }

            for (const auto& [fieldName, valueExpr] : stmt.emitFields)
            {
                const auto* field = findEventField (*shape, fieldName);

                if (field == nullptr)
                {
                    diagnostics.addError (valueExpr->location.line, valueExpr->location.column, "Unknown event field '" + fieldName + "'");
                    continue;
                }

                const auto value = coerceTo (lowerExpr (*valueExpr), field->type);

                emitInst ({ isFloatValueType (field->type) ? YdspIrOp::storeEventFieldF : YdspIrOp::storeEventFieldI,
                            -1,
                            value,
                            -1,
                            -1,
                            field->byteOffset });
            }

            for (const auto& field : shape->fields)
            {
                if (field.name == nullptr)
                    continue;

                bool provided = false;

                for (const auto& emitField : stmt.emitFields)
                {
                    if (emitField.first == StringRef (field.name))
                    {
                        provided = true;
                        break;
                    }
                }

                if (provided)
                    continue;

                const auto zero = field.type == YdspValueType::boolType ? emitConstB (false)
                                 : isFloatValueType (field.type)         ? emitConstF (0.0)
                                                                         : emitConstI (0);

                emitInst ({ isFloatValueType (field.type) ? YdspIrOp::storeEventFieldF : YdspIrOp::storeEventFieldI,
                            -1,
                            zero,
                            -1,
                            -1,
                            field.byteOffset });
            }

            int endpointIndex = -1;

            for (size_t i = 0; i < processor.outputEvents.size(); ++i)
            {
                if (processor.outputEvents[i]->name == stmt.endpointName)
                {
                    endpointIndex = static_cast<int> (i);
                    break;
                }
            }

            if (endpointIndex < 0)
            {
                diagnostics.addError (stmt.location.line, stmt.location.column, "Unknown emit target '" + stmt.endpointName + "'");
                break;
            }

            const auto sampleOffset = isEventHandler
                                         ? emitInst ({ YdspIrOp::loadEventFieldI, newValue (YdspValueType::int32Type), -1, -1, -1, static_cast<int> (offsetof (YdspEventContext, sampleOffset)) })
                                         : sampleIndex;

            emitInst ({ YdspIrOp::emitEvent, -1, sampleOffset, -1, -1, endpointIndex, 0.0, static_cast<int64_t> (shape->shape) });

            break;
        }

        default:
            break;
    }
}

//==============================================================================

int YdspIrBuilder::emitLoopBound (const YdspStmt& stmt, YdspLoopBound& outBound)
{
    YdspLoopBound bound;

    if (! resolveLoopBoundForStmt (stmt, bound))
        return emitConstI (0);

    outBound = bound;

    switch (bound.kind)
    {
        case YdspLoopBoundKind::constant:
            return emitConstI (bound.constant);

        case YdspLoopBoundKind::blockSize:
            return blockSizeValue;

        case YdspLoopBoundKind::blockSizeMinusConst:
        {
            const auto k = emitConstI (bound.constant);
            return emitInst ({ YdspIrOp::subI, newValue (YdspValueType::int32Type), blockSizeValue, k });
        }

        case YdspLoopBoundKind::blockSizePlusConst:
        {
            const auto k = emitConstI (bound.constant);
            return emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), blockSizeValue, k });
        }
    }

    return emitConstI (0);
}

//==============================================================================

bool YdspIrBuilder::resolveLoopBoundForStmt (const YdspStmt& stmt, YdspLoopBound& out) const
{
    if (stmt.endExpr == nullptr)
        return false;

    // Reuse the analyzer's bound resolution logic on the AST.
    if (stmt.endExpr->kind == YdspExprKind::intLiteral)
    {
        const auto value = static_cast<long long> (stmt.endExpr->number);
        if (value < 0)
            return false;

        out = { YdspLoopBoundKind::constant, static_cast<int> (value) };
        return true;
    }

    if (stmt.endExpr->kind == YdspExprKind::identifier && stmt.endExpr->text == "blockSize")
    {
        out = { YdspLoopBoundKind::blockSize, 0 };
        return true;
    }

    if (stmt.endExpr->kind == YdspExprKind::binary && stmt.endExpr->children.size() == 2)
    {
        const auto& lhs = *stmt.endExpr->children[0];
        const auto& rhs = *stmt.endExpr->children[1];

        const bool lhsIsBlockSize = lhs.kind == YdspExprKind::identifier && lhs.text == "blockSize";
        const bool rhsIsBlockSize = rhs.kind == YdspExprKind::identifier && rhs.text == "blockSize";

        if (stmt.endExpr->op == YdspOperator::sub && lhsIsBlockSize && rhs.kind == YdspExprKind::intLiteral)
        {
            const auto value = static_cast<long long> (rhs.number);
            if (value < 0)
                return false;

            out = { YdspLoopBoundKind::blockSizeMinusConst, static_cast<int> (value) };
            return true;
        }

        if (stmt.endExpr->op == YdspOperator::add && (lhsIsBlockSize || rhsIsBlockSize))
        {
            const auto& constSide = lhsIsBlockSize ? rhs : lhs;

            if (constSide.kind == YdspExprKind::intLiteral)
            {
                const auto value = static_cast<long long> (constSide.number);
                if (value < 0)
                    return false;

                out = { YdspLoopBoundKind::blockSizePlusConst, static_cast<int> (value) };
                return true;
            }
        }
    }

    return false;
}

//==============================================================================

void YdspIrBuilder::lowerAssignment (const YdspStmt& stmt)
{
    const auto& target = *stmt.target;
    const auto value = lowerExpr (*stmt.value);

    if (target.kind == YdspExprKind::identifier)
    {
        const auto it = locals.find (target.text);

        if (it != locals.end())
        {
            const auto localType = valueTypes[static_cast<size_t> (it->second)];
            const auto coerced = coerceTo (value, localType);
            emitInst ({ movOpFor (localType), it->second, coerced });
            return;
        }

        // Resolve against processor endpoints / states / builtins.
        if (const auto* endpoint = findEndpoint (target.text))
        {
            const auto endpointType = toStorageType (endpoint->type);

            switch (endpoint->kind)
            {
                case YdspEndpointKind::outputStream:
                {
                    const auto valueCoerced = coerceTo (value, endpointType);
                    emitInst ({ YdspIrOp::storeOutput, -1, sampleIndex, valueCoerced, -1, endpointStreamIndex (endpoint) });
                    lastOutputValue[static_cast<size_t> (endpointStreamIndex (endpoint))] = valueCoerced;
                    return;
                }

                case YdspEndpointKind::inputValue:
                    emitInst ({ YdspIrOp::storeParam, -1, coerceTo (value, endpointType), -1, -1, endpointValueIndex (endpoint) });
                    return;

                case YdspEndpointKind::outputValue:
                    emitInst ({ YdspIrOp::storeParamOut, -1, coerceTo (value, endpointType), -1, -1, endpointValueIndex (endpoint) });
                    return;

                default:
                    diagnostics.addError (target.location.line, target.location.column, "Invalid assignment target '" + target.text + "'");
                    return;
            }
        }

        if (const auto* state = findState (target.text))
        {
            if (state->arraySize > 0)
            {
                diagnostics.addError (target.location.line, target.location.column, "State array '" + target.text + "' must be indexed");
                return;
            }

            lowerStateStore (state, value);
            return;
        }

        diagnostics.addError (target.location.line, target.location.column, "Unknown assignment target '" + target.text + "'");
        return;
    }

    if (target.kind == YdspExprKind::index)
    {
        const auto& base = *target.children[0];
        const auto index = lowerExpr (*target.children[1]);

        if (base.kind == YdspExprKind::member)
        {
            // `state.buf[i] = v` / `voices[i].buf[j] = v`.
            int structBase = 0, stride = 0, instanceIndex = -1;
            YdspValueType structType = YdspValueType::float32Type;

            if (resolveStructFieldLayout (base, structBase, stride, instanceIndex, structType))
            {
                int elemIndex = index;

                if (instanceIndex >= 0 && stride > 0)
                {
                    const auto strideValue = emitConstIFor (stride, YdspValueType::int32Type);
                    const auto scaled = emitInst ({ YdspIrOp::mulI, newValue (YdspValueType::int32Type), instanceIndex, strideValue });
                    elemIndex = emitInst ({ YdspIrOp::addI, newValue (YdspValueType::int32Type), scaled, index });
                }

                emitInst ({ isFloatValueType (structType) ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI,
                            -1,
                            elemIndex,
                            coerceTo (value, structType),
                            -1,
                            structBase });
                return;
            }

            diagnostics.addError (target.location.line, target.location.column, "Invalid struct field index assignment target");
            return;
        }

        if (base.kind != YdspExprKind::identifier)
        {
            diagnostics.addError (target.location.line, target.location.column, "Invalid array assignment target");
            return;
        }

        if (const auto* state = findState (base.text))
        {
            if (state->arraySize <= 0)
            {
                diagnostics.addError (base.location.line, base.location.column, "Cannot index scalar state '" + base.text + "'");
                return;
            }

            const auto stateType = toStorageType (state->type);

            emitInst ({ stateIsInt (state) ? YdspIrOp::storeStateArrayI : YdspIrOp::storeStateArrayF,
                        -1,
                        index,
                        coerceTo (value, stateType),
                        -1,
                        stateArrayBase (state) });
            return;
        }

        if (const auto* endpoint = findEndpoint (base.text);
            endpoint != nullptr && endpoint->kind == YdspEndpointKind::outputStream)
        {
            emitInst ({ YdspIrOp::storeOutput, -1, index, coerceTo (value, toStorageType (endpoint->type)), -1, endpointStreamIndex (endpoint) });
            return;
        }

        diagnostics.addError (base.location.line, base.location.column, "Invalid array assignment target '" + base.text + "'");
        return;
    }

    if (target.kind == YdspExprKind::member)
    {
        // `state.field = v` / `voices[i].field = v`.
        int structBase = 0, stride = 0, instanceIndex = -1;
        YdspValueType structType = YdspValueType::float32Type;

        if (! resolveStructFieldLayout (target, structBase, stride, instanceIndex, structType))
        {
            diagnostics.addError (target.location.line, target.location.column, "Invalid struct member assignment target");
            return;
        }

        if (stride == 0)
        {
            emitInst ({ isFloatValueType (structType) ? YdspIrOp::storeStateF : YdspIrOp::storeStateI,
                        -1,
                        coerceTo (value, structType),
                        -1,
                        -1,
                        structBase });
        }
        else
        {
            emitInst ({ isFloatValueType (structType) ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI,
                        -1,
                        instanceIndex,
                        coerceTo (value, structType),
                        -1,
                        structBase });
        }

        return;
    }

    diagnostics.addError (target.location.line, target.location.column, "Invalid assignment target");
}

} // namespace yup
