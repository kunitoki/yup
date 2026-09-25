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

bool YdspOptimizer::deadCodeElimination (YdspIrFunction& fn)
{
    auto counts = YdspValueUseCounts (fn);

    struct Definition
    {
        YdspIrInst* inst;
        int next;
        bool dead = false;
    };

    std::vector<int> heads (fn.valueTypes.size(), -1);
    std::vector<Definition> definitions;
    size_t instructionCount = 0;

    for (const auto& block : fn.blocks)
        instructionCount += block.insts.size();

    definitions.reserve (instructionCount);

    for (auto& block : fn.blocks)
        for (auto& inst : block.insts)
        {
            if (inst.result < 0 || ! hasValueResult (inst.op))
                continue;

            auto& head = heads[static_cast<size_t> (inst.result)];
            definitions.push_back ({ &inst, head });
            head = static_cast<int> (definitions.size()) - 1;
        }

    std::vector<int> pending;
    pending.reserve (heads.size());

    for (size_t value = 0; value < heads.size(); ++value)
        if (heads[value] >= 0 && counts.uses[value] == 0)
            pending.push_back (static_cast<int> (value));

    bool changed = false;

    while (! pending.empty())
    {
        const auto value = pending.back();
        pending.pop_back();

        for (int index = heads[static_cast<size_t> (value)]; index >= 0; index = definitions[static_cast<size_t> (index)].next)
        {
            auto& definition = definitions[static_cast<size_t> (index)];
            definition.dead = true;
            changed = true;

            const auto& inst = *definition.inst;
            const int operands[] { inst.a, inst.b, inst.c };

            for (int operand = 0; operand < 3; ++operand)
            {
                const auto used = operands[operand];
                if (used >= 0 && isValueIdOperand (inst.op, operand)
                    && --counts.uses[static_cast<size_t> (used)] == 0
                    && heads[static_cast<size_t> (used)] >= 0)
                    pending.push_back (used);
            }
        }
    }

    size_t definitionIndex = 0;

    for (auto& block : fn.blocks)
        block.insts.erase (std::remove_if (block.insts.begin(), block.insts.end(), [&] (const YdspIrInst& inst)
        {
            if (inst.result < 0 || ! hasValueResult (inst.op))
                return false;

            return definitions[definitionIndex++].dead;
        }), block.insts.end());

    return changed;
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

    fn.loops.erase (std::remove_if (fn.loops.begin(), fn.loops.end(), [] (const YdspIrLoop& loop)
    {
        return loop.headerBlock < 0;
    }), fn.loops.end());

    std::vector<YdspIrBlock> compacted (static_cast<size_t> (next));

    for (size_t i = 0; i < fn.blocks.size(); ++i)
        if (reachable[i])
            compacted[static_cast<size_t> (remap[i])] = std::move (fn.blocks[i]);

    fn.blocks = std::move (compacted);
}
} // namespace yup
