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
    // Each promoted element holds a register across the whole sample loop.
    constexpr int maxArrayLength = 8;
    constexpr size_t maxPromotedElements = 16;

    if (! fn.isSampleMode || fn.stateArrays.empty())
        return false;

    // The per-sample loop is the one the entry block falls into.
    const auto sampleLoop = std::find_if (fn.loops.begin(), fn.loops.end(), [] (const YdspIrLoop& loop)
    {
        return loop.headerBlock == 1 && loop.bound.kind == YdspLoopBoundKind::blockSize && ! loop.unrolled;
    });

    if (sampleLoop == fn.loops.end() || sampleLoop->exitBlock <= 1 || static_cast<size_t> (sampleLoop->exitBlock) >= fn.blocks.size())
        return false;

    const auto exitBlock = static_cast<size_t> (sampleLoop->exitBlock);

    const YdspValueUseCounts counts (fn);
    std::vector<const YdspIrInst*> constants (fn.valueTypes.size(), nullptr);

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::constI && counts.definedOnce (inst.result))
                constants[static_cast<size_t> (inst.result)] = &inst;

    const auto isLoad = [] (YdspIrOp op) noexcept { return op == YdspIrOp::loadStateArrayF || op == YdspIrOp::loadStateArrayI; };
    const auto isStore = [] (YdspIrOp op) noexcept { return op == YdspIrOp::storeStateArrayF || op == YdspIrOp::storeStateArrayI; };

    // True when the instruction accesses the given array: same region (the
    // float and integer opcodes address different ones, as do the widths).
    const auto accesses = [&] (const YdspIrInst& inst, const YdspIrFunction::StateArray& array)
    {
        if ((! isLoad (inst.op) && ! isStore (inst.op)) || inst.memIndex != array.base)
            return false;

        const auto value = isLoad (inst.op) ? inst.result : inst.b;

        if (value < 0)
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
    };

    std::vector<Promotion> promotions;
    size_t numPromotedElements = 0;

    for (const auto& array : fn.stateArrays)
    {
        if (array.length > maxArrayLength)
            continue;

        Promotion promotion;
        promotion.array = &array;
        bool promotable = true;

        for (size_t b = 0; b < fn.blocks.size() && promotable; ++b)
        {
            for (const auto& inst : fn.blocks[b].insts)
            {
                if (! accesses (inst, array))
                    continue;

                const auto value = isLoad (inst.op) ? inst.result : inst.b;
                const auto* index = inst.a >= 0 ? constants[static_cast<size_t> (inst.a)] : nullptr;

                if (b <= 1 || b >= exitBlock || index == nullptr || index->ivalue < 0 || index->ivalue >= array.length
                    || fn.laneCountOf (value) != 1)
                {
                    promotable = false;
                    break;
                }

                auto element = std::find_if (promotion.elements.begin(), promotion.elements.end(), [index] (const Element& candidate)
                {
                    return candidate.index == index->ivalue;
                });

                if (element == promotion.elements.end())
                    element = promotion.elements.insert (promotion.elements.end(), { static_cast<int> (index->ivalue), -1, false });

                element->written = element->written || isStore (inst.op);
            }
        }

        if (! promotable || promotion.elements.empty() || numPromotedElements + promotion.elements.size() > maxPromotedElements)
            continue;

        numPromotedElements += promotion.elements.size();
        promotions.push_back (std::move (promotion));
    }

    if (promotions.empty())
        return false;

    const auto newValue = [&fn] (YdspValueType type)
    {
        const auto value = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
            fn.valueLanes.push_back (1);

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

        for (size_t b = 2; b < exitBlock; ++b)
        {
            for (auto& inst : fn.blocks[b].insts)
            {
                if (! accesses (inst, array))
                    continue;

                const auto index = constants[static_cast<size_t> (inst.a)]->ivalue;
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

    auto& entry = fn.blocks.front().insts;
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
