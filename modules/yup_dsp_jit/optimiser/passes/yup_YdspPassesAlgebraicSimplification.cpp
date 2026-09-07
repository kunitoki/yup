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

void YdspOptimizer::algebraicSimplification (YdspIrFunction& fn)
{
    std::unordered_map<int, int> definitionCount;
    std::unordered_map<int, int> useCount;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result >= 0)
                ++definitionCount[inst.result];

            if (inst.a >= 0)
                ++useCount[inst.a];

            if (inst.b >= 0)
                ++useCount[inst.b];

            if (inst.c >= 0)
                ++useCount[inst.c];
        }
    }

    for (auto& block : fn.blocks)
    {
        for (auto& inst : block.insts)
        {
            const int result = inst.result;

            if (result < 0)
                continue;

            auto literalOf = [&] (int id) -> YdspIrInst*
            {
                if (const auto count = definitionCount.find (id); count == definitionCount.end() || count->second != 1)
                    return nullptr;

                for (auto& other : block.insts)
                    if (other.result == id && isConstantOp (other.op))
                        return &other;

                return nullptr;
            };

            const auto isPowerOfTwo = [] (int64_t value)
            {
                return value > 0 && (value & (value - 1)) == 0;
            };

            // Proves a value is non-negative by looking at its (single)
            // definition: a non-negative constant, a copy of a known
            // non-negative value, or the result of AND with a non-negative
            // constant mask (which clears the sign bit).
            auto knownNonNegative = [&] (int id) -> bool
            {
                // A constant-bound loop's induction is always non-negative when
                // it is seeded from a non-negative literal: it only ever
                // increments from there (this is the value-range insight that
                // lets `i % 2^k` on a loop index become a mask).
                for (const auto& loop : fn.loops)
                {
                    if (loop.induction != id || loop.bound.kind != YdspLoopBoundKind::constant || loop.bound.constant < 0)
                        continue;

                    for (const auto& fnBlock : fn.blocks)
                    {
                        for (const auto& init : fnBlock.insts)
                        {
                            if (init.op != YdspIrOp::movI || init.result != id)
                                continue;

                            const auto count = definitionCount.find (init.a);

                            if (count == definitionCount.end() || count->second != 1)
                                continue;

                            for (const auto& def : fnBlock.insts)
                                if (def.result == init.a && def.op == YdspIrOp::constI && def.ivalue >= 0)
                                    return true;
                        }
                    }
                }

                for (int depth = 0; depth < 4 && id >= 0; ++depth)
                {
                    if (const auto count = definitionCount.find (id); count == definitionCount.end() || count->second != 1)
                        return false;

                    const YdspIrInst* definition = nullptr;

                    for (const auto& other : block.insts)
                        if (other.result == id)
                        {
                            definition = &other;
                            break;
                        }

                    if (definition == nullptr)
                        return false;

                    switch (definition->op)
                    {
                        case YdspIrOp::constI:
                            return definition->ivalue >= 0;

                        case YdspIrOp::movI:
                            id = definition->a;
                            continue;

                        case YdspIrOp::andI:
                            if (const auto* mask = literalOf (definition->a); mask != nullptr && mask->ivalue >= 0)
                                return true;

                            if (const auto* mask = literalOf (definition->b); mask != nullptr && mask->ivalue >= 0)
                                return true;

                            return false;

                        default:
                            return false;
                    }
                }

                return false;
            };

            switch (inst.op)
            {
                case YdspIrOp::addF:
                {
                    // x + 0.0 -> x is not exact in strict mode: x = -0.0 gives
                    // +0.0, so it is fast-math only (x - 0.0 below is exact).
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                        {
                            inst.op = YdspIrOp::movF;
                            inst.b = -1;
                            continue;
                        }

                        if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                        {
                            inst.op = YdspIrOp::movF;
                            inst.a = inst.b;
                            inst.b = -1;
                            continue;
                        }
                    }

                    break;
                }

                case YdspIrOp::addI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::subF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::subI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::mulF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    // x * 0.0 -> 0.0 is not exact: NaN * 0.0 and inf * 0.0 are
                    // NaN, and -0.0 * 0.0 is -0.0, so it is fast-math only.
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                        {
                            inst.op = YdspIrOp::constF;
                            inst.a = inst.b = -1;
                            inst.fvalue = 0.0;
                            continue;
                        }

                        if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                        {
                            inst.op = YdspIrOp::constF;
                            inst.a = inst.b = -1;
                            inst.fvalue = 0.0;
                            continue;
                        }
                    }

                    break;
                }

                case YdspIrOp::mulI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::divF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    // x / c -> x * (1 / c): the reciprocal removes the division
                    // (a helper or slow instruction on some targets) at the
                    // cost of an extra rounding, so it is fast-math only. The
                    // constant denominators appear throughout filters and
                    // oscillators. Only fires when the constant is defined once
                    // and read once, so rewriting its payload is safe.
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr
                            && lit->fvalue != 0.0 && std::isfinite (lit->fvalue)
                            && static_cast<size_t> (inst.result) < fn.valueTypes.size()
                            && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type)
                        {
                            if (const auto uses = useCount.find (inst.b); uses != useCount.end() && uses->second == 1)
                            {
                                if (auto* mut = literalOf (inst.b); mut != nullptr)
                                {
                                    mut->fvalue = 1.0 / lit->fvalue;
                                    inst.op = YdspIrOp::mulF;
                                    continue;
                                }
                            }
                        }
                    }

                    break;
                }

                case YdspIrOp::modI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    // x % 2^k -> x & (2^k - 1) is only equivalent to the C
                    // remainder when x is non-negative, so it is gated on a
                    // proof of that rather than on fastMath: a masked value
                    // (x & m) or a non-negative constant is safe.
                    if (const auto* lit = literalOf (inst.b); lit != nullptr
                        && isPowerOfTwo (lit->ivalue) && knownNonNegative (inst.a))
                    {
                        if (const auto uses = useCount.find (inst.b); uses != useCount.end() && uses->second == 1)
                        {
                            if (auto* mut = literalOf (inst.b); mut != nullptr)
                            {
                                mut->ivalue -= 1;
                                inst.op = YdspIrOp::andI;
                                continue;
                            }
                        }
                    }

                    break;
                }

                case YdspIrOp::divI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::andI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::orI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::xorI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::shlI:
                case YdspIrOp::shrI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::andB:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::constB;
                        inst.a = inst.b = -1;
                        inst.bvalue = false;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::orB:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::constB;
                        inst.a = inst.b = -1;
                        inst.bvalue = true;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::powF:
                {
                    // pow (x, 2.0) -> x * x under fast math: libm pow rounds
                    // once with its own algorithm, so it is not bit-equal to a
                    // multiply; strict mode keeps the intrinsic.
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 2.0)
                        {
                            inst.op = YdspIrOp::mulF;
                            inst.b = inst.a;
                            continue;
                        }
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }
}
} // namespace yup
