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
namespace detail
{

// Source-level induction proofs are made before mutable registers and inlining
// erase the relationship between a state update and its conditional wrap.
class YdspRingBoundsProof
{
    struct Range
    {
        double lower = -std::numeric_limits<double>::infinity();
        double upper = std::numeric_limits<double>::infinity();
        bool integer = false;
    };
    using Environment = std::unordered_map<String, Range>;

    static bool named (const YdspExpr* expr, const String& name)
    {
        return expr != nullptr && expr->kind == YdspExprKind::identifier && expr->text == name;
    }

    static const YdspStmt* single (const YdspStmt* stmt)
    {
        while (stmt != nullptr && stmt->kind == YdspStmtKind::block && stmt->children.size() == 1)
            stmt = stmt->children[0].get();
        return stmt;
    }

    static Range literal (const YdspExpr* expr)
    {
        if (expr == nullptr)
            return {};
        if (expr->kind == YdspExprKind::intLiteral && expr->integer >= -16777216 && expr->integer <= 16777216)
            return { static_cast<double> (expr->integer), static_cast<double> (expr->integer), true };
        if (expr->kind == YdspExprKind::floatLiteral && std::isfinite (expr->number) && std::abs (expr->number) <= 16777216)
        {
            const double rounded = static_cast<float> (expr->number);
            if (rounded == expr->number)
                return { rounded, rounded, false };
        }
        return {};
    }

    static bool constant (const YdspExpr* expr, int value)
    {
        const auto r = literal (expr);
        return r.integer && r.lower == value && r.upper == value;
    }

    bool validWrites (const YdspStmt& stmt, const String& name, int bound,
                      const std::unordered_set<const YdspStmt*>& updates) const
    {
        if (stmt.kind == YdspStmtKind::localDecl && stmt.name == name)
            return false;
        if (stmt.kind == YdspStmtKind::assign && named (stmt.target.get(), name) && updates.count (&stmt) == 0)
        {
            const auto value = literal (stmt.value.get());
            if (! value.integer || value.lower < 0 || value.upper >= bound)
                return false;
        }
        for (const auto& child : stmt.children)
            if (! validWrites (*child, name, bound, updates))
                return false;
        for (const auto* child : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
            if (child != nullptr && ! validWrites (*child, name, bound, updates))
                return false;
        return true;
    }

    Range evaluate (const YdspExpr& expr, const Environment& env)
    {
        std::vector<Range> args;
        for (const auto& child : expr.children)
            args.push_back (evaluate (*child, env));
        if (expr.kind == YdspExprKind::identifier)
        {
            const auto found = env.find (expr.text);
            return found == env.end() ? Range {} : found->second;
        }
        if (expr.kind == YdspExprKind::intLiteral || expr.kind == YdspExprKind::floatLiteral)
            return literal (&expr);
        if (expr.kind == YdspExprKind::index && args.size() == 2 && expr.children[0]->kind == YdspExprKind::identifier)
        {
            const auto array = arrays.find (expr.children[0]->text);
            if (array != arrays.end() && args[1].integer && args[1].lower >= 0 && args[1].upper < array->second)
                accesses.insert (&expr);
        }
        if (expr.kind == YdspExprKind::binary && args.size() == 2 && args[0].integer && args[1].integer)
        {
            Range result;
            if (expr.op == YdspOperator::add)
                result = { args[0].lower + args[1].lower, args[0].upper + args[1].upper, true };
            else if (expr.op == YdspOperator::sub)
                result = { args[0].lower - args[1].upper, args[0].upper - args[1].lower, true };
            if (result.lower >= std::numeric_limits<int32_t>::min() && result.upper <= std::numeric_limits<int32_t>::max())
            {
                arithmetic.insert (&expr);
                return result;
            }
        }
        if (expr.kind == YdspExprKind::call && expr.text == "clamp" && args.size() == 3
            && args[1].lower == args[1].upper && args[2].lower == args[2].upper
            && args[1].lower <= args[2].lower)
            return { args[1].lower, args[2].upper, args[0].integer && args[1].integer && args[2].integer };
        if (expr.kind == YdspExprKind::call && args.size() == 1)
        {
            if (expr.text == "int" || expr.text == "int32")
            {
                if (args[0].integer)
                    return args[0];
                // The finite portion is bounded. NaN survives ordinary float
                // clamps, so lowering explicitly maps it to zero before ftoi.
                if (args[0].lower >= 0 && args[0].upper <= 16777216)
                {
                    // Intrinsic clamps still use the general saturating conversion,
                    // which already maps a possible NaN to zero.
                    if (expr.children[0]->kind != YdspExprKind::call || expr.children[0]->text != "clamp")
                        casts.insert (&expr);
                    return { 0, std::trunc (args[0].upper), true };
                }
                return { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), true };
            }
            if ((expr.text == "float" || expr.text == "float32") && args[0].lower >= -16777216 && args[0].upper <= 16777216)
                return { args[0].lower, args[0].upper, false };
        }
        return {};
    }

