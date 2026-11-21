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

void YdspOptimizer::contractMultiplyAdd (YdspIrFunction& fn)
{
    const YdspValueUseCounts counts (fn);

    const auto isFusableFloat32 = [this, &fn] (int value)
    {
        return value >= 0
            && static_cast<size_t> (value) < fn.valueTypes.size()
            && fn.valueTypes[static_cast<size_t> (value)] == YdspValueType::float32Type
            && (fn.laneCountOf (value) == 1 || targetHasPackedFusedMultiplyAdd);
    };

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;
        std::vector<std::pair<size_t, YdspIrInst>> negatedAddends;

        // Track paths from carried registers through intermediate expressions,
        // so e.g. a * z + b * (in - z) fuses the later feedback product.
        std::vector<int> depth (fn.valueTypes.size(), -1);
        for (size_t value = 0; value < depth.size(); ++value)
            if (isFusableFloat32 (static_cast<int> (value)) && ! counts.definedOnce (static_cast<int> (value)))
                depth[value] = 0;

        std::vector<int> recurrenceDepth (insts.size(), -1);
        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& inst = insts[i];
            if (inst.result < 0)
                continue;

            int inputDepth = -1;
            const int operands[] { inst.a, inst.b, inst.c };
            for (int operand = 0; operand < 3; ++operand)
                if (isValueIdOperand (inst.op, operand) && operands[operand] >= 0)
                    inputDepth = std::max (inputDepth, depth[static_cast<size_t> (operands[operand])]);

            const auto resultDepth = inputDepth < 0 ? -1 : inputDepth + (inst.op == YdspIrOp::movF ? 0 : 1);
            depth[static_cast<size_t> (inst.result)] = resultDepth;
            recurrenceDepth[i] = resultDepth;
        }

        for (size_t i = 0; i < insts.size(); ++i)
        {
            if ((insts[i].op != YdspIrOp::addF && insts[i].op != YdspIrOp::subF) || ! isFusableFloat32 (insts[i].result))
                continue;

            const auto findMultiply = [&] (int operand) -> std::optional<size_t>
            {
                if (! isFusableFloat32 (operand) || ! counts.definedOnce (operand) || counts.useCount (operand) != 1)
                    return std::nullopt;

                for (size_t j = i; j-- > 0;)
                {
                    if (insts[j].result != operand)
                        continue;

                    if (insts[j].op != YdspIrOp::mulF || ! isFusableFloat32 (insts[j].a) || ! isFusableFloat32 (insts[j].b))
                        return std::nullopt;

                    for (size_t k = j + 1; k < i; ++k)
                        if (insts[k].result == insts[j].a || insts[k].result == insts[j].b)
                            return std::nullopt;

                    return j;
                }

                return std::nullopt;
            };

            const auto fromA = findMultiply (insts[i].a);
            const auto fromB = findMultiply (insts[i].b);

            if (! fromA.has_value() && ! fromB.has_value())
                continue;

            auto chosen = fromA.value_or (fromB.value_or (0));

            if (fromA.has_value() && fromB.has_value() && recurrenceDepth[*fromB] > recurrenceDepth[*fromA])
                chosen = *fromB;

            if (insts[i].op == YdspIrOp::subF && fromB.has_value())
                chosen = *fromB;

            const auto factorA = insts[chosen].a;
            const auto factorB = insts[chosen].b;
            const auto addend = (insts[chosen].result == insts[i].a) ? insts[i].b : insts[i].a;

            if (! isFusableFloat32 (addend))
                continue;

            const bool subtractAddend = insts[i].op == YdspIrOp::subF && insts[chosen].result == insts[i].a;
            insts[i].op = insts[i].op == YdspIrOp::addF || subtractAddend ? YdspIrOp::fmaF : YdspIrOp::fmsubF;
            insts[i].a = factorA;
            insts[i].b = factorB;
            insts[i].c = addend;

            if (subtractAddend)
            {
                const auto negated = static_cast<int> (fn.valueTypes.size());
                const auto lanes = fn.laneCountOf (insts[i].result);
                fn.valueTypes.push_back (YdspValueType::float32Type);
                if (! fn.valueLanes.empty())
                    fn.valueLanes.push_back (lanes);

                negatedAddends.push_back ({ i, { YdspIrOp::negF, negated, addend } });
                insts[i].c = negated;
            }
        }

        for (auto it = negatedAddends.rbegin(); it != negatedAddends.rend(); ++it)
            insts.insert (insts.begin() + static_cast<std::ptrdiff_t> (it->first), it->second);
    }
}
//==============================================================================

