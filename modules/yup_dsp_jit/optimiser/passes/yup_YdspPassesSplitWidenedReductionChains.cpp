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

    // Beyond this many links the addends are no longer all held at once for a
    // tree: they are summed into a few running partials as they arrive.
    constexpr size_t maxTreeLength = 8;
    constexpr size_t numPartials = 4;

    const bool allowScalar = fastMath || fn.fastMath;

    // A widened accumulator has already been re-associated by the vectoriser;
    // a scalar one is only re-associated under fastMath.
    if (! allowScalar && (! fn.vectorized || fn.valueLanes.empty()))
        return;

    fn.valueLanes.resize (fn.valueTypes.size(), 1);
    const int initialValueCount = static_cast<int> (fn.valueTypes.size());

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
        std::optional<YdspValueUseCounts> counts;

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

        const auto rewriteChain = [&] (int targetRegister,
                                       int initialSource,
                                       const std::vector<Link>& links,
                                       const std::vector<int>& linkAddends)
        {
            // ---- Rewrite: build a balanced tree over the addends ----
            // The serial chain `acc = ((acc + x1) + x2) + ...` has a
            // dependency depth of n. Each link's addend is independent, so the
            // depth collapses to ceil(log2(n + 1)) by pairing adjacent leaves
            // level by level. The pre-chain value of `targetRegister` is one
            // more leaf, snapshotted first.

            const auto chainBegin = links.front().addIndex;
            const auto chainEnd = links.back().moveIndex;

            // Snapshot the pre-chain accumulator value into a fresh id, placed
            // before the first link overwrites it.
            const auto initialAccumulator = newAccumulator (targetRegister);

            YdspIrInst accumulatorSnapshot;
            accumulatorSnapshot.op = YdspIrOp::movF;
            accumulatorSnapshot.result = initialAccumulator;
            accumulatorSnapshot.a = initialSource;

            std::vector<YdspIrInst> leafSnapshots;
            std::vector<size_t> leafAddIndexes;
            leafSnapshots.reserve (links.size());
            leafAddIndexes.reserve (links.size());

            std::vector<int> leaves;
            leaves.reserve (links.size() + 1);
            leaves.push_back (initialAccumulator);

            // A long scalar chain feeds its addends to the partials in turn,
            // so only those are live at any point; the tree below then sums
            // them. A widened chain keeps the association it always had.
            std::vector<int> partials (links.size() > maxTreeLength && fn.laneCountOf (targetRegister) == 1 ? numPartials : 0, -1);

            for (size_t n = 0; n < links.size(); ++n)
            {
                const auto& add = insts[links[n].addIndex];
                YdspIrInst leafSnapshot;

                if (add.op == YdspIrOp::fmaF)
                {
                    leafSnapshot.result = newAccumulator (add.result);

                    if (partials.empty() || partials[n % numPartials] < 0)
                    {
                        leafSnapshot.op = YdspIrOp::mulF;
                        leafSnapshot.a = add.a;
                        leafSnapshot.b = add.b;
                    }
                    else
                    {
                        leafSnapshot.op = YdspIrOp::fmaF;
                        leafSnapshot.a = add.a;
                        leafSnapshot.b = add.b;
                        leafSnapshot.c = partials[n % numPartials];
                    }
                }
                else
                {
                    const auto addend = linkAddends[n];
                    leafSnapshot.result = newAccumulator (addend);

                    if (partials.empty() || partials[n % numPartials] < 0)
                    {
                        leafSnapshot.op = YdspIrOp::movF;
                        leafSnapshot.a = addend;
                    }
                    else
                    {
                        leafSnapshot.op = YdspIrOp::addF;
                        leafSnapshot.a = partials[n % numPartials];
                        leafSnapshot.b = addend;
                    }
                }

                leafSnapshots.push_back (leafSnapshot);
                leafAddIndexes.push_back (links[n].addIndex);

                if (partials.empty())
                    leaves.push_back (leafSnapshot.result);
                else
                    partials[n % numPartials] = leafSnapshot.result;
            }

            leaves.insert (leaves.end(), partials.begin(), partials.end());

            // Balanced pairwise reduction.
            std::vector<YdspIrInst> tree;

            auto level = leaves;

            while (level.size() > 1)
            {
                std::vector<int> nextLevel;

                for (size_t i = 0; i + 1 < level.size(); i += 2)
                {
                    const auto partial = newAccumulator (targetRegister);

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
            writeBack.result = targetRegister;
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
            counts.reset();
        };

        for (size_t start = 0; start < insts.size(); ++start)
        {
            const auto startOp = insts[start].op;

            if (insts[start].result >= initialValueCount)
                continue;

            if (startOp != YdspIrOp::addF && (! allowScalar || startOp != YdspIrOp::fmaF))
                continue;

            int accumulator = -1;

            if (startOp == YdspIrOp::addF)
            {
                if (insts[start].a != insts[start].b)
                {
                    for (const auto candidate : { insts[start].a, insts[start].b })
                    {
                        if (candidate < 0 || (fn.laneCountOf (candidate) <= 1 && ! allowScalar)
                            || fn.valueTypes[static_cast<size_t> (candidate)] != YdspValueType::float32Type)
                            continue;

                        if (insts[start].result == candidate)
                            accumulator = candidate;
                        else if (start + 1 < insts.size()
                                 && insts[start + 1].op == YdspIrOp::movF
                                 && insts[start + 1].result == candidate
                                 && insts[start + 1].a == insts[start].result)
                            accumulator = candidate;
                    }
                }
            }
            else
            {
                const int candidate = insts[start].c;

                if (candidate >= 0
                    && insts[start].a != candidate
                    && insts[start].b != candidate
                    && (fn.laneCountOf (candidate) > 1 || allowScalar)
                    && fn.valueTypes[static_cast<size_t> (candidate)] == YdspValueType::float32Type)
                {
                    if (insts[start].result == candidate)
                        accumulator = candidate;
                    else if (start + 1 < insts.size()
                             && insts[start + 1].op == YdspIrOp::movF
                             && insts[start + 1].result == candidate
                             && insts[start + 1].a == insts[start].result)
                        accumulator = candidate;
                }
            }

            if (accumulator >= 0)
            {
                std::vector<Link> links;
                std::vector<int> linkAddends;

                for (size_t i = start; i < insts.size(); ++i)
                {
                    const auto& inst = insts[i];

                    const auto reads =
                        (isValueIdOperand (inst.op, 0) && inst.a == accumulator)
                        || (isValueIdOperand (inst.op, 1) && inst.b == accumulator)
                        || (isValueIdOperand (inst.op, 2) && inst.c == accumulator);

                    if (! reads && inst.result != accumulator)
                        continue;

                    const bool isAddLink = inst.op == YdspIrOp::addF
                                        && inst.a != inst.b
                                        && (inst.a == accumulator || inst.b == accumulator);
                    const bool isFmaLink = allowScalar
                                        && inst.op == YdspIrOp::fmaF
                                        && inst.c == accumulator
                                        && inst.a != accumulator
                                        && inst.b != accumulator;

                    if (! isAddLink && ! isFmaLink)
                        break;

                    const int addend = isAddLink ? (inst.a == accumulator ? inst.b : inst.a) : -1;

                    if (inst.result == accumulator)
                    {
                        links.push_back ({ i, i });
                        linkAddends.push_back (addend);
                        continue;
                    }

                    if (i + 1 < insts.size()
                        && insts[i + 1].op == YdspIrOp::movF
                        && insts[i + 1].result == accumulator
                        && insts[i + 1].a == inst.result
                        && ! hasForeignReader (i))
                    {
                        links.push_back ({ i, i + 1 });
                        linkAddends.push_back (addend);
                        ++i;
                        continue;
                    }

                    break;
                }

                if (links.size() >= minChainLength)
                {
                    rewriteChain (accumulator, accumulator, links, linkAddends);
                    continue;
                }
            }

            // Single-def addF chain (for example after copy propagation or
            // state write-back folding): t0 = a + b; t1 = t0 + c; ...
            if (startOp != YdspIrOp::addF || insts[start].result < 0
                || insts[start].a < 0 || insts[start].b < 0 || insts[start].a == insts[start].b)
                continue;

            const int startResult = insts[start].result;
            const int startLanes = fn.laneCountOf (startResult);

            if ((startLanes <= 1 && ! allowScalar)
                || fn.valueTypes[static_cast<size_t> (startResult)] != YdspValueType::float32Type
                || fn.laneCountOf (insts[start].a) != startLanes
                || fn.laneCountOf (insts[start].b) != startLanes)
                continue;

            if (! counts.has_value())
                counts.emplace (fn);

            if (! counts->definedOnce (startResult) || counts->useCount (startResult) != 1)
                continue;

            std::vector<Link> singleDefLinks { { start, start } };
            std::vector<int> singleDefAddends { insts[start].b };
            int currentSum = startResult;

            for (size_t i = start + 1; i < insts.size(); ++i)
            {
                const auto& inst = insts[i];

                const auto reads =
                    (isValueIdOperand (inst.op, 0) && inst.a == currentSum)
                    || (isValueIdOperand (inst.op, 1) && inst.b == currentSum)
                    || (isValueIdOperand (inst.op, 2) && inst.c == currentSum);

                if (! reads)
                    continue;

                if (inst.op == YdspIrOp::addF
                    && (inst.a == currentSum) != (inst.b == currentSum)
                    && inst.result >= 0
                    && inst.result != currentSum
                    && static_cast<size_t> (inst.result) < fn.valueTypes.size()
                    && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type
                    && fn.laneCountOf (inst.result) == startLanes
                    && fn.laneCountOf (inst.a) == startLanes
                    && fn.laneCountOf (inst.b) == startLanes
                    && counts->definedOnce (inst.result))
                {
                    singleDefLinks.push_back ({ i, i });
                    singleDefAddends.push_back (inst.a == currentSum ? inst.b : inst.a);
                    currentSum = inst.result;

                    if (counts->useCount (currentSum) != 1)
                        break;

                    continue;
                }

                break;
            }

            if (singleDefLinks.size() >= minChainLength)
                rewriteChain (currentSum, insts[start].a, singleDefLinks, singleDefAddends);
        }
    }
}
} // namespace yup
