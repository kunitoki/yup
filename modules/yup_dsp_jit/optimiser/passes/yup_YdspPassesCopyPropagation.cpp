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

bool YdspOptimizer::copyPropagation (YdspIrFunction& fn)
{
    bool changed = false;

    for (auto& block : fn.blocks)
    {
        for (size_t i = 0; i < block.insts.size(); ++i)
        {
            const auto& mov = block.insts[i];

            const bool isMov = isMoveOpcode (mov.op);

            if (! isMov || mov.result < 0 || mov.a < 0 || mov.result == mov.a)
                continue;

            const int dst = mov.result;
            const int src = mov.a;

            bool srcRedefined = false;
            bool dstRedefined = false;

            for (size_t j = i + 1; j < block.insts.size(); ++j)
            {
                auto& use = block.insts[j];

                if (use.result == src)
                {
                    srcRedefined = true;
                    break;
                }

                if (use.result == dst)
                {
                    dstRedefined = true;
                    break;
                }

                if (isValueIdOperand (use.op, 0) && use.a == dst)
                {
                    use.a = src;
                    changed = true;
                }
                if (isValueIdOperand (use.op, 1) && use.b == dst)
                {
                    use.b = src;
                    changed = true;
                }
                if (isValueIdOperand (use.op, 2) && use.c == dst)
                {
                    use.c = src;
                    changed = true;
                }
            }

            if (! srcRedefined && ! dstRedefined && block.termCond == dst)
            {
                block.termCond = src;
                changed = true;
            }
        }
    }

    if (fn.blocks.empty())
        return changed;

    // Cross-block SSA copy propagation: when both dst and src have a single
    // definition and src dominates dst, every dominated use (including phi
    // incoming edges) can read src directly.
    const auto types = resolveValueTypes (fn);
    const detail::YdspDomTree dom (fn);
    const size_t numValues = types.size();

    std::vector<int> defCount (numValues, 0);
    std::vector<int> defBlock (numValues, -1);
    std::vector<int> defInst (numValues, -1);

    for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
    {
        const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;

        for (int i = 0; i < static_cast<int> (insts.size()); ++i)
        {
            const int result = insts[static_cast<size_t> (i)].result;

            if (result < 0 || static_cast<size_t> (result) >= numValues)
                continue;

            const auto id = static_cast<size_t> (result);
            ++defCount[id];
            defBlock[id] = b;
            defInst[id] = i;
        }
    }

    std::unordered_set<int> loopInductions;

    for (const auto& loop : fn.loops)
        if (! loop.unrolled && loop.induction >= 0)
            loopInductions.insert (loop.induction);

    std::vector<int> replacement (numValues, -1);

    for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
    {
        if (! dom.isReachable (b))
            continue;

        const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;

        for (int i = 0; i < static_cast<int> (insts.size()); ++i)
        {
            const auto& mov = insts[static_cast<size_t> (i)];

            if (! isMoveOpcode (mov.op) || mov.result < 0 || mov.a < 0 || mov.result == mov.a)
                continue;

            const int dst = mov.result;
            const int src = mov.a;

            if (static_cast<size_t> (dst) >= numValues || static_cast<size_t> (src) >= numValues)
                continue;

            if (defCount[static_cast<size_t> (dst)] != 1 || defCount[static_cast<size_t> (src)] != 1)
                continue;

            if (loopInductions.count (dst) != 0)
                continue;

            if (types[static_cast<size_t> (dst)] != types[static_cast<size_t> (src)]
                || fn.laneCountOf (dst) != fn.laneCountOf (src))
                continue;

            const int sb = defBlock[static_cast<size_t> (src)];
            const int si = defInst[static_cast<size_t> (src)];

            if (! dom.dominates (sb, b) || (sb == b && si >= i))
                continue;

            replacement[static_cast<size_t> (dst)] = src;
        }
    }

    const auto resolveReplacement = [&replacement] (int v) noexcept -> int
    {
        if (v < 0 || static_cast<size_t> (v) >= replacement.size() || replacement[static_cast<size_t> (v)] < 0)
            return -1;

        int cur = v;

        for (size_t step = 0; step < replacement.size(); ++step)
        {
            const int next = replacement[static_cast<size_t> (cur)];

            if (next < 0)
                return cur;

            cur = next;
        }

        return -1;
    };

    for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
    {
        if (! dom.isReachable (b))
            continue;

        auto& block = fn.blocks[static_cast<size_t> (b)];

        for (int i = 0; i < static_cast<int> (block.insts.size()); ++i)
        {
            auto& inst = block.insts[static_cast<size_t> (i)];

            if (inst.op == YdspIrOp::phi)
            {
                for (auto& [pred, val] : inst.phiIncoming)
                {
                    const int rep = resolveReplacement (val);

                    if (rep >= 0 && dom.dominates (defBlock[static_cast<size_t> (rep)], pred))
                    {
                        val = rep;
                        changed = true;
                    }
                }

                continue;
            }

            const auto rewriteOperand = [&] (int operand, int& field)
            {
                if (! isValueIdOperand (inst.op, operand))
                    return;

                const int rep = resolveReplacement (field);

                if (rep < 0)
                    return;

                const int sb = defBlock[static_cast<size_t> (rep)];
                const int si = defInst[static_cast<size_t> (rep)];

                if (dom.dominates (sb, b) && (sb != b || si < i))
                {
                    field = rep;
                    changed = true;
                }
            };

            rewriteOperand (0, inst.a);
            rewriteOperand (1, inst.b);
            rewriteOperand (2, inst.c);
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond >= 0)
        {
            const int rep = resolveReplacement (block.termCond);

            if (rep >= 0 && dom.dominates (defBlock[static_cast<size_t> (rep)], b))
            {
                block.termCond = rep;
                changed = true;
            }
        }
    }

    return changed;
}
} // namespace yup
