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

    bool found = false;

    ydspForEachSubExpr (expr, [&] (const YdspExpr& child)
    {
        found = found || exprCallsFunction (child, funcName);
    });

    return found;
}

bool stmtCallsFunction (const YdspStmt& stmt, const String& funcName)
{
    bool found = false;

    ydspForEachSubExpr (stmt, [&] (const YdspExpr& expr)
    {
        found = found || exprCallsFunction (expr, funcName);
    });

    ydspForEachSubStmt (stmt, [&] (const YdspStmt& child)
    {
        found = found || stmtCallsFunction (child, funcName);
    });

    return found;
}

bool funcBodyCallsFunction (const std::vector<std::unique_ptr<YdspStmt>>& body, const String& funcName)
{
    for (const auto& stmt : body)
        if (stmt != nullptr && stmtCallsFunction (*stmt, funcName))
            return true;

    return false;
}

// True when `from` (transitively, through at least one intermediate call)
// reaches the function named `start` within `pool` - i.e. f calls g calls f.
bool callsIndirectlyBackTo (const String& start, const YdspAnalyzedFunc& from, const std::vector<YdspAnalyzedFunc>& pool,
                            std::vector<String>& visited, int hops)
{
    for (const auto& func : pool)
    {
        if (func.decl->name == start)
        {
            if (hops > 0 && funcBodyCallsFunction (from.decl->body, start))
                return true;

            continue;
        }

        if (! funcBodyCallsFunction (from.decl->body, func.decl->name))
            continue;

        if (std::find (visited.begin(), visited.end(), func.decl->name) != visited.end())
            continue;

        visited.push_back (func.decl->name);

        if (callsIndirectlyBackTo (start, func, pool, visited, hops + 1))
            return true;

        visited.pop_back();
    }

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

    detectRecursiveFunctions (proc.functions);
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

    detectRecursiveFunctions (out);
}

//==============================================================================

bool YdspSemanticAnalyzer::resolveFunctionCall (const String& name, const std::vector<YdspExprPtr>& args, const std::vector<YdspValueType>& argTypes, const YdspLocation& location, YdspValueType& returnType)
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

    // The arguments were already type-checked as expressions; verify each one
    // is usable where its parameter sits (widening within a kind, or a literal
    // the analyzer can adapt - an int/float mix is a compile error elsewhere).
    for (int i = 0; i < numParams; ++i)
    {
        const auto paramType = toValueType (params[static_cast<size_t> (i)].second);

        if (argTypes[static_cast<size_t> (i)] == paramType
            || isImplicitlyConvertibleTo (argTypes[static_cast<size_t> (i)], paramType)
            || isAdaptableTo (*args[static_cast<size_t> (i)], paramType))
            continue;

        error (location,
               "Function '" + name + "' parameter " + String (i + 1) + " has an incompatible type (use an explicit cast)");
    }

    returnType = func->hasReturnType ? static_cast<YdspValueType> (func->returnType) : YdspValueType::float32Type;

    return true;
}

//==============================================================================

void YdspSemanticAnalyzer::detectRecursiveFunctions (const std::vector<YdspAnalyzedFunc>& functions)
{
    for (const auto& func : functions)
        if (funcBodyCallsFunction (func.decl->body, func.decl->name))
            error (func.decl->location, "Recursive call detected in function '" + func.decl->name + "'. Functions cannot call themselves.");

    // Mutual recursion through another function: f -> g -> f.
    for (const auto& func : functions)
    {
        std::vector<String> visited;

        if (callsIndirectlyBackTo (func.decl->name, func, functions, visited, 0))
            error (func.decl->location, "Mutual recursion detected in function '" + func.decl->name + "'. Functions cannot call each other.");
    }
}

} // namespace yup
