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

namespace
{

bool stmtContainsEarlyReturn (const YdspStmt& stmt, bool topLevel, int& topLevelReturns)
{
    if (stmt.kind == YdspStmtKind::returnStmt)
    {
        if (! topLevel)
            return true;

        return ++topLevelReturns > 1;
    }

    const auto nestedContainsEarlyReturn = [&] (const YdspStmt* body)
    {
        return body != nullptr && stmtContainsEarlyReturn (*body, false, topLevelReturns);
    };

    switch (stmt.kind)
    {
        case YdspStmtKind::block:
            for (const auto& child : stmt.children)
                if (child != nullptr && stmtContainsEarlyReturn (*child, false, topLevelReturns))
                    return true;
            return false;

        case YdspStmtKind::ifStmt:
            return nestedContainsEarlyReturn (stmt.thenStmt.get())
                || nestedContainsEarlyReturn (stmt.elseStmt.get());

        case YdspStmtKind::forStmt:
            return nestedContainsEarlyReturn (stmt.body.get());

        default:
            return false;
    }
}

bool containsNestedOrMultipleReturns (const std::vector<std::unique_ptr<YdspStmt>>& statements)
{
    int topLevelReturns = 0;

    for (const auto& stmt : statements)
    {
        if (stmt != nullptr && stmtContainsEarlyReturn (*stmt, true, topLevelReturns))
            return true;
    }

    return false;
}

} // namespace

//==============================================================================

