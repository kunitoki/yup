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

void YdspOptimizer::splitWidenedReductionChains (YdspIrFunction& fn)
{
    constexpr size_t minChainLength = 4;

    if (! fn.vectorized || fn.valueLanes.empty())
        return;

    const auto newAccumulator = [&fn] (int like)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (fn.valueTypes[static_cast<size_t> (like)]);
        fn.valueLanes.push_back (fn.laneCountOf (like));

        return id;
    };

    struct Link
    {
        size_t addIndex = 0;
        size_t moveIndex = 0;
    };

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;

        for (size_t start = 0; start < insts.size(); ++start)
        {
            if (insts[start].op != YdspIrOp::addF)
                continue;

            int accumulator = -1;

            for (const auto candidate : { insts[start].a, insts[start].b })
            {
                if (candidate < 0 || fn.laneCountOf (candidate) <= 1)
                    continue;

                if (insts[start].result == candidate)
                    accumulator = candidate;
                else if (start + 1 < insts.size()
                         && insts[start + 1].op == YdspIrOp::movF
                         && insts[start + 1].result == candidate
                         && insts[start + 1].a == insts[start].result)
                    accumulator = candidate;
            }

            if (accumulator < 0)
                continue;

            std::vector<Link> links;

            for (size_t i = start; i < insts.size(); ++i)
            {
                const auto& inst = insts[i];

                const auto reads =
                    (isValueIdOperand (inst.op, 0) && inst.a == accumulator)
                    || (isValueIdOperand (inst.op, 1) && inst.b == accumulator)
                    || (isValueIdOperand (inst.op, 2) && inst.c == accumulator);

                if (! reads && inst.result != accumulator)
                    continue;

                if (inst.op == YdspIrOp::addF && inst.result == accumulator
                    && (inst.a == accumulator || inst.b == accumulator))
                {
                    links.push_back ({ i, i });
                    continue;
                }

                if (inst.op == YdspIrOp::addF && (inst.a == accumulator || inst.b == accumulator)
                    && i + 1 < insts.size()
                    && insts[i + 1].op == YdspIrOp::movF
                    && insts[i + 1].result == accumulator
                    && insts[i + 1].a == inst.result)
                {
                    links.push_back ({ i, i + 1 });
                    ++i;
                    continue;
                }

                break;
            }

            if (links.size() < minChainLength)
                continue;

            // ---- Rewrite ----
            const auto second = newAccumulator (accumulator);
            bool startedSecond = false;

            for (size_t k = 1; k < links.size(); k += 2)
            {
                auto& add = insts[links[k].addIndex];
                const auto addend = add.a == accumulator ? add.b : add.a;

                if (! startedSecond)
                {
                    add.op = YdspIrOp::movF;
                    add.a = addend;
                    add.b = -1;
                    startedSecond = true;
                }
                else
                {
                    add.a = second;
                    add.b = addend;
                }

                if (links[k].moveIndex == links[k].addIndex)
                    add.result = second;
                else
                    insts[links[k].moveIndex].result = second;
            }

            YdspIrInst combine;
            combine.op = YdspIrOp::addF;
            combine.result = accumulator;
            combine.a = accumulator;
            combine.b = second;

            insts.insert (insts.begin() + static_cast<std::ptrdiff_t> (links.back().moveIndex + 1), combine);
            fn.reductionSplit = true;
        }
    }
}
} // namespace yup
