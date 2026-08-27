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

    // Save local variable state so parameters don't leak into the caller.
    std::unordered_map<String, int> savedLocals = locals;

    // Bind arguments to parameters as local variables. Parameters are
    // pass-by-value: each argument is copied into a fresh value. IR locals
    // are mutable single-register slots, so aliasing the argument (coerceTo
    // returns it unchanged when the types already match) would let the
    // function body clobber the caller's local - e.g. `t = t / dt` inside
    // `polyBlep` corrupted the caller's `phase`, which is stored back to
    // state at the end of the block.
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

    // Lower the function body, capturing any return value.
    returnValue = -1;
    lowerFunctionBody (decl.body);

    const int result = returnValue >= 0 ? returnValue : emitConstF (0.0);

    // Restore caller locals.
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

            return; // Stop execution at the return
        }

        lowerStatement (*stmt);

        // If a return was encountered in a nested statement, stop.
        if (returnValue >= 0)
            return;
    }
}

} // namespace yup
