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

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

namespace yup
{

//==============================================================================
/** Maps one identifier to another during a deep clone (subgraph inlining). */
using YdspRenameMap = std::unordered_map<String, String>;

/** Applies `renames` to `name`, or returns `name` unchanged. */
inline String ydspRenamedTo (const YdspRenameMap& renames, const String& name)
{
    const auto entry = renames.find (name);
    return entry == renames.end() ? name : entry->second;
}

//==============================================================================
// One deep clone pair for the whole AST. Three implementations (the parser's
// expression clone, the compiler's statement clone and the subgraph-fusion
// rename clone) used to walk the same node fields with three bodies; they all
// live here now, parameterised by an optional rename map (nullptr = verbatim).
//
// A rename applies to identifier spellings only - expression text except the
// head of a `member` access (`a.b` keeps `a`, renames `b` through the child
// expression), and a statement's declared/assigned name.

YdspExprPtr ydspCloneExpr (const YdspExpr& source, const YdspRenameMap* renames = nullptr);
YdspStmtPtr ydspCloneStmt (const YdspStmt& source, const YdspRenameMap* renames = nullptr);

inline YdspExprPtr ydspCloneExpr (const YdspExpr& source, const YdspRenameMap* renames)
{
    auto copy = std::make_unique<YdspExpr>();

    copy->kind = source.kind;
    copy->location = source.location;
    copy->number = source.number;
    copy->flag = source.flag;
    copy->op = source.op;

    copy->text = renames != nullptr && source.kind != YdspExprKind::member
                   ? ydspRenamedTo (*renames, source.text)
                   : source.text;

    for (const auto& child : source.children)
        copy->children.push_back (child != nullptr ? ydspCloneExpr (*child, renames) : YdspExprPtr {});

    for (const auto& [name, value] : source.overrides)
        copy->overrides.emplace_back (name, value != nullptr ? ydspCloneExpr (*value, renames) : YdspExprPtr {});

    return copy;
}

inline YdspStmtPtr ydspCloneStmt (const YdspStmt& source, const YdspRenameMap* renames)
{
    auto copy = std::make_unique<YdspStmt>();

    copy->kind = source.kind;
    copy->location = source.location;
    copy->name = renames != nullptr ? ydspRenamedTo (*renames, source.name) : source.name;
    copy->isLet = source.isLet;
    copy->hasDeclType = source.hasDeclType;
    copy->declType = source.declType;

    const auto expr = [renames] (const YdspExprPtr& e)
    {
        return e != nullptr ? ydspCloneExpr (*e, renames) : YdspExprPtr {};
    };

    const auto stmt = [renames] (const YdspStmtPtr& s)
    {
        return s != nullptr ? ydspCloneStmt (*s, renames) : YdspStmtPtr {};
    };

    copy->cond = expr (source.cond);
    copy->thenStmt = stmt (source.thenStmt);
    copy->elseStmt = stmt (source.elseStmt);
    copy->startExpr = expr (source.startExpr);
    copy->endExpr = expr (source.endExpr);
    copy->body = stmt (source.body);
    copy->target = expr (source.target);
    copy->value = expr (source.value);
    copy->returnExpr = expr (source.returnExpr);

    copy->shapeName = source.shapeName;
    copy->endpointName = source.endpointName;

    for (const auto& child : source.children)
        copy->children.push_back (stmt (child));

    for (const auto& [fieldName, valueExpr] : source.emitFields)
        copy->emitFields.emplace_back (fieldName, expr (valueExpr));

    return copy;
}

//==============================================================================
// Shared child enumeration for the recursive walks (recursion detection, the
// rename walk, constant substitution). Each walker keeps its own semantics but
// no longer re-iterates the AST node's child fields with its own loop.

template <typename F>
inline void ydspForEachSubExpr (const YdspExpr& expr, F&& visit)
{
    for (const auto& child : expr.children)
        if (child != nullptr)
            visit (*child);

    for (const auto& override : expr.overrides)
        if (override.second != nullptr)
            visit (*override.second);
}

template <typename F>
inline void ydspForEachSubExpr (const YdspStmt& stmt, F&& visit)
{
    for (const auto* subExpr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(),
                                 stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (subExpr != nullptr)
            visit (*subExpr);

    for (const auto& field : stmt.emitFields)
        if (field.second != nullptr)
            visit (*field.second);
}

template <typename F>
inline void ydspForEachSubStmt (const YdspStmt& stmt, F&& visit)
{
    for (const auto* subStmt : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (subStmt != nullptr)
            visit (*subStmt);

    for (const auto& child : stmt.children)
        if (child != nullptr)
            visit (*child);
}

// Non-const overloads for the mutating walks (rename / substitute).

template <typename F>
inline void ydspForEachSubExpr (YdspExpr& expr, F&& visit)
{
    for (auto& child : expr.children)
        if (child != nullptr)
            visit (*child);

    for (auto& override : expr.overrides)
        if (override.second != nullptr)
            visit (*override.second);
}

template <typename F>
inline void ydspForEachSubExpr (YdspStmt& stmt, F&& visit)
{
    for (auto* subExpr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(),
                           stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (subExpr != nullptr)
            visit (*subExpr);

    for (auto& field : stmt.emitFields)
        if (field.second != nullptr)
            visit (*field.second);
}

template <typename F>
inline void ydspForEachSubStmt (YdspStmt& stmt, F&& visit)
{
    for (auto* subStmt : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (subStmt != nullptr)
            visit (*subStmt);

    for (auto& child : stmt.children)
        if (child != nullptr)
            visit (*child);
}

} // namespace yup
