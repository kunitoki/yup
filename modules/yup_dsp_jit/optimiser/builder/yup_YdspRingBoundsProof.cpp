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
        bool integer = false;
        bool wide = false;
        bool adaptable = false;
        int64_t iLower = std::numeric_limits<int64_t>::min();
        int64_t iUpper = std::numeric_limits<int64_t>::max();
        double fLower = -std::numeric_limits<double>::infinity();
        double fUpper = std::numeric_limits<double>::infinity();
    };
    using Environment = std::unordered_map<String, Range>;

    static Range intRange (int64_t lo, int64_t hi, bool wide = false, bool adaptable = false)
    {
        return { true, wide, adaptable, lo, hi, 0.0, 0.0 };
    }

    static Range floatRange (double lo, double hi, bool wide = false, bool adaptable = false)
    {
        return { false, wide, adaptable, 0, 0, lo, hi };
    }

    static Range anyInt32()
    {
        return intRange (std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max(), false);
    }

    static Range anyInt64()
    {
        return intRange (std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max(), true);
    }

    static Range anyFloat (bool wide = false)
    {
        return floatRange (-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), wide);
    }

    static double floatLower (const Range& r)
    {
        if (! r.integer)
            return r.fLower;

        if (r.iLower >= std::numeric_limits<int32_t>::min() && r.iUpper <= std::numeric_limits<int32_t>::max())
        {
            const double exact = static_cast<double> (r.iLower);
            const double narrow = static_cast<double> (static_cast<float> (r.iLower));
            return std::min (exact, narrow);
        }

        return -std::numeric_limits<double>::infinity();
    }

    static double floatUpper (const Range& r)
    {
        if (! r.integer)
            return r.fUpper;

        if (r.iLower >= std::numeric_limits<int32_t>::min() && r.iUpper <= std::numeric_limits<int32_t>::max())
        {
            const double exact = static_cast<double> (r.iUpper);
            const double narrow = static_cast<double> (static_cast<float> (r.iUpper));
            return std::max (exact, narrow);
        }

        return std::numeric_limits<double>::infinity();
    }

    static Range mergeRanges (const Range& a, const Range& b)
    {
        if (a.integer && b.integer)
            return intRange (std::min (a.iLower, b.iLower), std::max (a.iUpper, b.iUpper), a.wide || b.wide);

        const bool isWide = (! a.integer && a.wide) || (! b.integer && b.wide);
        return floatRange (std::min (floatLower (a), floatLower (b)),
                           std::max (floatUpper (a), floatUpper (b)),
                           isWide);
    }

    static bool checkedAdd (int64_t a, int64_t b, int64_t& out) noexcept
    {
        if ((b > 0 && a > std::numeric_limits<int64_t>::max() - b)
            || (b < 0 && a < std::numeric_limits<int64_t>::min() - b))
            return false;

        out = a + b;
        return true;
    }

    static bool checkedSub (int64_t a, int64_t b, int64_t& out) noexcept
    {
        if ((b < 0 && a > std::numeric_limits<int64_t>::max() + b)
            || (b > 0 && a < std::numeric_limits<int64_t>::min() + b))
            return false;

        out = a - b;
        return true;
    }

    static bool checkedMul (int64_t a, int64_t b, int64_t& out) noexcept
    {
        if (a == 0 || b == 0)
        {
            out = 0;
            return true;
        }

        const bool negative = (a < 0) != (b < 0);
        const uint64_t ua = a < 0 ? (uint64_t { 0 } - static_cast<uint64_t> (a)) : static_cast<uint64_t> (a);
        const uint64_t ub = b < 0 ? (uint64_t { 0 } - static_cast<uint64_t> (b)) : static_cast<uint64_t> (b);
        const uint64_t limit = negative ? (uint64_t { 1 } << 63) : static_cast<uint64_t> (std::numeric_limits<int64_t>::max());

        if (ua > limit / ub)
            return false;

        const uint64_t prod = ua * ub;
        out = negative ? static_cast<int64_t> (uint64_t { 0 } - prod) : static_cast<int64_t> (prod);
        return true;
    }

    static int64_t saturatingNeg (int64_t v, bool wide) noexcept
    {
        if (wide)
            return v == std::numeric_limits<int64_t>::min() ? std::numeric_limits<int64_t>::max() : -v;

        return v <= std::numeric_limits<int32_t>::min() ? std::numeric_limits<int32_t>::max() : -v;
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

        if (expr->kind == YdspExprKind::unary && expr->op == YdspOperator::neg && expr->children.size() == 1)
        {
            const auto* child = expr->children[0].get();
            if (child == nullptr)
                return {};

            if (child->kind == YdspExprKind::intLiteral && child->integer != std::numeric_limits<int64_t>::min())
            {
                const int64_t val = -child->integer;
                const bool isWide = val < std::numeric_limits<int32_t>::min() || val > std::numeric_limits<int32_t>::max();
                return intRange (val, val, isWide, true);
            }

            if (child->kind == YdspExprKind::floatLiteral && std::isfinite (child->number) && std::abs (child->number) <= 2147483647.0)
            {
                const double val = -child->number;
                const double rounded = static_cast<double> (static_cast<float> (val));
                return floatRange (std::min (val, rounded), std::max (val, rounded), false, true);
            }

            return {};
        }

        if (expr->kind == YdspExprKind::intLiteral)
        {
            const bool isWide = expr->integer < std::numeric_limits<int32_t>::min() || expr->integer > std::numeric_limits<int32_t>::max();
            return intRange (expr->integer, expr->integer, isWide, true);
        }

        if (expr->kind == YdspExprKind::floatLiteral && std::isfinite (expr->number) && std::abs (expr->number) <= 2147483647.0)
        {
            const double rounded = static_cast<double> (static_cast<float> (expr->number));
            return floatRange (std::min (expr->number, rounded), std::max (expr->number, rounded), false, true);
        }

        return {};
    }

    static bool constant (const YdspExpr* expr, int value)
    {
        const auto r = literal (expr);
        return r.integer && r.iLower == value && r.iUpper == value;
    }

    /** The mask of `value & mask` (either way round), or -1. */
    static int64_t maskOf (const YdspExpr* expr)
    {
        if (expr == nullptr || expr->kind != YdspExprKind::binary || expr->op != YdspOperator::bitAnd || expr->children.size() != 2)
            return -1;

        for (const auto& side : expr->children)
            if (const auto mask = literal (side.get()); mask.integer && mask.iLower >= 0)
                return mask.iLower;

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
        return limit.integer && limit.iLower >= 1 ? limit.iLower : -1;
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

            if (value.integer && value.iLower >= 0 && value.iUpper < std::numeric_limits<int64_t>::max())
                bound = std::max (bound, value.iUpper + 1);
            else if (mask >= 0 && mask < std::numeric_limits<int64_t>::max())
                bound = std::max (bound, mask + 1);
            else if (increments && limit > 0)
                bound = std::max (bound, limit);
            else
                return false;

            wrappedByNext = increments && limit > 0 && mask < 0 && ! (value.integer && value.iLower >= 0);
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
        if (expr.kind == YdspExprKind::ternary && expr.children.size() == 3)
        {
            evaluate (*expr.children[0], env);
            auto yes = env, no = env;
            refine (yes, *expr.children[0], true);
            refine (no, *expr.children[0], false);
            return mergeRanges (evaluate (*expr.children[1], yes), evaluate (*expr.children[2], no));
        }
        if (expr.kind == YdspExprKind::call && expr.text == "select" && expr.children.size() == 3)
        {
            evaluate (*expr.children[0], env);
            auto yes = env, no = env;
            refine (yes, *expr.children[0], true);
            refine (no, *expr.children[0], false);
            return mergeRanges (evaluate (*expr.children[1], yes), evaluate (*expr.children[2], no));
        }

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
        if (expr.kind == YdspExprKind::unary && expr.op == YdspOperator::neg && args.size() == 1)
        {
            if (const auto lit = literal (&expr); lit.adaptable)
                return lit;
            if (args[0].integer)
                return intRange (saturatingNeg (args[0].iUpper, args[0].wide),
                                 saturatingNeg (args[0].iLower, args[0].wide),
                                 args[0].wide);
            return floatRange (-args[0].fUpper, -args[0].fLower, args[0].wide);
        }
        if (expr.kind == YdspExprKind::index && args.size() == 2 && expr.children[0]->kind == YdspExprKind::identifier)
        {
            const auto array = arrays.find (expr.children[0]->text);
            if (array != arrays.end() && args[1].integer && args[1].iLower >= 0 && args[1].iUpper < array->second)
                accesses.insert (&expr);

            const auto elemRange = arrayElementRanges.find (expr.children[0]->text);
            return elemRange != arrayElementRanges.end() ? elemRange->second : Range {};
        }
        if (expr.kind == YdspExprKind::binary && args.size() == 2)
        {
            if (expr.op == YdspOperator::bitAnd)
            {
                // Bitwise operators take integers only: one side known to be
                // non-negative bounds the result whatever the other holds.
                const bool isWide = args[0].wide || args[1].wide;
                const bool lhsNonNeg = args[0].integer && args[0].iLower >= 0;
                const bool rhsNonNeg = args[1].integer && args[1].iLower >= 0;
                if (lhsNonNeg || rhsNonNeg)
                {
                    int64_t limit = isWide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
                    if (lhsNonNeg)
                        limit = std::min (limit, args[0].iUpper);
                    if (rhsNonNeg)
                        limit = std::min (limit, args[1].iUpper);
                    return intRange (0, limit, isWide);
                }
            }
            if (expr.op == YdspOperator::shr
                && args[0].integer && args[1].integer && args[1].iLower == args[1].iUpper)
            {
                const bool isWide = args[0].wide || args[1].wide;
                const int maxShift = isWide ? 63 : 31;
                if (args[1].iLower >= 0 && args[1].iLower <= maxShift)
                {
                    const int shift = static_cast<int> (args[1].iLower);
                    return intRange (args[0].iLower >> shift, args[0].iUpper >> shift, isWide);
                }
            }
            if (expr.op == YdspOperator::shl
                && args[0].integer && args[0].iLower >= 0 && args[1].integer && args[1].iLower == args[1].iUpper)
            {
                const bool isWide = args[0].wide || args[1].wide;
                const int maxShift = isWide ? 62 : 30;
                if (args[1].iLower >= 0 && args[1].iLower <= maxShift)
                {
                    const int shift = static_cast<int> (args[1].iLower);
                    const int64_t typeMax = isWide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
                    if (args[0].iUpper <= (typeMax >> shift))
                        return intRange (args[0].iLower << shift, args[0].iUpper << shift, isWide);
                }
            }
            if (args[0].integer && args[1].integer)
            {
                const bool isWide = args[0].wide || args[1].wide;
                const int64_t typeMin = isWide ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min();
                const int64_t typeMax = isWide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();

                if (expr.op == YdspOperator::add || expr.op == YdspOperator::sub || expr.op == YdspOperator::mul)
                {
                    int64_t resLo = 0, resHi = 0;
                    bool ok = false;

                    if (expr.op == YdspOperator::add)
                        ok = checkedAdd (args[0].iLower, args[1].iLower, resLo)
                          && checkedAdd (args[0].iUpper, args[1].iUpper, resHi);
                    else if (expr.op == YdspOperator::sub)
                        ok = checkedSub (args[0].iLower, args[1].iUpper, resLo)
                          && checkedSub (args[0].iUpper, args[1].iLower, resHi);
                    else
                    {
                        int64_t c00 = 0, c01 = 0, c10 = 0, c11 = 0;
                        ok = checkedMul (args[0].iLower, args[1].iLower, c00)
                          && checkedMul (args[0].iLower, args[1].iUpper, c01)
                          && checkedMul (args[0].iUpper, args[1].iLower, c10)
                          && checkedMul (args[0].iUpper, args[1].iUpper, c11);
                        if (ok)
                        {
                            const auto limits = std::minmax ({ c00, c01, c10, c11 });
                            resLo = limits.first;
                            resHi = limits.second;
                        }
                    }

                    if (ok && resLo >= typeMin && resHi <= typeMax)
                    {
                        arithmetic.insert (&expr);
                        return intRange (resLo, resHi, isWide);
                    }

                    return isWide ? anyInt64() : anyInt32();
                }

                if (expr.op == YdspOperator::mod && args[0].iLower >= 0 && args[1].iLower > 0)
                    return intRange (0, args[1].iUpper - 1, isWide);

                return isWide ? anyInt64() : anyInt32();
            }

            if (expr.op == YdspOperator::add || expr.op == YdspOperator::sub
                || expr.op == YdspOperator::mul || expr.op == YdspOperator::div)
            {
                const bool isWide = (! args[0].integer && args[0].wide) || (! args[1].integer && args[1].wide);
                const double aLo = floatLower (args[0]);
                const double aHi = floatUpper (args[0]);
                const double bLo = floatLower (args[1]);
                const double bHi = floatUpper (args[1]);

                if (std::isfinite (aLo) && std::isfinite (aHi) && std::isfinite (bLo) && std::isfinite (bHi))
                {
                    const auto widen = [isWide] (double lo, double hi) -> Range
                    {
                        if (! isWide)
                        {
                            lo = std::min (lo, static_cast<double> (static_cast<float> (lo)));
                            hi = std::max (hi, static_cast<double> (static_cast<float> (hi)));
                        }
                        return std::isfinite (lo) && std::isfinite (hi) ? floatRange (lo, hi, isWide) : anyFloat (isWide);
                    };

                    if (expr.op == YdspOperator::add)
                        return widen (aLo + bLo, aHi + bHi);
                    if (expr.op == YdspOperator::sub)
                        return widen (aLo - bHi, aHi - bLo);
                    if (expr.op == YdspOperator::mul)
                    {
                        const auto limits = std::minmax ({ aLo * bLo, aLo * bHi, aHi * bLo, aHi * bHi });
                        return widen (limits.first, limits.second);
                    }
                    if (expr.op == YdspOperator::div && (bLo > 0.0 || bHi < 0.0))
                    {
                        const auto limits = std::minmax ({ aLo / bLo, aLo / bHi, aHi / bLo, aHi / bHi });
                        return widen (limits.first, limits.second);
                    }
                }

                return anyFloat (isWide);
            }
        }
        if (expr.kind == YdspExprKind::call && args.size() == 2 && (expr.text == "min" || expr.text == "max"))
        {
            if (args[0].integer && args[1].integer)
            {
                const bool isWide = args[0].wide || args[1].wide;
                return expr.text == "min"
                           ? intRange (std::min (args[0].iLower, args[1].iLower), std::min (args[0].iUpper, args[1].iUpper), isWide)
                           : intRange (std::max (args[0].iLower, args[1].iLower), std::max (args[0].iUpper, args[1].iUpper), isWide);
            }

            const bool isWide = (! args[0].integer && args[0].wide) || (! args[1].integer && args[1].wide);
            const double aLo = floatLower (args[0]);
            const double aHi = floatUpper (args[0]);
            const double bLo = floatLower (args[1]);
            const double bHi = floatUpper (args[1]);

            return expr.text == "min"
                       ? floatRange (std::min (aLo, bLo), std::min (aHi, bHi), isWide)
                       : floatRange (std::max (aLo, bLo), std::max (aHi, bHi), isWide);
        }
        if (expr.kind == YdspExprKind::call && expr.text == "clamp" && args.size() == 3)
        {
            if (args[0].integer && args[1].integer && args[2].integer)
            {
                const bool isWide = args[0].wide || args[1].wide || args[2].wide;
                return intRange (std::min (std::max (args[0].iLower, args[1].iLower), args[2].iLower),
                                 std::min (std::max (args[0].iUpper, args[1].iUpper), args[2].iUpper),
                                 isWide);
            }

            const bool isWide = (! args[0].integer && args[0].wide)
                             || (! args[1].integer && args[1].wide)
                             || (! args[2].integer && args[2].wide);
            const double aLo = floatLower (args[0]);
            const double aHi = floatUpper (args[0]);
            const double bLo = floatLower (args[1]);
            const double bHi = floatUpper (args[1]);
            const double cLo = floatLower (args[2]);
            const double cHi = floatUpper (args[2]);

            return floatRange (std::min (std::max (aLo, bLo), cLo),
                               std::min (std::max (aHi, bHi), cHi),
                               isWide);
        }
        if (expr.kind == YdspExprKind::call && args.size() == 1)
        {
            if (expr.text == "floor" || expr.text == "ceil" || expr.text == "round" || expr.text == "rint")
            {
                const bool isWide = ! args[0].integer && args[0].wide;
                const double lo = floatLower (args[0]);
                const double hi = floatUpper (args[0]);

                if (expr.text == "floor")
                    return floatRange (std::floor (lo), std::floor (hi), isWide);
                if (expr.text == "ceil")
                    return floatRange (std::ceil (lo), std::ceil (hi), isWide);
                if (expr.text == "round")
                    return floatRange (std::round (lo), std::round (hi), isWide);

                return floatRange (std::floor (lo), std::ceil (hi), isWide);
            }
            if (expr.text == "abs")
            {
                if (args[0].integer)
                {
                    if (args[0].iLower >= 0)
                        return args[0];
                    if (args[0].iUpper <= 0)
                        return intRange (saturatingNeg (args[0].iUpper, args[0].wide),
                                         saturatingNeg (args[0].iLower, args[0].wide),
                                         args[0].wide);
                    return intRange (0, std::max (saturatingNeg (args[0].iLower, args[0].wide), args[0].iUpper), args[0].wide);
                }

                if (args[0].fLower >= 0.0)
                    return args[0];
                if (args[0].fUpper <= 0.0)
                    return floatRange (-args[0].fUpper, -args[0].fLower, args[0].wide);
                return floatRange (0.0, std::max (-args[0].fLower, args[0].fUpper), args[0].wide);
            }
            if (expr.text == "sqrt")
            {
                const bool isWide = ! args[0].integer && args[0].wide;
                const double lo = floatLower (args[0]);
                const double hi = floatUpper (args[0]);
                if (lo >= 0.0)
                {
                    double rLo = std::sqrt (lo);
                    double rHi = std::sqrt (hi);
                    if (! isWide)
                    {
                        rLo = std::min (rLo, static_cast<double> (std::sqrt (static_cast<float> (lo))));
                        rHi = std::max (rHi, static_cast<double> (std::sqrt (static_cast<float> (hi))));
                    }
                    return floatRange (rLo, rHi, isWide);
                }
                return anyFloat (isWide);
            }
            if (expr.text == "sin" || expr.text == "cos")
                return floatRange (-1.0001, 1.0001, ! args[0].integer && args[0].wide);
            if (expr.text == "tanh")
                return floatRange (-1.0, 1.0, ! args[0].integer && args[0].wide);

            if (expr.text == "int" || expr.text == "int32")
            {
                if (args[0].integer)
                {
                    if (args[0].iLower >= std::numeric_limits<int32_t>::min()
                        && args[0].iUpper <= std::numeric_limits<int32_t>::max())
                        return intRange (args[0].iLower, args[0].iUpper, false);
                    return anyInt32();
                }

                const double maxSafe = args[0].wide ? 2147483647.0 : 2147483520.0;
                const double minSafe = -2147483648.0;

                // Lowering emits a non-negative NaN guard (selectB (value >= 0.0, value, 0.0))
                // when the expression is placed in `casts`, so only non-negative ranges
                // can safely use `boundedFloatToInt`.
                if (args[0].fLower >= 0.0 && args[0].fUpper <= maxSafe)
                {
                    casts.insert (&expr);
                    return intRange (0, static_cast<int64_t> (std::trunc (args[0].fUpper)), false);
                }

                // The conversion maps NaN to zero, so zero joins the range.
                if (args[0].fLower >= minSafe && args[0].fUpper <= maxSafe)
                {
                    const int64_t lo = static_cast<int64_t> (std::trunc (args[0].fLower));
                    const int64_t hi = static_cast<int64_t> (std::trunc (args[0].fUpper));
                    return intRange (std::min<int64_t> (0, lo), std::max<int64_t> (0, hi), false);
                }

                return anyInt32();
            }
            if (expr.text == "int64")
            {
                if (args[0].integer)
                    return intRange (args[0].iLower, args[0].iUpper, true);

                if (args[0].fLower >= -4503599627370496.0 && args[0].fUpper <= 4503599627370496.0)
                {
                    const int64_t lo = static_cast<int64_t> (std::trunc (args[0].fLower));
                    const int64_t hi = static_cast<int64_t> (std::trunc (args[0].fUpper));
                    return intRange (std::min<int64_t> (0, lo), std::max<int64_t> (0, hi), true);
                }

                return anyInt64();
            }
            if (expr.text == "float" || expr.text == "float32")
            {
                const double lo = floatLower (args[0]);
                const double hi = floatUpper (args[0]);
                if (std::isfinite (lo) && std::isfinite (hi))
                {
                    const double lo32 = static_cast<double> (static_cast<float> (lo));
                    const double hi32 = static_cast<double> (static_cast<float> (hi));
                    return floatRange (std::min (lo, lo32), std::max (hi, hi32), false);
                }
                return anyFloat (false);
            }
            if (expr.text == "float64")
            {
                if (args[0].integer)
                {
                    if (args[0].iLower >= std::numeric_limits<int32_t>::min()
                        && args[0].iUpper <= std::numeric_limits<int32_t>::max())
                        return floatRange (static_cast<double> (args[0].iLower), static_cast<double> (args[0].iUpper), true);
                    return anyFloat (true);
                }
                return floatRange (args[0].fLower, args[0].fUpper, true);
            }
        }
        return {};
    }

    static void refine (Environment& env, const YdspExpr& cond, bool taken)
    {
        if (cond.kind == YdspExprKind::unary && cond.op == YdspOperator::notL && cond.children.size() == 1)
        {
            refine (env, *cond.children[0], ! taken);
            return;
        }
        if (cond.kind == YdspExprKind::binary && cond.children.size() == 2)
        {
            if (taken && cond.op == YdspOperator::andL)
            {
                refine (env, *cond.children[0], true);
                refine (env, *cond.children[1], true);
                return;
            }
            if (! taken && cond.op == YdspOperator::orL)
            {
                refine (env, *cond.children[0], false);
                refine (env, *cond.children[1], false);
                return;
            }
        }
        if (cond.kind != YdspExprKind::binary || cond.children.size() != 2)
            return;

        const YdspExpr* target = nullptr;
        const YdspExpr* limitExpr = nullptr;
        auto op = cond.op;

        if (cond.children[0]->kind == YdspExprKind::identifier)
        {
            target = cond.children[0].get();
            limitExpr = cond.children[1].get();
        }
        else if (cond.children[1]->kind == YdspExprKind::identifier)
        {
            target = cond.children[1].get();
            limitExpr = cond.children[0].get();
            switch (op)
            {
                case YdspOperator::lt: op = YdspOperator::gt; break;
                case YdspOperator::le: op = YdspOperator::ge; break;
                case YdspOperator::gt: op = YdspOperator::lt; break;
                case YdspOperator::ge: op = YdspOperator::le; break;
                default: return;
            }
        }
        else
        {
            return;
        }

        auto found = env.find (target->text);
        const auto limit = literal (limitExpr);
        if (found == env.end() || ! limit.adaptable)
            return;

        auto& range = found->second;
        if (range.integer)
        {
            if (! limit.integer)
                return;

            const int64_t typeMin = range.wide ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int32_t>::min();
            const int64_t typeMax = range.wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max();
            switch (op)
            {
                case YdspOperator::lt:
                    if (taken)
                    {
                        if (limit.iUpper > std::numeric_limits<int64_t>::min())
                            range.iUpper = std::min (range.iUpper, limit.iUpper - 1);
                    }
                    else
                    {
                        range.iLower = std::max (range.iLower, limit.iLower);
                    }
                    break;
                case YdspOperator::le:
                    if (taken)
                    {
                        range.iUpper = std::min (range.iUpper, limit.iUpper);
                    }
                    else if (limit.iLower < std::numeric_limits<int64_t>::max())
                    {
                        range.iLower = std::max (range.iLower, limit.iLower + 1);
                    }
                    break;
                case YdspOperator::ge:
                    if (taken)
                    {
                        range.iLower = std::max (range.iLower, limit.iLower);
                    }
                    else if (limit.iUpper > std::numeric_limits<int64_t>::min())
                    {
                        range.iUpper = std::min (range.iUpper, limit.iUpper - 1);
                    }
                    break;
                case YdspOperator::gt:
                    if (taken)
                    {
                        if (limit.iLower < std::numeric_limits<int64_t>::max())
                            range.iLower = std::max (range.iLower, limit.iLower + 1);
                    }
                    else
                    {
                        range.iUpper = std::min (range.iUpper, limit.iUpper);
                    }
                    break;
                default:
                    break;
            }

            range.iLower = std::max (range.iLower, typeMin);
            range.iUpper = std::min (range.iUpper, typeMax);
            if (range.iLower > range.iUpper)
                range = range.wide ? anyInt64() : anyInt32();
            return;
        }

        const double limitLo = floatLower (limit);
        const double limitHi = floatUpper (limit);
        switch (op)
        {
            case YdspOperator::lt:
            case YdspOperator::le:
                if (taken)
                    range.fUpper = std::min (range.fUpper, limitHi);
                else
                    range.fLower = std::max (range.fLower, limitLo);
                break;
            case YdspOperator::ge:
            case YdspOperator::gt:
                if (taken)
                    range.fLower = std::max (range.fLower, limitLo);
                else
                    range.fUpper = std::min (range.fUpper, limitHi);
                break;
            default:
                break;
        }
        if (range.fLower > range.fUpper)
            range = anyFloat (range.wide);
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
                range = mergeRanges (yes[name], no[name]);
            return;
        }
        if (stmt.kind == YdspStmtKind::localDecl || stmt.kind == YdspStmtKind::assign)
        {
            auto value = stmt.value != nullptr ? evaluate (*stmt.value, env) : Range {};
            if (stmt.target != nullptr)
                evaluate (*stmt.target, env);
            if (stmt.kind == YdspStmtKind::localDecl)
            {
                if (stmt.hasDeclType)
                {
                    switch (stmt.declType)
                    {
                        case YdspPrimitiveType::int32Type:
                            if (! value.integer
                                || value.iLower < std::numeric_limits<int32_t>::min()
                                || value.iUpper > std::numeric_limits<int32_t>::max())
                                value = anyInt32();
                            else
                                value = intRange (value.iLower, value.iUpper, false);
                            break;

                        case YdspPrimitiveType::int64Type:
                            value = value.integer ? intRange (value.iLower, value.iUpper, true) : anyInt64();
                            break;

                        case YdspPrimitiveType::float32Type:
                            value = floatRange (floatLower (value), floatUpper (value), false);
                            break;

                        case YdspPrimitiveType::float64Type:
                            if (value.integer)
                            {
                                if (value.iLower >= std::numeric_limits<int32_t>::min()
                                    && value.iUpper <= std::numeric_limits<int32_t>::max())
                                    value = floatRange (static_cast<double> (value.iLower), static_cast<double> (value.iUpper), true);
                                else
                                    value = anyFloat (true);
                            }
                            else
                            {
                                value = floatRange (value.fLower, value.fUpper, true);
                            }
                            break;

                        case YdspPrimitiveType::boolType:
                            value = {};
                            break;
                    }
                }
                else
                {
                    value.adaptable = false;
                }
                env[stmt.name] = value;
            }
            else if (stmt.target != nullptr && stmt.target->kind == YdspExprKind::identifier)
            {
                const auto existing = env.find (stmt.target->text);
                if (existing != env.end() && existing->second.integer)
                {
                    const bool targetWide = existing->second.wide;
                    if (! value.integer
                        || (! targetWide && (value.iLower < std::numeric_limits<int32_t>::min()
                                             || value.iUpper > std::numeric_limits<int32_t>::max())))
                        value = targetWide ? anyInt64() : anyInt32();
                    else
                        value = intRange (value.iLower, value.iUpper, targetWide);
                }
                else if (existing != env.end())
                {
                    const bool targetWide = existing->second.wide;
                    if (value.integer)
                    {
                        if (targetWide && value.iLower >= std::numeric_limits<int32_t>::min()
                            && value.iUpper <= std::numeric_limits<int32_t>::max())
                            value = floatRange (static_cast<double> (value.iLower), static_cast<double> (value.iUpper), true);
                        else if (! targetWide)
                            value = floatRange (floatLower (value), floatUpper (value), false);
                        else
                            value = anyFloat (true);
                    }
                    else
                    {
                        value = floatRange (value.fLower, value.fUpper, targetWide);
                    }
                }
                else
                {
                    value.integer = false;
                    value.adaptable = false;
                }
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
                    known->second = known->second.integer
                                        ? (known->second.wide ? anyInt64() : anyInt32())
                                        : anyFloat (known->second.wide);
            }

            const auto outer = env;

            // A body that assigns its own loop variable says nothing about it.
            const bool counts = assigned.count (stmt.name) == 0;
            const int64_t lo = counts && first.integer
                                   ? std::max<int64_t> (first.iLower, std::numeric_limits<int32_t>::min())
                                   : std::numeric_limits<int32_t>::min();
            const int64_t hi = counts && last.integer && last.iUpper > std::numeric_limits<int32_t>::min()
                                   ? std::min<int64_t> (last.iUpper - 1, std::numeric_limits<int32_t>::max())
                                   : std::numeric_limits<int32_t>::max();
            env[stmt.name] = lo <= hi ? intRange (lo, hi, false) : anyInt32();

            statement (*stmt.body, env);
            env = outer;
            return;
        }
        // Other control flow invalidates local facts. State invariants have
        // already been checked independently.
        env = stateRanges;
    }

    static Range rangeForType (YdspPrimitiveType type)
    {
        switch (type)
        {
            case YdspPrimitiveType::int32Type: return anyInt32();
            case YdspPrimitiveType::int64Type: return anyInt64();
            case YdspPrimitiveType::float32Type: return anyFloat (false);
            case YdspPrimitiveType::float64Type: return anyFloat (true);
            case YdspPrimitiveType::boolType: return {};
        }
        return {};
    }