void YdspOptimizer::rebalanceMultiplyAddChains (YdspIrFunction& fn)
{
    // Rough latencies, in cycles. Only their order matters.
    const auto latencyOf = [] (YdspIrOp op) noexcept
    {
        switch (op)
        {
            case YdspIrOp::fmaF:
            case YdspIrOp::fmsubF:
                return 4;

            case YdspIrOp::mulF:
            case YdspIrOp::loadInput:
            case YdspIrOp::loadOutput:
            case YdspIrOp::loadStateArrayF:
            case YdspIrOp::loadStateArrayI:
                return 3;

            case YdspIrOp::movF:
            case YdspIrOp::movI:
            case YdspIrOp::movB:
            case YdspIrOp::constF:
            case YdspIrOp::constI:
            case YdspIrOp::constB:
                return 0;

            case YdspIrOp::divF:
            case YdspIrOp::sqrtF:
                return 10;

            case YdspIrOp::sinF:
            case YdspIrOp::cosF:
            case YdspIrOp::tanF:
            case YdspIrOp::asinF:
            case YdspIrOp::acosF:
            case YdspIrOp::atanF:
            case YdspIrOp::sinhF:
            case YdspIrOp::coshF:
            case YdspIrOp::tanhF:
            case YdspIrOp::asinhF:
            case YdspIrOp::acoshF:
            case YdspIrOp::atanhF:
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::fmodF:
            case YdspIrOp::atan2F:
                return 20;

            default:
                return 2;
        }
    };

    // A value the block reads and then recomputes from itself is carried from
    // one pass over the block to the next: whatever sits between that read and
    // that write is paid on every sample, however early the read comes.
    constexpr int carriedDepth = 1000;
    constexpr size_t maxCarriedValues = 32;

    const auto isFusedOp = [] (YdspIrOp op) noexcept { return op == YdspIrOp::fmaF || op == YdspIrOp::fmsubF; };
    const auto numValues = fn.valueTypes.size();
    const YdspValueUseCounts counts (fn);

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;

        if (std::count_if (insts.begin(), insts.end(), [&] (const YdspIrInst& inst) { return isFusedOp (inst.op); }) < 2)
            continue;

        const auto forEachOperand = [] (const YdspIrInst& inst, auto&& visit)
        {
            if (isValueIdOperand (inst.op, 0) && inst.a >= 0)
                visit (inst.a);
            if (isValueIdOperand (inst.op, 1) && inst.b >= 0)
                visit (inst.b);
            if (isValueIdOperand (inst.op, 2) && inst.c >= 0)
                visit (inst.c);
        };

        std::vector<int> carried;
        {
            std::vector<char> defined (numValues, 0), readFirst (numValues, 0);

            for (const auto& inst : insts)
            {
                forEachOperand (inst, [&] (int value)
                {
                    if (! defined[static_cast<size_t> (value)])
                        readFirst[static_cast<size_t> (value)] = 1;
                });

                if (inst.result < 0)
                    continue;

                // Float values only: a counter or the sample index is carried
                // too, but through a single add, and marking it late would make
                // everything read from a stream look late with it.
                if (readFirst[static_cast<size_t> (inst.result)] && ! defined[static_cast<size_t> (inst.result)]
                    && isFloatValueType (fn.valueTypes[static_cast<size_t> (inst.result)])
                    && carried.size() < maxCarriedValues)
                    carried.push_back (inst.result);

                defined[static_cast<size_t> (inst.result)] = 1;
            }
        }

        std::vector<int> entryDepth (numValues, 0);

        for (const auto candidate : carried)
        {
            std::vector<char> dependsOnEntry (numValues, 0);
            dependsOnEntry[static_cast<size_t> (candidate)] = 1;

            for (const auto& inst : insts)
            {
                bool depends = false;

                forEachOperand (inst, [&] (int value)
                {
                    depends = depends || dependsOnEntry[static_cast<size_t> (value)];
                });

                if (inst.result == candidate && depends)
                    entryDepth[static_cast<size_t> (candidate)] = carriedDepth;

                if (inst.result >= 0)
                    dependsOnEntry[static_cast<size_t> (inst.result)] = depends ? 1 : 0;
            }
        }

        // Each swap moves a late product one multiply-add further down its
        // chain, so a chain of n settles in at most n rounds.
        for (int round = 0; round < 8; ++round)
        {
            bool changed = false;
            auto depth = entryDepth;
            std::vector<int> definedAt (numValues, -1);

            for (size_t r = 0; r < insts.size(); ++r)
            {
                auto& outer = insts[r];

                const auto recordDepth = [&]
                {
                    if (outer.result < 0)
                        return;

                    int latest = 0;

                    forEachOperand (outer, [&] (int value)
                    {
                        latest = jmax (latest, depth[static_cast<size_t> (value)]);
                    });

                    depth[static_cast<size_t> (outer.result)] = latest + latencyOf (outer.op);
                    definedAt[static_cast<size_t> (outer.result)] = static_cast<int> (r);
                };

                const auto innerIndex = isFusedOp (outer.op) && outer.c >= 0 ? definedAt[static_cast<size_t> (outer.c)] : -1;

                if (innerIndex < 0 || ! isFusedOp (insts[static_cast<size_t> (innerIndex)].op)
                    || outer.a == outer.c || outer.b == outer.c || outer.result < 0
                    || ! counts.definedOnce (outer.c) || counts.uses[static_cast<size_t> (outer.c)] != 1
                    || fn.laneCountOf (outer.result) != 1 || fn.laneCountOf (outer.c) != 1)
                {
                    recordDepth();
                    continue;
                }

                auto& inner = insts[static_cast<size_t> (innerIndex)];

                // Swapping evaluates the outer product where the inner one was
                // and the inner one where the outer was, so each product's
                // factors must hold the same values at both places.
                bool movable = true;

                for (size_t j = static_cast<size_t> (innerIndex); j < r && movable; ++j)
                {
                    const auto result = insts[j].result;

                    movable = result < 0
                           || (result != outer.a && result != outer.b
                               && (j == static_cast<size_t> (innerIndex) || (result != inner.a && result != inner.b)));
                }

                if (! movable)
                {
                    recordDepth();
                    continue;
                }

                const auto depthOf = [&depth] (int value) { return depth[static_cast<size_t> (value)]; };
                const auto innerFactors = jmax (depthOf (inner.a), depthOf (inner.b));
                const auto outerFactors = jmax (depthOf (outer.a), depthOf (outer.b));
                const auto addend = depthOf (inner.c);

                const auto current = jmax (jmax (innerFactors, addend) + 4, outerFactors) + 4;
                const auto swapped = jmax (jmax (outerFactors, addend) + 4, innerFactors) + 4;

                if (swapped < current)
                {
                    const auto innerOp = inner.op;
                    const auto innerA = inner.a;
                    const auto innerB = inner.b;

                    inner.op = outer.op;
                    inner.a = outer.a;
                    inner.b = outer.b;

                    outer.op = innerOp;
                    outer.a = innerA;
                    outer.b = innerB;

                    depth[static_cast<size_t> (inner.result)] = jmax (outerFactors, addend) + 4;
                    changed = true;
                }

                recordDepth();
            }

            if (! changed)
                break;
        }
    }
}

} // namespace yup
