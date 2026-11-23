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

bool YdspOptimizer::promoteStateArrays (YdspIrFunction& fn)
{
    if (fn.isInit || fn.isEventHandler || fn.stateArrays.empty() || fn.blocks.empty())
        return false;

    // The per-sample loop is the outermost blockSize-bound loop.
    const auto sampleLoop = std::find_if (fn.loops.begin(), fn.loops.end(), [&fn] (const YdspIrLoop& loop)
    {
        return loop.bound.kind == YdspLoopBoundKind::blockSize
            && ! loop.unrolled
            && loop.headerBlock >= 1
            && loop.exitBlock > loop.headerBlock
            && static_cast<size_t> (loop.exitBlock) < fn.blocks.size();
    });

    if (sampleLoop == fn.loops.end())
        return false;

    const auto headerBlock = static_cast<size_t> (sampleLoop->headerBlock);
    const auto exitBlock = static_cast<size_t> (sampleLoop->exitBlock);
    const size_t preheaderBlock = fn.isSampleMode ? size_t { 0 } : (headerBlock - 1);

    std::vector<char> reachable (fn.blocks.size(), 0);
    {
        std::vector<int> stack { 0 };
        reachable[0] = 1;

        while (! stack.empty())
        {
            const auto idx = stack.back();
            stack.pop_back();
            const auto& block = fn.blocks[static_cast<size_t> (idx)];

            const auto visit = [&] (int target)
            {
                if (target >= 0 && static_cast<size_t> (target) < fn.blocks.size() && ! reachable[static_cast<size_t> (target)])
                {
                    reachable[static_cast<size_t> (target)] = 1;
                    stack.push_back (target);
                }
            };

            if (block.term == YdspIrTerm::fallthrough)
                visit (idx + 1);
            else if (block.term == YdspIrTerm::branch)
                visit (block.termTarget);
            else if (block.term == YdspIrTerm::branchIf)
            {
                visit (block.termTarget);
                visit (block.termTarget2);
            }
        }
    }

    if (! reachable[headerBlock] || ! reachable[exitBlock] || ! reachable[preheaderBlock])
        return false;

    if (! fn.isSampleMode)
    {
        const auto& pre = fn.blocks[preheaderBlock];
        const bool preReachesHeader = pre.term == YdspIrTerm::fallthrough
                                   || (pre.term == YdspIrTerm::branch && pre.termTarget == static_cast<int> (headerBlock));

        if (! preReachesHeader)
            return false;

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            if (! reachable[b])
                continue;

            const bool inLoop = b >= headerBlock && b < exitBlock;
            const auto checkTarget = [&] (int target) -> bool
            {
                if (target < 0 || static_cast<size_t> (target) >= fn.blocks.size())
                    return true;

                const auto t = static_cast<size_t> (target);

                if (inLoop)
                    return (t >= headerBlock && t < exitBlock) || t == exitBlock;

                if (t > headerBlock && t < exitBlock)
                    return false;

                if (t == headerBlock && b != preheaderBlock)
                    return false;

                return true;
            };

            const auto& block = fn.blocks[b];
            if (block.term == YdspIrTerm::fallthrough)
            {
                if (! checkTarget (static_cast<int> (b) + 1))
                    return false;
            }
            else if (block.term == YdspIrTerm::branch)
            {
                if (! checkTarget (block.termTarget))
                    return false;
            }
            else if (block.term == YdspIrTerm::branchIf)
            {
                if (! checkTarget (block.termTarget) || ! checkTarget (block.termTarget2))
                    return false;
            }
        }
    }

    bool hasCallsInSampleLoop = false;

    for (size_t b = headerBlock; b < exitBlock && ! hasCallsInSampleLoop; ++b)
    {
        if (! reachable[b])
            continue;

        for (const auto& inst : fn.blocks[b].insts)
        {
            if (isCallBoundary (inst.op) || inst.op == YdspIrOp::expF)
            {
                hasCallsInSampleLoop = true;
                break;
            }
        }
    }

    // Scale register budgets to target register file size and call/scalar pressure.
    // AArch64 provides 32 FP + 30 GP registers (8 callee-saved FP across calls).
    // x86-64 / other targets provide 16 XMM + 16 GP registers (0 callee-saved XMM on SysV).
    const bool hasLargeRegisterFile = targetHasFusedMultiplyAdd && vectorWidth > 1 && vectorWidth <= 4;

    int maxArrayLength = 12;
    size_t maxTotalFpSlots = 14;
    size_t maxTotalIntSlots = 8;

    if (hasLargeRegisterFile)
    {
        maxArrayLength = 16;
        maxTotalFpSlots = 24;
        maxTotalIntSlots = 20;
    }

    if (hasCallsInSampleLoop)
    {
        if (hasLargeRegisterFile)
        {
            maxArrayLength = 8;
            maxTotalFpSlots = 8;
            maxTotalIntSlots = 8;
        }
        else
        {
            maxArrayLength = 4;
            maxTotalFpSlots = 4;
            maxTotalIntSlots = 4;
        }
    }

    const int maxCandidateArrayLength = hasCallsInSampleLoop ? maxArrayLength : 24;

    std::vector<char> usedInLoop (fn.valueTypes.size(), 0);
    for (size_t b = headerBlock; b < exitBlock; ++b)
    {
        if (! reachable[b])
            continue;

        for (const auto& inst : fn.blocks[b].insts)
        {
            if (isValueIdOperand (inst.op, 0) && inst.a >= 0 && static_cast<size_t> (inst.a) < usedInLoop.size())
                usedInLoop[static_cast<size_t> (inst.a)] = 1;
            if (isValueIdOperand (inst.op, 1) && inst.b >= 0 && static_cast<size_t> (inst.b) < usedInLoop.size())
                usedInLoop[static_cast<size_t> (inst.b)] = 1;
            if (isValueIdOperand (inst.op, 2) && inst.c >= 0 && static_cast<size_t> (inst.c) < usedInLoop.size())
                usedInLoop[static_cast<size_t> (inst.c)] = 1;
        }
    }

    std::unordered_set<int> preLoopFpValues;
    std::unordered_set<int> preLoopIntValues;

    for (size_t b = 0; b < headerBlock; ++b)
    {
        if (! reachable[b])
            continue;

        for (const auto& inst : fn.blocks[b].insts)
        {
            if (inst.op == YdspIrOp::loadStateF || inst.op == YdspIrOp::loadStateArrayF)
                preLoopFpValues.insert (inst.result);
            else if (inst.op == YdspIrOp::loadStateI || inst.op == YdspIrOp::loadStateArrayI)
                preLoopIntValues.insert (inst.result);
            else if (! fn.isSampleMode && inst.result >= 0
                     && static_cast<size_t> (inst.result) < usedInLoop.size()
                     && usedInLoop[static_cast<size_t> (inst.result)]
                     && inst.op != YdspIrOp::constF && inst.op != YdspIrOp::constI && inst.op != YdspIrOp::constB
                     && inst.op != YdspIrOp::loadParam)
            {
                const auto type = fn.valueTypes[static_cast<size_t> (inst.result)];
                if (isFloatValueType (type))
                    preLoopFpValues.insert (inst.result);
                else if (type == YdspValueType::int32Type || type == YdspValueType::int64Type)
                    preLoopIntValues.insert (inst.result);
            }
        }
    }

    const int preLoopFpScalars = static_cast<int> (preLoopFpValues.size());
    const int preLoopIntScalars = static_cast<int> (preLoopIntValues.size());

    const auto liveFpScalars = static_cast<size_t> (std::max (fn.float32Scalars + fn.float64Scalars, preLoopFpScalars));
    const auto liveIntScalars = static_cast<size_t> (std::max (fn.int32Scalars + fn.int64Scalars, preLoopIntScalars));

    const size_t remainingFpBudget = maxTotalFpSlots > liveFpScalars ? maxTotalFpSlots - liveFpScalars : 0;
    const size_t remainingIntBudget = maxTotalIntSlots > liveIntScalars ? maxTotalIntSlots - liveIntScalars : 0;

    const YdspValueUseCounts counts (fn);
    std::vector<const YdspIrInst*> defInst (fn.valueTypes.size(), nullptr);

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0 && static_cast<size_t> (inst.result) < defInst.size() && counts.definedOnce (inst.result))
                defInst[static_cast<size_t> (inst.result)] = &inst;

    std::vector<std::optional<int64_t>> constIntValues (fn.valueTypes.size(), std::nullopt);
    std::vector<char> resolvedConst (fn.valueTypes.size(), 0);
    std::vector<char> visiting (fn.valueTypes.size(), 0);

    std::function<std::optional<int64_t> (int)> resolveConstInt = [&] (int value) -> std::optional<int64_t>
    {
        if (value < 0 || static_cast<size_t> (value) >= defInst.size())
            return std::nullopt;

        const auto idx = static_cast<size_t> (value);

        if (resolvedConst[idx])
            return constIntValues[idx];

        const auto* inst = defInst[idx];

        if (inst == nullptr || visiting[idx])
            return std::nullopt;

        visiting[idx] = 1;
        std::optional<int64_t> resolved;

        if (inst->op == YdspIrOp::constI)
        {
            resolved = fn.valueTypes[idx] == YdspValueType::int32Type
                         ? static_cast<int64_t> (static_cast<int32_t> (inst->ivalue))
                         : inst->ivalue;
        }
        else if (inst->op == YdspIrOp::movI)
        {
            resolved = resolveConstInt (inst->a);
        }
        else if (inst->op == YdspIrOp::addI || inst->op == YdspIrOp::subI)
        {
            if (const auto lhs = resolveConstInt (inst->a))
            {
                if (const auto rhs = resolveConstInt (inst->b))
                {
                    if (fn.valueTypes[idx] == YdspValueType::int32Type)
                    {
                        const int64_t sum = inst->op == YdspIrOp::addI ? (*lhs + *rhs) : (*lhs - *rhs);

                        if (sum >= std::numeric_limits<int32_t>::min() && sum <= std::numeric_limits<int32_t>::max())
                            resolved = sum;
                    }
                    else
                    {
                        if (inst->op == YdspIrOp::addI)
                        {
                            if ((*rhs >= 0 && *lhs <= std::numeric_limits<int64_t>::max() - *rhs)
                                || (*rhs < 0 && *lhs >= std::numeric_limits<int64_t>::min() - *rhs))
                                resolved = *lhs + *rhs;
                        }
                        else
                        {
                            if ((*rhs >= 0 && *lhs >= std::numeric_limits<int64_t>::min() + *rhs)
                                || (*rhs < 0 && *lhs <= std::numeric_limits<int64_t>::max() + *rhs))
                                resolved = *lhs - *rhs;
                        }
                    }
                }
            }
        }

        visiting[idx] = 0;
        resolvedConst[idx] = 1;
        constIntValues[idx] = resolved;
        return resolved;
    };

    const auto isLoad = [] (YdspIrOp op) noexcept { return op == YdspIrOp::loadStateArrayF || op == YdspIrOp::loadStateArrayI; };
    const auto isStore = [] (YdspIrOp op) noexcept { return op == YdspIrOp::storeStateArrayF || op == YdspIrOp::storeStateArrayI; };

    // True when the instruction accesses the given array: same region (the
    // float and integer opcodes address different ones, as do the widths).
    const auto accesses = [&] (const YdspIrInst& inst, const YdspIrFunction::StateArray& array)
    {
        if ((! isLoad (inst.op) && ! isStore (inst.op)) || inst.memIndex != array.base)
            return false;

        const auto value = isLoad (inst.op) ? inst.result : inst.b;

        if (value < 0 || static_cast<size_t> (value) >= fn.valueTypes.size())
            return false;

        const auto type = fn.valueTypes[static_cast<size_t> (value)];
        const bool isFloatAccess = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::storeStateArrayF;

        return isFloatAccess == isFloatValueType (array.type) && is64BitValueType (type) == is64BitValueType (array.type);
    };

    struct Element
    {
        int index = 0;
        int reg = -1;
        bool written = false;
    };

    struct Promotion
    {
        const YdspIrFunction::StateArray* array = nullptr;
        std::vector<Element> elements;
        int loopAccessCount = 0;
        bool hasReads = false;
        bool hasWrites = false;
    };

    std::vector<Promotion> candidates;

    for (const auto& array : fn.stateArrays)
    {
        if (array.length <= 0 || array.length > maxCandidateArrayLength)
            continue;

        const bool isFloat = isFloatValueType (array.type);
        const size_t arrayLimit = isFloat
                                    ? (hasLargeRegisterFile ? remainingFpBudget : std::min (remainingFpBudget, static_cast<size_t> (maxArrayLength)))
                                    : remainingIntBudget;

        if (arrayLimit == 0)
            continue;

        Promotion promotion;
        promotion.array = &array;
        bool promotable = true;

        for (size_t b = 0; b < fn.blocks.size() && promotable; ++b)
        {
            if (! reachable[b])
                continue;

            for (const auto& inst : fn.blocks[b].insts)
            {
                if (! accesses (inst, array))
                    continue;

                if (b < headerBlock && ! fn.isSampleMode)
                    continue;

                const auto value = isLoad (inst.op) ? inst.result : inst.b;
                const auto index = resolveConstInt (inst.a);

                if (b < headerBlock || b >= exitBlock || ! index.has_value() || *index < 0 || *index >= array.length
                    || fn.laneCountOf (value) != 1)
                {
                    promotable = false;
                    break;
                }

                auto element = std::find_if (promotion.elements.begin(), promotion.elements.end(), [index] (const Element& candidate)
                {
                    return candidate.index == *index;
                });

                if (element == promotion.elements.end())
                {
                    if (promotion.elements.size() >= arrayLimit)
                    {
                        promotable = false;
                        break;
                    }

                    element = promotion.elements.insert (promotion.elements.end(), { static_cast<int> (*index), -1, false });
                }

                const bool written = isStore (inst.op);
                element->written = element->written || written;
                promotion.hasReads = promotion.hasReads || ! written;
                promotion.hasWrites = promotion.hasWrites || written;
                ++promotion.loopAccessCount;
            }
        }

        if (! promotable || promotion.elements.empty() || ! promotion.hasReads)
            continue;

        candidates.push_back (std::move (promotion));
    }

    // Prioritize read-write arrays (whose elements eliminate both loads and
    // stores) and smaller/higher-density arrays when register budget is tight.
    std::stable_sort (candidates.begin(), candidates.end(), [] (const Promotion& a, const Promotion& b)
    {
        if (a.hasWrites != b.hasWrites)
            return a.hasWrites;

        const double densityA = static_cast<double> (a.loopAccessCount) / static_cast<double> (a.elements.size());
        const double densityB = static_cast<double> (b.loopAccessCount) / static_cast<double> (b.elements.size());

        if (densityA != densityB)
            return densityA > densityB;

        return a.elements.size() < b.elements.size();
    });

    // In block mode with many candidate arrays (e.g. a 10-band EQ), keep the
    // promoted budget tighter so unrolled arithmetic intermediates do not spill.
    const size_t effectiveFpBudget = (! fn.isSampleMode && hasLargeRegisterFile && candidates.size() >= 4)
                                       ? std::min (remainingFpBudget, size_t { 16 })
                                       : remainingFpBudget;

    std::vector<Promotion> promotions;
    size_t promotedFpElements = 0;
    size_t promotedIntElements = 0;

    for (auto& promotion : candidates)
    {
        const bool isFloat = isFloatValueType (promotion.array->type);
        const size_t remainingBudget = isFloat
                                         ? (effectiveFpBudget > promotedFpElements ? effectiveFpBudget - promotedFpElements : 0)
                                         : (remainingIntBudget > promotedIntElements ? remainingIntBudget - promotedIntElements : 0);
        const size_t maxForArray = isFloat
                                     ? (hasLargeRegisterFile ? remainingBudget : std::min (remainingBudget, static_cast<size_t> (maxArrayLength)))
                                     : remainingBudget;

        if (promotion.elements.size() > maxForArray)
            continue;

        if (isFloat)
            promotedFpElements += promotion.elements.size();
        else
            promotedIntElements += promotion.elements.size();

        std::sort (promotion.elements.begin(), promotion.elements.end(), [] (const Element& a, const Element& b)
        {
            return a.index < b.index;
        });

        promotions.push_back (std::move (promotion));
    }

    if (promotions.empty())
        return false;

    const auto newValue = [&fn] (YdspValueType type)
    {
        const auto value = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
        {
            if (fn.valueLanes.size() < fn.valueTypes.size() - 1)
                fn.valueLanes.resize (fn.valueTypes.size() - 1, 1);

            fn.valueLanes.push_back (1);
        }

        return value;
    };

    std::vector<YdspIrInst> loads, stores;

    for (auto& promotion : promotions)
    {
        const auto& array = *promotion.array;
        const bool isFloat = isFloatValueType (array.type);

        for (auto& element : promotion.elements)
        {
            const auto index = newValue (YdspValueType::int32Type);
            element.reg = newValue (array.type);

            loads.push_back ({ YdspIrOp::constI, index, -1, -1, -1, -1, 0.0, element.index });
            loads.push_back ({ isFloat ? YdspIrOp::loadStateArrayF : YdspIrOp::loadStateArrayI, element.reg, index, -1, -1, array.base });

            if (element.written)
                stores.push_back ({ isFloat ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI, -1, index, element.reg, -1, array.base });
        }

        for (size_t b = headerBlock; b < exitBlock; ++b)
        {
            if (! reachable[b])
                continue;

            for (auto& inst : fn.blocks[b].insts)
            {
                if (! accesses (inst, array))
                    continue;

                const auto index = *constIntValues[static_cast<size_t> (inst.a)];
                const auto reg = std::find_if (promotion.elements.begin(), promotion.elements.end(), [index] (const Element& candidate)
                {
                    return candidate.index == index;
                })->reg;

                const bool wasLoad = isLoad (inst.op);
                const auto stored = inst.b;

                inst.op = moveOpcodeFor (array.type);
                inst.result = wasLoad ? inst.result : reg;
                inst.a = wasLoad ? reg : stored;
                inst.b = -1;
                inst.memIndex = -1;
            }
        }
    }

    auto& entry = fn.blocks[preheaderBlock].insts;
    entry.insert (entry.end(), loads.begin(), loads.end());

    auto& epilogue = fn.blocks[exitBlock].insts;
    epilogue.insert (epilogue.begin(), stores.begin(), stores.end());

    return true;
}

