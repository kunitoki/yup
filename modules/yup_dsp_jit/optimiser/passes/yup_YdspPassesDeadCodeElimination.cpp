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

void YdspOptimizer::deadCodeElimination (YdspIrFunction& fn)
{
    for (;;)
    {
        std::unordered_map<int, int> useCounts;

        forEachValueUse (fn, [&useCounts] (int, int, int, int value)
        {
            ++useCounts[value];
        });

        bool removed = false;

        for (auto& block : fn.blocks)
        {
            auto& insts = block.insts;

            insts.erase (std::remove_if (insts.begin(), insts.end(), [&] (const YdspIrInst& inst)
            {
                if (inst.result < 0 || ! hasValueResult (inst.op))
                    return false;

                const auto it = useCounts.find (inst.result);

                if (it != useCounts.end() && it->second > 0)
                    return false;

                removed = true;
                return true;
            }),
                         insts.end());
        }

        if (! removed)
            break;
    }
}

//==============================================================================

void YdspOptimizer::removeUnreachableBlocks (YdspIrFunction& fn)
{
    if (fn.blocks.size() < 2)
        return;

    std::vector<char> reachable (fn.blocks.size(), 0);
    std::vector<int> stack { 0 };
    reachable[0] = 1;

    while (! stack.empty())
    {
        const auto index = stack.back();
        stack.pop_back();

        const auto& block = fn.blocks[static_cast<size_t> (index)];
        const auto visit = [&] (int target)
        {
            if (target >= 0 && ! reachable[static_cast<size_t> (target)])
            {
                reachable[static_cast<size_t> (target)] = 1;
                stack.push_back (target);
            }
        };

        if (block.term == YdspIrTerm::fallthrough)
        {
            if (index + 1 < static_cast<int> (fn.blocks.size()))
                visit (index + 1);
            continue;
        }

        visit (block.termTarget);
        visit (block.termTarget2);
    }

    if (std::find (reachable.begin(), reachable.end(), 0) == reachable.end())
        return;

    std::vector<int> remap (fn.blocks.size(), -1);
    int next = 0;

    for (size_t i = 0; i < fn.blocks.size(); ++i)
        if (reachable[i])
            remap[i] = next++;

    for (auto& block : fn.blocks)
    {
        if (block.termTarget >= 0)
            block.termTarget = remap[static_cast<size_t> (block.termTarget)];
        if (block.termTarget2 >= 0)
            block.termTarget2 = remap[static_cast<size_t> (block.termTarget2)];
    }

    for (auto& loop : fn.loops)
    {
        if (loop.headerBlock >= 0)
            loop.headerBlock = remap[static_cast<size_t> (loop.headerBlock)];
        if (loop.exitBlock >= 0)
            loop.exitBlock = remap[static_cast<size_t> (loop.exitBlock)];
    }

    std::vector<YdspIrBlock> compacted (static_cast<size_t> (next));

    for (size_t i = 0; i < fn.blocks.size(); ++i)
        if (reachable[i])
            compacted[static_cast<size_t> (remap[i])] = std::move (fn.blocks[i]);

    fn.blocks = std::move (compacted);
}
} // namespace yup
