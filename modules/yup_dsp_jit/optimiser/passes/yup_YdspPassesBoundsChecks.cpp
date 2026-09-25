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

bool YdspOptimizer::eliminateProvenBoundsChecks (YdspIrFunction& fn)
{
    struct Range
    {
        int64_t lower = std::numeric_limits<int32_t>::min();
        int64_t upper = std::numeric_limits<int32_t>::max();
    };
    bool arithmeticChanged = false;
    const auto count = fn.valueTypes.size();
    std::vector<int> definitions (count, 0);
    std::vector<Range> ranges (count);
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0)
                ++definitions[static_cast<size_t> (inst.result)];

    const auto rangeOf = [&] (int value) -> Range
    {
        if (value < 0 || definitions[static_cast<size_t> (value)] != 1
            || fn.valueTypes[static_cast<size_t> (value)] != YdspValueType::int32Type
            || fn.laneCountOf (value) != 1)
            return {};
        return ranges[static_cast<size_t> (value)];
    };

    // Begin with the full domain and only narrow it. A bounded number of
    // rounds limits compile time; stopping early merely keeps more checks.
    for (int round = 0; round < 16; ++round)
    {
        bool changed = false;
        for (auto& block : fn.blocks)
            for (auto& inst : block.insts)
            {
                if (inst.result < 0 || definitions[static_cast<size_t> (inst.result)] != 1
                    || fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::int32Type
                    || fn.laneCountOf (inst.result) != 1)
                    continue;
                const auto a = rangeOf (inst.a);
                const auto b = rangeOf (inst.b);
                const auto c = rangeOf (inst.c);
                Range result;
                switch (inst.op)
                {
                    case YdspIrOp::constI:
                        result = { static_cast<int32_t> (inst.ivalue), static_cast<int32_t> (inst.ivalue) };
                        break;
                    case YdspIrOp::movI:
                        result = a;
                        break;
                    case YdspIrOp::andI:
                        if (a.lower >= 0 || b.lower >= 0)
                            result = { 0, std::min (a.lower >= 0 ? a.upper : result.upper,
                                                   b.lower >= 0 ? b.upper : result.upper) };
                        break;
                    case YdspIrOp::minI:
                        result = { std::min (a.lower, b.lower), std::min (a.upper, b.upper) };
                        break;
                    case YdspIrOp::maxI:
                        result = { std::max (a.lower, b.lower), std::max (a.upper, b.upper) };
                        break;
                    case YdspIrOp::clampI:
                        // clamp is min(max(a, b), c), including reversed bounds.
                        result = { std::min (std::max (a.lower, b.lower), c.lower),
                                   std::min (std::max (a.upper, b.upper), c.upper) };
                        break;
                    case YdspIrOp::selectB:
                        result = { std::min (b.lower, c.lower), std::max (b.upper, c.upper) };
                        break;
                    case YdspIrOp::addI:
                        result = { a.lower + b.lower, a.upper + b.upper };
                        break;
                    case YdspIrOp::subI:
                        result = { a.lower - b.upper, a.upper - b.lower };
                        break;
                    case YdspIrOp::mulI:
                    {
                        // Products of int32 endpoints fit in int64, including MIN * MIN.
                        const auto limits = std::minmax ({ a.lower * b.lower, a.lower * b.upper,
                                                          a.upper * b.lower, a.upper * b.upper });
                        result = { limits.first, limits.second };
                        break;
                    }
                    default:
                        continue;
                }
                // Never use wrapping arithmetic as an address-range proof.
                if (result.lower < std::numeric_limits<int32_t>::min()
                    || result.upper > std::numeric_limits<int32_t>::max())
                    continue;
                if (inst.saturatingIntegerArithmetic && (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI || inst.op == YdspIrOp::mulI))
                {
                    inst.saturatingIntegerArithmetic = false;
                    arithmeticChanged = true;
                }
                auto& known = ranges[static_cast<size_t> (inst.result)];
                if (result.lower != known.lower || result.upper != known.upper)
                {
                    known = result;
                    changed = true;
                }
            }
        if (! changed)
            break;
    }

    std::unordered_set<int> proven;
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op != YdspIrOp::ltUI || inst.result < 0
                || definitions[static_cast<size_t> (inst.result)] != 1
                || fn.valueTypes[static_cast<size_t> (inst.a)] != YdspValueType::int32Type
                || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::int32Type)
                continue;
            const auto index = rangeOf (inst.a);
            const auto bound = rangeOf (inst.b);
            if (index.lower >= 0 && index.upper < bound.lower)
                proven.insert (inst.result);
        }
    if (proven.empty())
        return arithmeticChanged;

    for (auto& block : fn.blocks)
    {
        for (auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::ltUI && proven.count (inst.result) != 0)
            {
                inst.op = YdspIrOp::constB;
                inst.a = inst.b = inst.c = -1;
                inst.bvalue = true;
            }
            else if (inst.op == YdspIrOp::selectB && proven.count (inst.a) != 0)
            {
                inst.op = moveOpcodeFor (fn.valueTypes[static_cast<size_t> (inst.result)]);
                inst.a = inst.b;
                inst.b = inst.c = -1;
            }
        }
        if (block.term == YdspIrTerm::branchIf && proven.count (block.termCond) != 0)
        {
            block.term = YdspIrTerm::branch;
            block.termCond = block.termTarget2 = -1;
            block.preservesConditionalEvaluation = false;
        }
    }
    return true;
}

} // namespace yup
