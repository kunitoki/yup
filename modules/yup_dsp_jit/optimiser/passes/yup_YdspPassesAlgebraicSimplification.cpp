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

bool YdspOptimizer::algebraicSimplification (YdspIrFunction& fn)
{
    bool changed = false;
    struct InsertedOperands
    {
        YdspIrBlock* block;
        size_t index;
        std::vector<YdspIrInst> instructions;
    };
    std::vector<InsertedOperands> insertedOperands;

    const YdspValueUseCounts counts (fn);
    std::vector<YdspIrInst*> definitions (fn.valueTypes.size(), nullptr);
    std::vector<const YdspIrBlock*> definitionBlocks (fn.valueTypes.size(), nullptr);
    std::vector<bool> boundedInduction (fn.valueTypes.size(), false);
    std::vector<bool> nonNegativeInduction (fn.valueTypes.size(), false);

    for (const auto& loop : fn.loops)
        if (loop.induction >= 0 && loop.bound.kind == YdspLoopBoundKind::constant && loop.bound.constant >= 0)
            boundedInduction[static_cast<size_t> (loop.induction)] = true;

    for (auto& block : fn.blocks)
        for (auto& inst : block.insts)
            if (counts.definedOnce (inst.result))
            {
                definitions[static_cast<size_t> (inst.result)] = &inst;
                definitionBlocks[static_cast<size_t> (inst.result)] = &block;
            }

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::movI && inst.result >= 0
                && boundedInduction[static_cast<size_t> (inst.result)] && counts.definedOnce (inst.a))
            {
                const auto* initial = definitions[static_cast<size_t> (inst.a)];
                if (definitionBlocks[static_cast<size_t> (inst.a)] == &block
                    && initial->op == YdspIrOp::constI && initial->ivalue >= 0)
                    nonNegativeInduction[static_cast<size_t> (inst.result)] = true;
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
                if (! counts.definedOnce (id) || definitionBlocks[static_cast<size_t> (id)] != &block)
                    return nullptr;

                auto* definition = definitions[static_cast<size_t> (id)];
                return isConstantOp (definition->op) ? definition : nullptr;
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
                if (id >= 0 && nonNegativeInduction[static_cast<size_t> (id)])
                    return true;

                for (int depth = 0; depth < 4 && id >= 0; ++depth)
                {
                    if (! counts.definedOnce (id) || definitionBlocks[static_cast<size_t> (id)] != &block)
                        return false;

                    const auto* definition = definitions[static_cast<size_t> (id)];

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
                    // +0.0, so it is fast-math only (subtraction needs a positive zero).
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                        {
                            changed = true;
                            inst.op = YdspIrOp::movF;
                            inst.b = -1;
                            continue;
                        }

                        if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                        {
                            changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::subF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0
                        && (fn.fastMath || ! std::signbit (lit->fvalue)))
                    {
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::mulF:
                {
                    if (fn.fastMath)
                    {
                        for (const bool swap : { false, true })
                        {
                            const int product = swap ? inst.b : inst.a;
                            const auto* outerConstant = literalOf (swap ? inst.a : inst.b);
                            if (outerConstant == nullptr || outerConstant->op != YdspIrOp::constF
                                || ! counts.definedOnce (product) || counts.useCount (product) != 1
                                || definitionBlocks[static_cast<size_t> (product)] != &block)
                                continue;
                            const auto* inner = definitions[static_cast<size_t> (product)];
                            if (inner->op != YdspIrOp::mulF || inner >= &inst)
                                continue;
                            const auto* innerConstant = literalOf (inner->b);
                            int input = inner->a;
                            if (innerConstant == nullptr || innerConstant->op != YdspIrOp::constF)
                            {
                                innerConstant = literalOf (inner->a);
                                input = inner->b;
                            }
                            if (innerConstant == nullptr || innerConstant->op != YdspIrOp::constF)
                                continue;
                            const auto type = fn.valueTypes[static_cast<size_t> (result)];
                            if (fn.valueTypes[static_cast<size_t> (product)] != type
                                || fn.valueTypes[static_cast<size_t> (input)] != type
                                || fn.valueTypes[static_cast<size_t> (innerConstant->result)] != type
                                || fn.valueTypes[static_cast<size_t> (outerConstant->result)] != type)
                                continue;
                            bool stable = true;
                            for (auto* between = inner + 1; between < &inst; ++between)
                                if (between->result == input)
                                    stable = false;
                            if (! stable)
                                continue;
                            const bool single = type == YdspValueType::float32Type;
                            const double a = single ? static_cast<float> (innerConstant->fvalue) : innerConstant->fvalue;
                            const double b = single ? static_cast<float> (outerConstant->fvalue) : outerConstant->fvalue;
                            const double combined = single ? static_cast<double> (static_cast<float> (a * b)) : a * b;
                            if (! std::isnormal (a) || ! std::isnormal (b)
                                || ! std::isnormal (combined)
                                || (single && (! std::isnormal (static_cast<float> (a)) || ! std::isnormal (static_cast<float> (b))
                                               || ! std::isnormal (static_cast<float> (combined)))))
                                continue;
                            const int constant = static_cast<int> (fn.valueTypes.size());
                            fn.valueLanes.resize (fn.valueTypes.size(), 1);
                            fn.valueTypes.push_back (type);
                            fn.valueLanes.push_back (fn.laneCountOf (result));
                            insertedOperands.push_back ({ &block, static_cast<size_t> (&inst - block.insts.data()),
                                                          { { YdspIrOp::constF, constant, -1, -1, -1, -1, combined } } });
                            inst.a = input;
                            inst.b = constant;
                            changed = true;
                            break;
                        }
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 1.0)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 1.0)
                    {
                        changed = true;
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
                            changed = true;
                            inst.op = YdspIrOp::constF;
                            inst.a = inst.b = -1;
                            inst.fvalue = 0.0;
                            continue;
                        }

                        if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                        {
                            changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 1)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    // Keep the original constant intact: other divisions and
                    // non-division uses may share it. CSE shares the reciprocals.
                    if (fn.fastMath)
                    {
                        if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->op == YdspIrOp::constF)
                        {
                            const auto type = fn.valueTypes[static_cast<size_t> (result)];
                            const bool narrow = type == YdspValueType::float32Type;
                            const double denominator = narrow ? static_cast<double> (static_cast<float> (lit->fvalue)) : lit->fvalue;
                            if (! std::isfinite (denominator) || denominator == 0.0)
                                break;
                            const double reciprocal = narrow ? static_cast<double> (1.0f / static_cast<float> (denominator)) : 1.0 / denominator;
                            if (narrow ? std::isnormal (static_cast<float> (reciprocal)) : std::isnormal (reciprocal))
                            {
                                const int value = static_cast<int> (fn.valueTypes.size());
                                fn.valueTypes.push_back (type);
                                fn.valueLanes.resize (fn.valueTypes.size(), 1);
                                fn.valueLanes[static_cast<size_t> (value)] = fn.laneCountOf (result);
                                insertedOperands.push_back ({ &block, static_cast<size_t> (&inst - block.insts.data()),
                                                              { { YdspIrOp::constF, value, -1, -1, -1, -1, reciprocal } } });
                                inst.op = YdspIrOp::mulF;
                                inst.b = value;
                                changed = true;
                            }
                        }
                    }

                    break;
                }

                case YdspIrOp::modI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        changed = true;
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
                        if (counts.useCount (inst.b) == 1)
                        {
                            if (auto* mut = literalOf (inst.b); mut != nullptr)
                            {
                                mut->ivalue -= 1;
                                changed = true;
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
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        changed = true;
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        changed = true;
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        changed = true;
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
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->bvalue)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && ! lit->bvalue)
                    {
                        changed = true;
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
                        changed = true;
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && ! lit->bvalue)
                    {
                        changed = true;
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->bvalue)
                    {
                        changed = true;
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
                            changed = true;
                            inst.op = YdspIrOp::mulF;
                            inst.b = inst.a;
                            continue;
                        }

                        if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->op == YdspIrOp::constF)
                        {
                            const auto type = fn.valueTypes[static_cast<size_t> (result)];
                            const double base = type == YdspValueType::float32Type ? static_cast<double> (static_cast<float> (lit->fvalue))
                                                                                 : lit->fvalue;
                            if (base > 0.0 && base != 1.0 && std::isfinite (base))
                            {
                                const int logarithm = static_cast<int> (fn.valueTypes.size());
                                fn.valueTypes.push_back (type);
                                const int product = static_cast<int> (fn.valueTypes.size());
                                fn.valueTypes.push_back (type);
                                fn.valueLanes.resize (fn.valueTypes.size(), 1);
                                fn.valueLanes[static_cast<size_t> (logarithm)] = fn.laneCountOf (result);
                                fn.valueLanes[static_cast<size_t> (product)] = fn.laneCountOf (result);
                                insertedOperands.push_back ({ &block, static_cast<size_t> (&inst - block.insts.data()),
                                                              { { YdspIrOp::constF, logarithm, -1, -1, -1, -1, std::log (base) },
                                                                { YdspIrOp::mulF, product, inst.b, logarithm } } });
                                inst.op = YdspIrOp::expF;
                                inst.a = product;
                                inst.b = inst.c = -1;
                                changed = true;
                            }
                        }
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }
    for (auto it = insertedOperands.rbegin(); it != insertedOperands.rend(); ++it)
        it->block->insts.insert (it->block->insts.begin() + static_cast<std::ptrdiff_t> (it->index), it->instructions.begin(), it->instructions.end());

    return changed;
}
} // namespace yup
