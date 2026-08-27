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

namespace
{

bool exprCallsFunction (const YdspExpr& expr, const String& funcName)
{
    if (expr.kind == YdspExprKind::call && expr.text == funcName)
        return true;

    for (const auto& child : expr.children)
        if (child != nullptr && exprCallsFunction (*child, funcName))
            return true;

    return false;
}

bool stmtCallsFunction (const YdspStmt& stmt, const String& funcName)
{
    for (const auto* subExpr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(), stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (subExpr != nullptr && exprCallsFunction (*subExpr, funcName))
            return true;

    for (const auto* subStmt : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (subStmt != nullptr && stmtCallsFunction (*subStmt, funcName))
            return true;

    for (const auto& child : stmt.children)
        if (child != nullptr && stmtCallsFunction (*child, funcName))
            return true;

    return false;
}

bool funcBodyCallsFunction (const std::vector<std::unique_ptr<YdspStmt>>& body, const String& funcName)
{
    for (const auto& stmt : body)
        if (stmt != nullptr && stmtCallsFunction (*stmt, funcName))
            return true;

    return false;
}

} // namespace

//==============================================================================

void YdspSemanticAnalyzer::analyzeFunctionBodies (YdspAnalyzedProcessor& proc)
{
    const auto* decl = proc.decl;

    proc.functions.reserve (decl->functions.size());

    currentProcessorFunctions = &proc.functions;

    for (const auto& func : decl->functions)
    {
        if (findFunctionInScope (func.name, &proc.functions, nullptr) != nullptr)
        {
            error (func.location, "Duplicate function '" + func.name + "'");
            continue;
        }

        YdspAnalyzedFunc analyzedFunc;
        analyzedFunc.decl = &func;
        analyzedFunc.returnType = func.returnType;
        analyzedFunc.hasReturnType = func.hasReturnType;

        proc.functions.push_back (std::move (analyzedFunc));
    }

    for (const auto& func : decl->functions)
        if (funcBodyCallsFunction (func.body, func.name))
            error (func.location, "Recursive call detected in function '" + func.name + "'. Functions cannot call themselves.");
}

//==============================================================================

void YdspSemanticAnalyzer::analyzeProgramFunctions (YdspProgram& program, std::vector<YdspAnalyzedFunc>& out)
{
    out.reserve (program.functions.size());

    currentProgramFunctions = &out;

    for (const auto& func : program.functions)
    {
        if (findFunctionInScope (func.name, &out, nullptr) != nullptr)
        {
            error (func.location, "Duplicate function '" + func.name + "'");
            continue;
        }

        YdspAnalyzedFunc analyzedFunc;
        analyzedFunc.decl = &func;
        analyzedFunc.returnType = func.returnType;
        analyzedFunc.hasReturnType = func.hasReturnType;

        out.push_back (std::move (analyzedFunc));
    }

    for (const auto& func : program.functions)
        if (funcBodyCallsFunction (func.body, func.name))
            error (func.location, "Recursive call detected in function '" + func.name + "'. Functions cannot call themselves.");
}

//==============================================================================

bool YdspSemanticAnalyzer::resolveFunctionCall (const String& name, const std::vector<YdspExprPtr>& args, const YdspLocation& location, YdspValueType& returnType)
{
    const auto* func = findFunctionInScope (name, currentProcessorFunctions, currentProgramFunctions);

    if (func == nullptr)
        return false;

    const auto& params = func->decl->params;
    const int numArgs = static_cast<int> (args.size());
    const int numParams = static_cast<int> (params.size());

    if (numArgs != numParams)
    {
        error (location, "Function '" + name + "' expects " + String (numParams) + " arguments, got " + String (numArgs));
        returnType = YdspValueType::float32Type;
        return true;
    }

    for (int i = 0; i < numArgs; ++i)
    {
        [[maybe_unused]] const auto argType = static_cast<YdspValueType> (params[static_cast<size_t> (i)].second);
    }

    returnType = func->hasReturnType ? static_cast<YdspValueType> (func->returnType) : YdspValueType::float32Type;

    return true;
}

} // namespace yup