    static void refine (Environment& env, const YdspExpr& cond, bool taken)
    {
        if (cond.kind != YdspExprKind::binary || cond.children.size() != 2
            || cond.children[0]->kind != YdspExprKind::identifier)
            return;
        auto found = env.find (cond.children[0]->text);
        const auto limit = literal (cond.children[1].get());
        if (found == env.end() || limit.lower != limit.upper)
            return;
        auto& range = found->second;
        if (range.integer && ! limit.integer)
            return;
        const auto step = range.integer ? 1.0 : 0.0;
        switch (cond.op)
        {
            case YdspOperator::lt:
                if (taken) range.upper = std::min (range.upper, limit.upper - step);
                else range.lower = std::max (range.lower, limit.lower);
                break;
            case YdspOperator::ge:
                if (taken) range.lower = std::max (range.lower, limit.lower);
                else range.upper = std::min (range.upper, limit.upper - step);
                break;
            case YdspOperator::gt:
                if (taken) range.lower = std::max (range.lower, limit.lower + step);
                else range.upper = std::min (range.upper, limit.upper);
                break;
            default:
                break;
        }
        if (range.lower > range.upper)
            range = {};
    }

    void statements (const std::vector<YdspStmtPtr>& body, Environment& env)
    {
        for (const auto& stmt : body)
            statement (*stmt, env);
    }

    void statement (const YdspStmt& stmt, Environment& env)
    {
        if (stmt.kind == YdspStmtKind::block)
        {
            const auto outer = env;
            statements (stmt.children, env);
            std::erase_if (env, [&] (const auto& entry) { return outer.count (entry.first) == 0; });
            return;
        }
        if (stmt.kind == YdspStmtKind::ifStmt)
        {
            evaluate (*stmt.cond, env);
            auto yes = env, no = env;
            refine (yes, *stmt.cond, true);
            refine (no, *stmt.cond, false);
            if (stmt.thenStmt != nullptr) statement (*stmt.thenStmt, yes);
            if (stmt.elseStmt != nullptr) statement (*stmt.elseStmt, no);
            for (auto& [name, range] : env)
            {
                const auto a = yes[name], b = no[name];
                range = { std::min (a.lower, b.lower), std::max (a.upper, b.upper), a.integer && b.integer };
            }
            return;
        }
        if (stmt.kind == YdspStmtKind::localDecl || stmt.kind == YdspStmtKind::assign)
        {
            auto value = stmt.value != nullptr ? evaluate (*stmt.value, env) : Range {};
            if (stmt.target != nullptr)
                evaluate (*stmt.target, env);
            if (stmt.kind == YdspStmtKind::localDecl)
            {
                if (stmt.hasDeclType && stmt.declType != YdspPrimitiveType::int32Type && stmt.declType != YdspPrimitiveType::float32Type)
                    value = {};
                if (stmt.hasDeclType && stmt.declType == YdspPrimitiveType::float32Type)
                    value.integer = false;
                if (stmt.hasDeclType && stmt.declType == YdspPrimitiveType::int32Type && ! value.integer)
                    value = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), true };
                env[stmt.name] = value;
            }
            else if (stmt.target->kind == YdspExprKind::identifier)
            {
                value.integer = value.integer && env[stmt.target->text].integer;
                env[stmt.target->text] = value;
            }
            return;
        }
        // Explicit loops and other unsupported control flow invalidate local
        // facts. State invariants have already been checked independently.
        env = stateRanges;
    }