int YdspIrBuilder::lowerFunctionCall (const YdspAnalyzedFunc& func, const YdspExpr& expr)
{
    const auto& decl = *func.decl;

    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        diagnostics.addError (expr.location.line, expr.location.column, "Function call chain nested too deeply to inline");
        return emitConstF (0.0);
    }

    if (! functionsBeingInlined.insert (decl.name).second)
    {
        diagnostics.addError (expr.location.line, expr.location.column, "Recursive call to '" + decl.name + "' cannot be inlined");
        return emitConstF (0.0);
    }

    const auto savedLocals = locals;
    const auto savedLowerFunctionReturns = lowerFunctionReturns;
    const auto savedReturnSlot = returnSlot;
    const auto savedReturnBlocks = returnBlocks;

    for (size_t i = 0; i < decl.params.size() && i < expr.children.size(); ++i)
    {
        const auto argValue = lowerExpr (*expr.children[static_cast<size_t> (i)]);
        const auto& paramName = decl.params[i].first;
        const auto paramType = toStorageType (decl.params[i].second);

        const auto coerced = coerceTo (argValue, paramType);
        const auto paramValue = newValue (paramType);
        emitInst ({ moveOpcodeFor (paramType), paramValue, coerced });
        locals[paramName] = paramValue;
    }

    // A single top-level `return` does not need terminator lowering: keeping
    // the original straight-line layout preserves intra-block copy propagation
    // and produces wasm-structured control flow. Only functions whose returns
    // are nested (early returns inside if/else) need the branch-terminator
    // path below.
    const bool hasTerminatingReturns = containsNestedOrMultipleReturns (decl.body);

    lowerFunctionReturns = false;
    returnSlot = -1;
    returnBlocks.clear();

    if (hasTerminatingReturns)
    {
        // A body that is only a chain of `if (c) { return A; } ... return Z;`
        // statements is lowered into nested selects: straight-line IR with no
        // branch out of an if region, which both the asm and wasm backends
        // handle, and which keeps the branch that actually fires.
        const auto buildChain = [&] (auto&& self, size_t index) -> int
        {
            if (index >= decl.body.size())
                return -1;

            const auto& stmt = decl.body[index];

            if (stmt->kind == YdspStmtKind::returnStmt)
            {
                if (stmt->returnExpr == nullptr)
                    return -1;

                return lowerExpr (*stmt->returnExpr);
            }

            if (stmt->kind != YdspStmtKind::ifStmt || stmt->elseStmt != nullptr)
                return -1;

            const auto* thenBodyPtr = stmt->thenStmt ? &stmt->thenStmt->children : nullptr;
            if (thenBodyPtr == nullptr || thenBodyPtr->size() != 1
                || (*thenBodyPtr)[0]->kind != YdspStmtKind::returnStmt
                || (*thenBodyPtr)[0]->returnExpr == nullptr)
                return -1;

            const auto& thenBody = *thenBodyPtr;
            const auto cond = lowerExpr (*stmt->cond);
            const auto thenValue = lowerExpr (*thenBody[0]->returnExpr);
            const auto elseValue = self (self, index + 1);

            if (elseValue < 0)
                return -1;

            const auto resultType = valueTypes[static_cast<size_t> (thenValue)];
            const auto coercedElse = coerceTo (elseValue, resultType);

            return emitInst ({ YdspIrOp::selectB,
                               newValue (resultType),
                               cond,
                               thenValue,
                               coercedElse });
        };

        const int chainedResult = buildChain (buildChain, 0);

        if (chainedResult >= 0)
        {
            locals = savedLocals;
            lowerFunctionReturns = savedLowerFunctionReturns;
            returnSlot = savedReturnSlot;
            returnBlocks = std::move (savedReturnBlocks);

            functionsBeingInlined.erase (decl.name);

            return chainedResult;
        }
    }

    if (! hasTerminatingReturns)
    {
        returnValue = -1;
        lowerFunctionBody (decl.body);

        const int result = returnValue >= 0 ? returnValue : emitConstF (0.0);

        locals = savedLocals;
        lowerFunctionReturns = savedLowerFunctionReturns;
        returnSlot = savedReturnSlot;
        returnBlocks = std::move (savedReturnBlocks);

        functionsBeingInlined.erase (decl.name);

        return result;
    }

    lowerFunctionReturns = true;
    returnSlot = func.hasReturnType ? newValue (toStorageType (func.returnType)) : -1;
    returnValue = -1;
    returnBlocks.clear();

    if (returnSlot >= 0)
    {
        const auto slotType = valueTypes[static_cast<size_t> (returnSlot)];
        const auto zero = slotType == YdspValueType::boolType    ? emitConstB (false)
                        : slotType == YdspValueType::int32Type   ? emitConstI (0)
                        : slotType == YdspValueType::int64Type   ? emitConstI64 (0)
                        : slotType == YdspValueType::float64Type ? emitConstF64 (0.0)
                                                                 : emitConstF (0.0);
        emitInst ({ moveOpcodeFor (slotType), returnSlot, zero });
    }

    const int functionEntry = newBlock();
    if (fn.blocks[static_cast<size_t> (currentBlock)].term == YdspIrTerm::fallthrough)
    {
        auto& callerBlock = fn.blocks[static_cast<size_t> (currentBlock)];
        callerBlock.term = YdspIrTerm::branch;
        callerBlock.termTarget = functionEntry;
    }

    currentBlock = functionEntry;

    lowerFunctionBody (decl.body);

    const int functionJoin = newBlock();

    for (const auto returnBlock : returnBlocks)
    {
        auto& block = fn.blocks[static_cast<size_t> (returnBlock)];
        block.term = YdspIrTerm::branch;
        block.termTarget = functionJoin;
    }

    auto& endBlock = fn.blocks[static_cast<size_t> (currentBlock)];
    if (endBlock.term == YdspIrTerm::fallthrough)
    {
        endBlock.term = YdspIrTerm::branch;
        endBlock.termTarget = functionJoin;
    }

    currentBlock = functionJoin;

    const int result = returnSlot >= 0 ? returnSlot
                     : returnValue >= 0 ? returnValue
                                        : emitConstF (0.0);

    locals = savedLocals;
    lowerFunctionReturns = savedLowerFunctionReturns;
    returnSlot = savedReturnSlot;
    returnBlocks = std::move (savedReturnBlocks);

    functionsBeingInlined.erase (decl.name);

    return result;
}

//==============================================================================

void YdspIrBuilder::lowerFunctionBody (const std::vector<std::unique_ptr<YdspStmt>>& body)
{
    for (const auto& stmt : body)
    {
        if (stmt == nullptr)
            continue;

        if (stmt->kind == YdspStmtKind::returnStmt && ! lowerFunctionReturns)
        {
            if (stmt->returnExpr != nullptr)
                returnValue = lowerExpr (*stmt->returnExpr);

            return;
        }

        lowerStatement (*stmt);

        if (lowerFunctionReturns && fn.blocks[static_cast<size_t> (currentBlock)].term != YdspIrTerm::fallthrough)
            return;
    }
}

} // namespace yup
