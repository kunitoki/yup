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

        const auto hasForeignReader = [&fn, &block, &insts] (size_t addIndex) -> bool
        {
            const int sum = insts[addIndex].result;

            const auto reads = [] (const YdspIrInst& inst, int value)
            {
                for (int operand = 0; operand < 3; ++operand)
                {
                    if (! isValueIdOperand (inst.op, operand))
                        continue;

                    if ((operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c)) == value)
                        return true;
                }

                return false;
            };

            for (size_t k = addIndex + 2; k < insts.size(); ++k)
            {
                if (insts[k].result == sum)
                    return false; // rebound: everything before read this copy's sum

                if (reads (insts[k], sum))
                    return true;
            }

            for (const auto& otherBlock : fn.blocks)
            {
                if (&otherBlock == &block)
                    continue;

                for (const auto& inst : otherBlock.insts)
                    if (reads (inst, sum))
                        return true;
            }

            return false;
        };

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
                    && insts[i + 1].a == inst.result
                    && ! hasForeignReader (i))
                {
                    links.push_back ({ i, i + 1 });
                    ++i;
                    continue;
                }

                break;
            }

            if (links.size() < minChainLength)
                continue;

            // ---- Rewrite: build a balanced tree over the addends ----
            // The serial chain `acc = ((acc + x1) + x2) + ...` has a
            // dependency depth of n. Each link's addend is independent, so the
            // depth collapses to ceil(log2(n + 1)) by pairing adjacent leaves
            // level by level. The pre-chain value of `accumulator` is one more
            // leaf, snapshotted first.

            const auto chainBegin = links.front().addIndex;
            const auto chainEnd = links.back().moveIndex;

            // Snapshot the pre-chain accumulator value into a fresh id, placed
            // before the first link overwrites it.
            const auto initialAccumulator = newAccumulator (accumulator);

            YdspIrInst accumulatorSnapshot;
            accumulatorSnapshot.op = YdspIrOp::movF;
            accumulatorSnapshot.result = initialAccumulator;
            accumulatorSnapshot.a = accumulator;

            std::vector<YdspIrInst> leafSnapshots;
            std::vector<size_t> leafAddIndexes;
            leafSnapshots.reserve (links.size());
            leafAddIndexes.reserve (links.size());

            std::vector<int> leaves;
            leaves.reserve (links.size() + 1);
            leaves.push_back (initialAccumulator);

            for (const auto& link : links)
            {
                const auto& add = insts[link.addIndex];

                const auto addend = add.a == accumulator ? add.b : add.a;
                const auto leaf = newAccumulator (addend);

                YdspIrInst leafSnapshot;
                leafSnapshot.op = YdspIrOp::movF;
                leafSnapshot.result = leaf;
                leafSnapshot.a = addend;

                leafSnapshots.push_back (leafSnapshot);
                leafAddIndexes.push_back (link.addIndex);
                leaves.push_back (leaf);
            }

            // Balanced pairwise reduction.
            std::vector<YdspIrInst> tree;

            auto level = leaves;

            while (level.size() > 1)
            {
                std::vector<int> nextLevel;

                for (size_t i = 0; i + 1 < level.size(); i += 2)
                {
                    const auto partial = newAccumulator (accumulator);

                    YdspIrInst combine;
                    combine.op = YdspIrOp::addF;
                    combine.result = partial;
                    combine.a = level[i];
                    combine.b = level[i + 1];
                    tree.push_back (combine);

                    nextLevel.push_back (partial);
                }

                if ((level.size() & 1) != 0)
                    nextLevel.push_back (level.back());

                level = std::move (nextLevel);
            }

            const auto root = level.front();

            YdspIrInst writeBack;
            writeBack.op = YdspIrOp::movF;
            writeBack.result = accumulator;
            writeBack.a = root;

            // Reassemble the block: prefix, the accumulator snapshot at the
            // chain start, the middle with each link's addend snapshotted at
            // its own position (skipping the chain's add/move instructions),
            // then the tree and the final write-back where the chain ended.
            std::vector<bool> skipped (insts.size(), false);

            for (const auto& link : links)
            {
                skipped[link.addIndex] = true;
                skipped[link.moveIndex] = true;
            }

            std::vector<YdspIrInst> rewritten;
            rewritten.reserve (insts.size() + tree.size() + leafSnapshots.size() + 2);

            rewritten.insert (rewritten.end(), insts.begin(), insts.begin() + static_cast<std::ptrdiff_t> (chainBegin));
            rewritten.push_back (accumulatorSnapshot);

            size_t nextLeaf = 0;

            for (size_t i = chainBegin; i <= chainEnd; ++i)
            {
                if (! skipped[i])
                {
                    rewritten.push_back (insts[i]);
                    continue;
                }

                if (nextLeaf < leafSnapshots.size() && i == leafAddIndexes[nextLeaf])
                    rewritten.push_back (leafSnapshots[nextLeaf++]);
            }

            rewritten.insert (rewritten.end(), tree.begin(), tree.end());
            rewritten.push_back (writeBack);

            rewritten.insert (rewritten.end(),
                              insts.begin() + static_cast<std::ptrdiff_t> (chainEnd + 1),
                              insts.end());

            insts = std::move (rewritten);
            fn.reductionSplit = true;
        }
    }
}
} // namespace yup