public:
    explicit YdspRingBoundsProof (const YdspAnalyzedProcessor& processor)
    {
        const auto& decl = *processor.decl;
        if (processor.mode != YdspProcessMode::sample || decl.process == nullptr || ! decl.functions.empty())
            return;
        for (const auto* state : processor.states)
            if (state->arraySize > 0 && state->structName.isEmpty())
                arrays[state->name] = state->arraySize;
        for (const auto* state : processor.states)
        {
            if (state->arraySize != 0 || ! state->structName.isEmpty() || state->type != YdspPrimitiveType::int32Type || ! state->annotations.empty())
                continue;
            int bound = 0;
            std::unordered_set<const YdspStmt*> updates;
            const auto& body = decl.process->body;
            for (size_t i = 0; i + 1 < body.size(); ++i)
            {
                const auto& increment = *body[i];
                const auto& wrap = *body[i + 1];
                const auto* reset = single (wrap.thenStmt.get());
                if (increment.kind != YdspStmtKind::assign || ! named (increment.target.get(), state->name)
                    || increment.value->kind != YdspExprKind::binary || increment.value->op != YdspOperator::add
                    || ! named (increment.value->children[0].get(), state->name) || ! constant (increment.value->children[1].get(), 1)
                    || wrap.kind != YdspStmtKind::ifStmt || wrap.elseStmt != nullptr || wrap.cond == nullptr
                    || wrap.cond->kind != YdspExprKind::binary || wrap.cond->op != YdspOperator::ge
                    || ! named (wrap.cond->children[0].get(), state->name)
                    || reset == nullptr || reset->kind != YdspStmtKind::assign || ! named (reset->target.get(), state->name)
                    || ! constant (reset->value.get(), 0))
                    continue;
                const auto limit = literal (wrap.cond->children[1].get());
                if (! limit.integer || limit.lower < 1 || limit.lower != limit.upper || (bound != 0 && bound != limit.lower))
                    continue;
                bound = static_cast<int> (limit.lower);
                updates.insert (&increment);
            }
            if (bound == 0)
                continue;
            bool valid = true;
            for (const auto& initial : state->initialisers)
            {
                const auto value = literal (initial.get());
                valid &= value.integer && value.lower >= 0 && value.upper < bound;
            }
            for (const auto& stmt : body)
                valid &= validWrites (*stmt, state->name, bound, updates);
            const std::unordered_set<const YdspStmt*> noUpdates;
            if (decl.init != nullptr)
                for (const auto& stmt : decl.init->body)
                    valid &= validWrites (*stmt, state->name, bound, noUpdates);
            for (const auto& handler : decl.eventHandlers)
                for (const auto& stmt : handler.body)
                    valid &= validWrites (*stmt, state->name, bound, noUpdates);
            if (valid)
                stateRanges[state->name] = { 0, static_cast<double> (bound - 1), true };
        }
        if (! stateRanges.empty())
        {
            auto env = stateRanges;
            statements (decl.process->body, env);
        }
    }

    std::unordered_set<const YdspExpr*> accesses;
    std::unordered_set<const YdspExpr*> casts;
    std::unordered_set<const YdspExpr*> arithmetic;

private:
    Environment stateRanges;
    std::unordered_map<String, int> arrays;
};

} // namespace detail
} // namespace yup
