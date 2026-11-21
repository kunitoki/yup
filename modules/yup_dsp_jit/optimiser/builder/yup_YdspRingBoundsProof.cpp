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

// Source-level range proofs are made before mutable registers and inlining
// erase the relationship between a state update and its wrap or mask, and
// between a loop and the range of its variable.
class YdspRingBoundsProof
{
    struct Range
    {
        double lower = -std::numeric_limits<double>::infinity();
        double upper = std::numeric_limits<double>::infinity();
        bool integer = false;
    };
    using Environment = std::unordered_map<String, Range>;

    static Range anyInt32()
    {
        return { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), true };
    }

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
        if (expr->kind == YdspExprKind::intLiteral
            && expr->integer >= std::numeric_limits<int32_t>::min() && expr->integer <= std::numeric_limits<int32_t>::max())
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

    /** The mask of `value & mask` (either way round), or -1. */
    static int64_t maskOf (const YdspExpr* expr)
    {
        if (expr == nullptr || expr->kind != YdspExprKind::binary || expr->op != YdspOperator::bitAnd || expr->children.size() != 2)
            return -1;

        for (const auto& side : expr->children)
            if (const auto mask = literal (side.get()); mask.integer && mask.lower >= 0)
                return static_cast<int64_t> (mask.lower);

        return -1;
    }

    /** The limit of `if (name >= limit) { name = 0; }`, or -1. */
    static int64_t wrapLimitOf (const YdspStmt& wrap, const String& name)
    {
        const auto* reset = single (wrap.thenStmt.get());

        if (wrap.kind != YdspStmtKind::ifStmt || wrap.elseStmt != nullptr || wrap.cond == nullptr
            || wrap.cond->kind != YdspExprKind::binary || wrap.cond->op != YdspOperator::ge
            || ! named (wrap.cond->children[0].get(), name)
            || reset == nullptr || reset->kind != YdspStmtKind::assign || ! named (reset->target.get(), name)
            || ! constant (reset->value.get(), 0))
            return -1;

        const auto limit = literal (wrap.cond->children[1].get());
        return limit.integer && limit.lower >= 1 ? static_cast<int64_t> (limit.lower) : -1;
    }

    /** Walks every write to a state and widens `bound` so that [0, bound) holds
        them all. Returns false for a write no rule covers.

        Three writes keep a state inside a range whatever it held before: a
        literal, a masked value (`s = expr & mask`), and an increment that the
        very next statement wraps (`s = s + 1; if (s >= n) { s = 0; }`). Between
        those two statements the state is one past the range, which is why they
        have to be neighbors: nothing else can then observe it.
    */
    static bool boundWrites (const std::vector<YdspStmtPtr>& list, const String& name, int64_t& bound)
    {
        for (size_t i = 0; i < list.size(); ++i)
        {
            bool wrappedByNext = false;

            if (! boundWrites (*list[i], i + 1 < list.size() ? list[i + 1].get() : nullptr, name, bound, wrappedByNext))
                return false;

            if (wrappedByNext)
                ++i;
        }

        return true;
    }

    static bool boundWrites (const YdspStmt& stmt, const YdspStmt* next, const String& name, int64_t& bound, bool& wrappedByNext)
    {
        if ((stmt.kind == YdspStmtKind::localDecl || stmt.kind == YdspStmtKind::forStmt) && stmt.name == name)
            return false;

        if (stmt.kind == YdspStmtKind::assign && named (stmt.target.get(), name))
        {
            const auto value = literal (stmt.value.get());
            const auto mask = maskOf (stmt.value.get());
            const auto limit = next != nullptr ? wrapLimitOf (*next, name) : -1;

            const bool increments = stmt.value != nullptr && stmt.value->kind == YdspExprKind::binary
                                 && stmt.value->op == YdspOperator::add && stmt.value->children.size() == 2
                                 && named (stmt.value->children[0].get(), name) && constant (stmt.value->children[1].get(), 1);

            if (value.integer && value.lower >= 0)
                bound = std::max (bound, static_cast<int64_t> (value.lower) + 1);
            else if (mask >= 0)
                bound = std::max (bound, mask + 1);
            else if (increments && limit > 0)
                bound = std::max (bound, limit);
            else
                return false;

            wrappedByNext = increments && limit > 0 && mask < 0 && ! (value.integer && value.lower >= 0);
            return true;
        }

        if (! boundWrites (stmt.children, name, bound))
            return false;

        // A branch or a loop body that is one statement has no neighbor.
        for (const auto* child : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        {
            bool unused = false;

            if (child != nullptr && ! boundWrites (*child, nullptr, name, bound, unused))
                return false;
        }

        return true;
    }

    /** The names a statement assigns. */
    static void collectAssigned (const YdspStmt& stmt, std::unordered_set<String>& names)
    {
        if (stmt.kind == YdspStmtKind::assign && stmt.target != nullptr && stmt.target->kind == YdspExprKind::identifier)
            names.insert (stmt.target->text);

        for (const auto& child : stmt.children)
            collectAssigned (*child, names);

        for (const auto* child : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
            if (child != nullptr)
                collectAssigned (*child, names);
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
        if (expr.kind == YdspExprKind::binary && args.size() == 2 && expr.op == YdspOperator::bitAnd)
        {
            // Bitwise operators take integers only: one side known to be
            // non-negative bounds the result whatever the other holds.
            const auto limit = std::min (args[0].integer && args[0].lower >= 0 ? args[0].upper : std::numeric_limits<double>::infinity(),
                                         args[1].integer && args[1].lower >= 0 ? args[1].upper : std::numeric_limits<double>::infinity());

            if (std::isfinite (limit))
                return { 0, limit, true };
        }
        if (expr.kind == YdspExprKind::binary && args.size() == 2 && expr.op == YdspOperator::shr
            && args[0].integer && args[0].lower >= 0 && std::isfinite (args[0].upper)
            && args[1].integer && args[1].lower == args[1].upper && args[1].lower >= 0 && args[1].lower < 31)
        {
            const auto shift = std::exp2 (args[1].lower);
            return { std::floor (args[0].lower / shift), std::floor (args[0].upper / shift), true };
        }
        if (expr.kind == YdspExprKind::binary && args.size() == 2 && args[0].integer && args[1].integer)
        {
            Range result;
            if (expr.op == YdspOperator::add)
                result = { args[0].lower + args[1].lower, args[0].upper + args[1].upper, true };
            else if (expr.op == YdspOperator::sub)
                result = { args[0].lower - args[1].upper, args[0].upper - args[1].lower, true };
            else if (expr.op == YdspOperator::mul)
            {
                const auto limits = std::minmax ({ args[0].lower * args[1].lower, args[0].lower * args[1].upper,
                                                  args[0].upper * args[1].lower, args[0].upper * args[1].upper });
                result = { limits.first, limits.second, true };
            }
            // NaN (infinity times zero) fails both comparisons.
            if (result.lower >= std::numeric_limits<int32_t>::min() && result.upper <= std::numeric_limits<int32_t>::max())
            {
                arithmetic.insert (&expr);
                return result;
            }
        }
        if (expr.kind == YdspExprKind::call && args.size() == 2 && (expr.text == "min" || expr.text == "max"))
        {
            const bool integer = args[0].integer && args[1].integer;

            return expr.text == "min" ? Range { std::min (args[0].lower, args[1].lower), std::min (args[0].upper, args[1].upper), integer }
                                      : Range { std::max (args[0].lower, args[1].lower), std::max (args[0].upper, args[1].upper), integer };
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
                // The conversion maps NaN to zero, so zero joins the range.
                if (args[0].lower >= -16777216 && args[0].upper <= 16777216)
                    return { std::min (0.0, std::trunc (args[0].lower)), std::max (0.0, std::trunc (args[0].upper)), true };
                return anyInt32();
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
                    value = anyInt32();
                env[stmt.name] = value;
            }
            else if (stmt.target->kind == YdspExprKind::identifier)
            {
                value.integer = value.integer && env[stmt.target->text].integer;
                env[stmt.target->text] = value;
            }
            return;
        }
        if (stmt.kind == YdspStmtKind::forStmt && stmt.body != nullptr)
        {
            const auto first = stmt.startExpr != nullptr ? evaluate (*stmt.startExpr, env) : Range {};
            const auto last = stmt.endExpr != nullptr ? evaluate (*stmt.endExpr, env) : Range {};

            // What the body assigns differs from one iteration to the next:
            // only a state invariant survives that.
            std::unordered_set<String> assigned;
            collectAssigned (*stmt.body, assigned);

            for (const auto& name : assigned)
            {
                if (const auto invariant = stateRanges.find (name); invariant != stateRanges.end())
                    env[name] = invariant->second;
                else if (const auto known = env.find (name); known != env.end())
                    known->second = known->second.integer ? anyInt32() : Range {};
            }

            const auto outer = env;

            // A body that assigns its own loop variable says nothing about it.
            const bool counts = assigned.count (stmt.name) == 0;

            env[stmt.name] = { counts && first.integer ? first.lower : anyInt32().lower,
                               counts && last.integer && std::isfinite (last.upper) ? last.upper - 1.0 : anyInt32().upper,
                               true };

            statement (*stmt.body, env);
            env = outer;
            return;
        }
        // Other control flow invalidates local facts. State invariants have
        // already been checked independently.
        env = stateRanges;
    }

public:
    explicit YdspRingBoundsProof (const YdspAnalyzedProcessor& processor)
    {
        const auto& decl = *processor.decl;
        if (decl.process == nullptr || ! decl.functions.empty())
            return;
        for (const auto* state : processor.states)
            if (state->arraySize > 0 && state->structName.isEmpty())
                arrays[state->name] = state->arraySize;
        for (const auto* state : processor.states)
        {
            if (state->arraySize != 0 || ! state->structName.isEmpty() || state->type != YdspPrimitiveType::int32Type)
                continue;

            int64_t bound = 0;
            bool valid = state->annotations.empty() && boundWrites (decl.process->body, state->name, bound);

            if (decl.init != nullptr)
                valid = valid && boundWrites (decl.init->body, state->name, bound);

            for (const auto& handler : decl.eventHandlers)
                valid = valid && boundWrites (handler.body, state->name, bound);

            for (const auto& initial : state->initialisers)
            {
                const auto value = literal (initial.get());
                valid &= value.integer && value.lower >= 0 && value.upper < static_cast<double> (bound);
            }

            // Without a tighter one, the type is the invariant: the state is
            // at least known to be an integer.
            stateRanges[state->name] = valid && bound > 0 ? Range { 0, static_cast<double> (bound - 1), true } : anyInt32();
        }
        auto env = stateRanges;
        statements (decl.process->body, env);
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