public:
    explicit YdspRingBoundsProof (const YdspAnalyzedProcessor& processor)
    {
        const auto& decl = *processor.decl;
        if (decl.process == nullptr || ! decl.functions.empty())
            return;

        stateRanges["blockSize"] = intRange (0, std::numeric_limits<int32_t>::max(), false);
        stateRanges["pi"] = floatRange (3.1415925025939941, 3.1415927410125732, false);
        stateRanges["e"] = floatRange (2.7182817459106445, 2.7182819843292236, false);
        stateRanges["sampleRate"] = anyFloat (false);
        stateRanges["samplePeriod"] = anyFloat (false);

        for (const auto& endpoint : decl.endpoints)
        {
            if (endpoint.kind == YdspEndpointKind::inputStream
                || endpoint.kind == YdspEndpointKind::outputStream
                || endpoint.kind == YdspEndpointKind::inputParameter
                || endpoint.kind == YdspEndpointKind::outputParameter)
            {
                stateRanges[endpoint.name] = rangeForType (endpoint.type);
            }
        }

        for (const auto* state : processor.states)
        {
            if (state->arraySize > 0 && state->structName.isEmpty())
            {
                arrays[state->name] = state->arraySize;
                arrayElementRanges[state->name] = rangeForType (state->type);
            }
        }

        for (const auto* state : processor.states)
        {
            if (state->arraySize != 0 || ! state->structName.isEmpty())
                continue;

            if (state->type != YdspPrimitiveType::int32Type && state->type != YdspPrimitiveType::int64Type)
            {
                stateRanges[state->name] = rangeForType (state->type);
                continue;
            }

            const bool isWide = state->type == YdspPrimitiveType::int64Type;
            int64_t bound = 0;
            bool valid = state->annotations.empty() && boundWrites (decl.process->body, state->name, bound);

            if (decl.init != nullptr)
                valid = valid && boundWrites (decl.init->body, state->name, bound);

            for (const auto& handler : decl.eventHandlers)
                valid = valid && boundWrites (handler.body, state->name, bound);

            for (const auto& initial : state->initialisers)
            {
                const auto value = literal (initial.get());
                valid &= value.integer && value.iLower >= 0 && value.iUpper < bound;
            }

            // Without a tighter one, the type is the invariant: the state is
            // at least known to be an integer.
            stateRanges[state->name] = valid && bound > 0 ? intRange (0, bound - 1, isWide) : (isWide ? anyInt64() : anyInt32());
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
    std::unordered_map<String, Range> arrayElementRanges;
};

} // namespace detail
} // namespace yup