//==============================================================================

bool YdspOptimizer::combineInvariantFactors (YdspIrFunction& fn)
{
    if (fn.blocks.size() < 2)
        return false;

    const YdspValueUseCounts counts (fn);
    std::vector<char> definedInEntry (fn.valueTypes.size(), 0);

    for (const auto& inst : fn.blocks.front().insts)
        if (counts.definedOnce (inst.result))
            definedInEntry[static_cast<size_t> (inst.result)] = 1;

    // Values this pass creates lie past the end and are never candidates.
    const auto isInvariant = [&] (int value)
    {
        return value >= 0 && static_cast<size_t> (value) < definedInEntry.size() && definedInEntry[static_cast<size_t> (value)] != 0;
    };

    std::vector<YdspIrInst> hoisted;

    for (size_t b = 1; b < fn.blocks.size(); ++b)
    {
        auto& insts = fn.blocks[b].insts;

        for (size_t r = 0; r < insts.size(); ++r)
        {
            auto& outer = insts[r];

            if (outer.op != YdspIrOp::mulF || outer.result < 0 || fn.laneCountOf (outer.result) != 1)
                continue;

            // outer = inner * k2, inner = x * k1, with k1 and k2 fixed for the
            // whole call and x not.
            for (const bool innerIsA : { true, false })
            {
                const auto product = innerIsA ? outer.a : outer.b;
                const auto outerFactor = innerIsA ? outer.b : outer.a;

                if (! isInvariant (outerFactor) || product < 0 || isInvariant (product)
                    || static_cast<size_t> (product) >= counts.uses.size()
                    || ! counts.definedOnce (product) || counts.uses[static_cast<size_t> (product)] != 1)
                    continue;

                const auto inner = std::find_if (insts.begin(), insts.begin() + static_cast<std::ptrdiff_t> (r), [product] (const YdspIrInst& inst)
                {
                    return inst.result == product;
                });

                if (inner == insts.begin() + static_cast<std::ptrdiff_t> (r) || inner->op != YdspIrOp::mulF)
                    continue;

                const auto innerFactor = isInvariant (inner->a) ? inner->a : inner->b;
                const auto varying = isInvariant (inner->a) ? inner->b : inner->a;
                const auto type = fn.valueTypes[static_cast<size_t> (outer.result)];

                if (! isInvariant (innerFactor) || isInvariant (varying) || varying < 0
                    || fn.valueTypes[static_cast<size_t> (innerFactor)] != type
                    || fn.valueTypes[static_cast<size_t> (outerFactor)] != type
                    || fn.valueTypes[static_cast<size_t> (varying)] != type)
                    continue;

                // The varying factor is read where the outer multiply sits, so
                // it must still hold what the inner one read.
                if (std::any_of (inner, insts.begin() + static_cast<std::ptrdiff_t> (r), [varying] (const YdspIrInst& inst)
                    {
                        return inst.result == varying;
                    }))
                    continue;

                const auto combined = static_cast<int> (fn.valueTypes.size());
                fn.valueTypes.push_back (type);

                if (! fn.valueLanes.empty())
                    fn.valueLanes.push_back (1);

                hoisted.push_back ({ YdspIrOp::mulF, combined, innerFactor, outerFactor });

                outer.a = varying;
                outer.b = combined;
                break;
            }
        }
    }

    auto& entry = fn.blocks.front().insts;
    entry.insert (entry.end(), hoisted.begin(), hoisted.end());

    return ! hoisted.empty();
}

} // namespace yup
