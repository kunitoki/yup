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

    std::unordered_map<String, int> savedLocals = locals;

    for (size_t i = 0; i < decl.params.size() && i < expr.children.size(); ++i)
    {
        const auto argValue = lowerExpr (*expr.children[static_cast<size_t> (i)]);
        const auto& paramName = decl.params[i].first;
        const auto paramType = toStorageType (decl.params[i].second);

        const auto coerced = coerceTo (argValue, paramType);
        const auto paramValue = newValue (paramType);
        emitInst ({ movOpFor (paramType), paramValue, coerced });
        locals[paramName] = paramValue;
    }

    returnValue = -1;
    lowerFunctionBody (decl.body);

    const int result = returnValue >= 0 ? returnValue : emitConstF (0.0);

    locals = std::move (savedLocals);

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

        if (stmt->kind == YdspStmtKind::returnStmt)
        {
            if (stmt->returnExpr != nullptr)
                returnValue = lowerExpr (*stmt->returnExpr);

            return;
        }

        lowerStatement (*stmt);

        if (returnValue >= 0)
            return;
    }
}

} // namespace yup
