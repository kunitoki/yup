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

namespace yup
{

namespace wgsl
{

//==============================================================================
// Node builders. Every builder records the type of the node it creates when known.

inline Expr makeExpr (SourceLocation l, ExprVariant value, std::optional<TypeSpecifier> type = std::nullopt)
{
    Expr e;
    e.loc = l;
    e.value = std::move (value);
    e.type = std::move (type);
    return e;
}

inline TypeSpecifier makeType (TypeKind kind)
{
    return TypeSpecifier::make ({}, kind);
}

inline Expr makeVariable (SourceLocation l, const std::string& name, std::optional<TypeSpecifier> type = std::nullopt)
{
    return makeExpr (l, ExprVariable { l, name }, std::move (type));
}

inline Expr makeIntLiteral (SourceLocation l, int64_t value)
{
    return makeExpr (l, ExprIntConst { l, value }, makeType (TypeKind::intType));
}

inline Expr makeUIntLiteral (SourceLocation l, unsigned int value)
{
    return makeExpr (l, ExprUIntConst { l, value }, makeType (TypeKind::uintType));
}

inline Expr makeFloatLiteral (SourceLocation l, double value)
{
    return makeExpr (l, ExprFloatConst { l, value, false }, makeType (TypeKind::floatType));
}

inline Expr makeBoolLiteral (SourceLocation l, bool value)
{
    return makeExpr (l, ExprBoolConst { l, value }, makeType (TypeKind::boolType));
}

inline std::unique_ptr<Expr> boxed (Expr e)
{
    return std::make_unique<Expr> (std::move (e));
}

inline Expr makeDot (SourceLocation l, Expr base, const std::string& member, std::optional<TypeSpecifier> type = std::nullopt)
{
    return makeExpr (l, ExprDot { l, boxed (std::move (base)), member }, std::move (type));
}

inline Expr makeIndex (SourceLocation l, Expr base, Expr index, std::optional<TypeSpecifier> type = std::nullopt)
{
    return makeExpr (l, ExprBracket { l, boxed (std::move (base)), boxed (std::move (index)) }, std::move (type));
}

inline Expr makeCall (SourceLocation l, const std::string& name, std::vector<Expr> args, std::optional<TypeSpecifier> type = std::nullopt)
{
    ExprFunCall call;
    call.loc = l;
    call.callee = boxed (makeVariable (l, name));
    call.args = std::move (args);
    return makeExpr (l, std::move (call), std::move (type));
}

inline Expr makeUnary (SourceLocation l, UnaryOp op, Expr operand, std::optional<TypeSpecifier> type = std::nullopt)
{
    return makeExpr (l, ExprUnary { l, op, boxed (std::move (operand)) }, std::move (type));
}

inline Expr makeBinary (SourceLocation l, BinaryOp op, Expr left, Expr right, std::optional<TypeSpecifier> type = std::nullopt)
{
    return makeExpr (l, ExprBinary { l, op, boxed (std::move (left)), boxed (std::move (right)) }, std::move (type));
}

inline Expr makeAssign (SourceLocation l, AssignmentOp op, Expr lhs, Expr rhs)
{
    auto type = lhs.type;
    return makeExpr (l, ExprAssignment { l, op, boxed (std::move (lhs)), boxed (std::move (rhs)) }, std::move (type));
}

inline Expr makeConstruct (SourceLocation l, const TypeSpecifier& type, std::vector<Expr> args)
{
    ExprTypeConstructor ctor;
    ctor.loc = l;
    ctor.type = type;
    ctor.args = std::move (args);
    return makeExpr (l, std::move (ctor), type);
}

inline Expr makeParen (SourceLocation l, Expr inner)
{
    auto type = inner.type;
    return makeExpr (l, ExprParen { l, boxed (std::move (inner)) }, std::move (type));
}

inline Statement makeStatement (SourceLocation l, StatementVariant value)
{
    Statement s;
    s.loc = l;
    s.value = std::move (value);
    return s;
}

inline Statement makeExprStatement (SourceLocation l, Expr e)
{
    return makeStatement (l, StmtExpr { l, boxed (std::move (e)) });
}

/** A local declaration: `var name: type = init` or, when isConst, `let name: type = init`. */
inline Statement makeVarDeclaration (SourceLocation l, const std::string& name, const TypeSpecifier& type, std::optional<Expr> init, bool isConst = false)
{
    auto list = std::make_unique<InitDeclaratorList>();
    list->loc = l;
    list->type = type;

    if (isConst)
    {
        list->qualifier = std::make_unique<TypeQualifier>();
        list->qualifier->storage.push_back (StorageQualifier::constQual);
    }

    SingleDeclaration single;
    single.loc = l;
    single.name = name;

    if (init.has_value())
    {
        single.initializer = std::make_unique<Initializer>();
        single.initializer->loc = l;
        single.initializer->expr = boxed (std::move (*init));
    }

    list->declarations.push_back (std::move (single));

    Declaration decl;
    decl.loc = l;
    decl.initDeclaratorList = std::move (list);
    return makeStatement (l, StmtDeclaration { l, std::move (decl) });
}

inline Statement makeBlock (SourceLocation l, std::vector<Statement> statements)
{
    return Statement::makeCompound (l, std::move (statements));
}

inline Statement makeIf (SourceLocation l, Expr condition, Statement thenBranch, std::optional<Statement> elseBranch = std::nullopt)
{
    StmtSelection sel;
    sel.loc = l;
    sel.condition = boxed (std::move (condition));
    sel.thenBranch = std::make_unique<Statement> (std::move (thenBranch));

    if (elseBranch.has_value())
        sel.elseBranch = std::make_unique<Statement> (std::move (*elseBranch));

    return makeStatement (l, std::move (sel));
}

inline Statement makeJumpStatement (SourceLocation l, JumpKind kind)
{
    return makeStatement (l, StmtJump { l, kind, nullptr });
}

inline TypeSpecifier makeArrayType (TypeSpecifier element, int64_t size)
{
    ArraySpecifier array;
    array.sizeExpr = boxed (makeIntLiteral ({}, size));
    element.arraySpecifiers.insert (element.arraySpecifiers.begin(), std::move (array));
    return element;
}

/** The type of one element of an array type. */
inline TypeSpecifier elementType (TypeSpecifier type)
{
    if (! type.arraySpecifiers.empty())
        type.arraySpecifiers.erase (type.arraySpecifiers.begin());

    return type;
}

/** Full type of a declarator, array dimensions outermost first: `float[5] a[3]` and `float a[3][5]` both give [3][5]. */
inline TypeSpecifier declaratorType (const TypeSpecifier& base, const std::vector<ArraySpecifier>& arrays)
{
    auto type = base;
    type.arraySpecifiers.insert (type.arraySpecifiers.begin(), arrays.begin(), arrays.end());
    return type;
}

/** Evaluates an integer constant expression made of literals, or returns nullopt. */
inline std::optional<int64_t> evaluateIntConstant (const Expr& e)
{
    if (e.is<ExprIntConst>())
        return e.as<ExprIntConst>().value;

    if (e.is<ExprUIntConst>())
        return static_cast<int64_t> (e.as<ExprUIntConst>().value);

    if (e.is<ExprParen>() && e.as<ExprParen>().expr != nullptr)
        return evaluateIntConstant (*e.as<ExprParen>().expr);

    if (e.is<ExprUnary>() && e.as<ExprUnary>().operand != nullptr)
    {
        const auto& unary = e.as<ExprUnary>();
        const auto operand = evaluateIntConstant (*unary.operand);

        if (operand.has_value() && unary.op == UnaryOp::minus)
            return -*operand;

        if (operand.has_value() && unary.op == UnaryOp::plus)
            return operand;
    }

    if (e.is<ExprBinary>() && e.as<ExprBinary>().left != nullptr && e.as<ExprBinary>().right != nullptr)
    {
        const auto& binary = e.as<ExprBinary>();
        const auto a = evaluateIntConstant (*binary.left);
        const auto b = evaluateIntConstant (*binary.right);

        if (! a.has_value() || ! b.has_value())
            return std::nullopt;

        switch (binary.op)
        {
            case BinaryOp::add:
                return *a + *b;
            case BinaryOp::sub:
                return *a - *b;
            case BinaryOp::mul:
                return *a * *b;
            case BinaryOp::div:
                return *b != 0 ? std::optional<int64_t> (*a / *b) : std::nullopt;
            default:
                return std::nullopt;
        }
    }

    return std::nullopt;
}

//==============================================================================
// Walkers

/** Calls f on each direct child expression of e. */
template <typename F>
void forEachChildExpr (Expr& e, F&& f)
{
    const auto visitPtr = [&f] (std::unique_ptr<Expr>& child)
    {
        if (child != nullptr)
            f (*child);
    };

    std::visit ([&] (auto& alt)
    {
        using T = std::decay_t<decltype (alt)>;

        if constexpr (std::is_same_v<T, ExprUnary>)
            visitPtr (alt.operand);
        else if constexpr (std::is_same_v<T, ExprBinary>)
        {
            visitPtr (alt.left);
            visitPtr (alt.right);
        }
        else if constexpr (std::is_same_v<T, ExprTernary>)
        {
            visitPtr (alt.condition);
            visitPtr (alt.trueBranch);
            visitPtr (alt.falseBranch);
        }
        else if constexpr (std::is_same_v<T, ExprAssignment>)
        {
            visitPtr (alt.lhs);
            visitPtr (alt.rhs);
        }
        else if constexpr (std::is_same_v<T, ExprBracket>)
        {
            visitPtr (alt.base);
            visitPtr (alt.index);
        }
        else if constexpr (std::is_same_v<T, ExprFunCall>)
        {
            visitPtr (alt.callee);
            for (auto& arg : alt.args)
                f (arg);
        }
        else if constexpr (std::is_same_v<T, ExprDot>)
            visitPtr (alt.base);
        else if constexpr (std::is_same_v<T, ExprComma>)
        {
            visitPtr (alt.left);
            visitPtr (alt.right);
        }
        else if constexpr (std::is_same_v<T, ExprTypeConstructor>)
        {
            for (auto& arg : alt.args)
                f (arg);
        }
        else if constexpr (std::is_same_v<T, ExprParen>)
            visitPtr (alt.expr);
    },
                e.value);
}

/** Visits e and all its descendants, parents first. */
template <typename F>
void walkExpr (Expr& e, F&& f)
{
    f (e);
    forEachChildExpr (e, [&f] (Expr& child)
    {
        walkExpr (child, f);
    });
}

template <typename F>
void walkInitializer (Initializer& init, F&& f)
{
    if (init.expr != nullptr)
        walkExpr (*init.expr, f);

    for (auto& child : init.aggregate)
        walkInitializer (child, f);
}

/** Visits every expression reachable from a declaration: initializers and array sizes. */
template <typename F>
void walkDeclarationExprs (Declaration& decl, F&& f)
{
    if (decl.initDeclaratorList == nullptr)
        return;

    for (auto& array : decl.initDeclaratorList->type.arraySpecifiers)
        if (array.sizeExpr != nullptr)
            walkExpr (*array.sizeExpr, f);

    for (auto& single : decl.initDeclaratorList->declarations)
    {
        for (auto& array : single.arraySpecifiers)
            if (array.sizeExpr != nullptr)
                walkExpr (*array.sizeExpr, f);

        if (single.initializer != nullptr)
            walkInitializer (*single.initializer, f);
    }
}

template <typename F>
void forEachInitializerExpr (Initializer& init, F&& f)
{
    if (init.expr != nullptr)
        f (*init.expr);

    for (auto& child : init.aggregate)
        forEachInitializerExpr (child, f);
}

/** Calls f on each top-level initializer expression of a declaration. */
template <typename F>
void forEachDeclarationExpr (Declaration& decl, F&& f)
{
    if (decl.initDeclaratorList == nullptr)
        return;

    for (auto& single : decl.initDeclaratorList->declarations)
        if (single.initializer != nullptr)
            forEachInitializerExpr (*single.initializer, f);
}

/** Calls f on each direct child statement of s. */
template <typename F>
void forEachChildStatement (Statement& s, F&& f)
{
    const auto visitPtr = [&f] (std::unique_ptr<Statement>& child)
    {
        if (child != nullptr)
            f (*child);
    };

    std::visit ([&] (auto& alt)
    {
        using T = std::decay_t<decltype (alt)>;

        if constexpr (std::is_same_v<T, StmtSelection>)
        {
            visitPtr (alt.thenBranch);
            visitPtr (alt.elseBranch);
        }
        else if constexpr (std::is_same_v<T, StmtSwitch>)
        {
            for (auto& child : alt.body)
                f (child);

            for (auto& clause : alt.clauses)
                for (auto& child : clause.body)
                    f (child);
        }
        else if constexpr (std::is_same_v<T, StmtWhile> || std::is_same_v<T, StmtDoWhile>)
            visitPtr (alt.body);
        else if constexpr (std::is_same_v<T, StmtFor>)
        {
            visitPtr (alt.init);
            visitPtr (alt.body);
        }
        else if constexpr (std::is_same_v<T, StmtCompound>)
        {
            for (auto& child : alt.statements)
                f (child);
        }
        else if constexpr (std::is_same_v<T, StmtLoop>)
        {
            for (auto& child : alt.body)
                f (child);

            for (auto& child : alt.continuing)
                f (child);
        }
    },
                s.value);
}

/** Calls f on each expression directly owned by s (not by its child statements). */
template <typename F>
void forEachOwnExpr (Statement& s, F&& f)
{
    const auto visitPtr = [&f] (std::unique_ptr<Expr>& child)
    {
        if (child != nullptr)
            f (*child);
    };

    std::visit ([&] (auto& alt)
    {
        using T = std::decay_t<decltype (alt)>;

        if constexpr (std::is_same_v<T, StmtSelection>)
            visitPtr (alt.condition);
        else if constexpr (std::is_same_v<T, StmtSwitch>)
        {
            visitPtr (alt.selector);

            for (auto& clause : alt.clauses)
                for (auto& label : clause.labels)
                    visitPtr (label);
        }
        else if constexpr (std::is_same_v<T, StmtCaseLabel>)
            visitPtr (alt.label);
        else if constexpr (std::is_same_v<T, StmtWhile> || std::is_same_v<T, StmtDoWhile>)
            visitPtr (alt.condition);
        else if constexpr (std::is_same_v<T, StmtFor>)
        {
            visitPtr (alt.condition);
            visitPtr (alt.update);
        }
        else if constexpr (std::is_same_v<T, StmtJump>)
            visitPtr (alt.returnValue);
        else if constexpr (std::is_same_v<T, StmtExpr>)
            visitPtr (alt.expr);
        else if constexpr (std::is_same_v<T, StmtDeclaration>)
            forEachDeclarationExpr (alt.declaration, f);
        else if constexpr (std::is_same_v<T, StmtLoop>)
            visitPtr (alt.breakIf);
    },
                s.value);
}

} // namespace wgsl
} // namespace yup
