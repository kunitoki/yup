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
    // Below four links there is nothing to halve: three adds become two plus a
    // combine, which is the same depth for one more instruction. It is also
    // what stops the repeated halving below, since n links leave n/2 + 1.
    constexpr size_t minChainLength = 4;

    // Only a widened accumulator is touched. Its association has *already* been
    // changed by the vectoriser - lane j sums elements j, j+4, j+8 ... and the
    // lanes are folded pairwise - and that is documented as not bit-exact. A
    // scalar chain carries no such licence, so splitting one would newly break
    // a promise the language makes.
    if (! fn.vectorized || fn.valueLanes.empty())
        return;

    const auto newAccumulator = [&fn] (int like)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (fn.valueTypes[static_cast<size_t> (like)]);
        fn.valueLanes.push_back (fn.laneCountOf (like));

        return id;
    };

    // One link of the chain. The vectoriser leaves an accumulation as the pair
    // `addF t, acc, x` / `movF acc, t` rather than a self-add, so a link is two
    // instructions in general and one when something upstream has collapsed it.
    struct Link
    {
        size_t addIndex = 0;
        size_t moveIndex = 0; // == addIndex when the add writes the accumulator
    };

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;

        for (size_t start = 0; start < insts.size(); ++start)
        {
            if (insts[start].op != YdspIrOp::addF)
                continue;

            // The accumulator is whichever operand the add reads and, one way
            // or another, writes back.
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

            // Collect the run of accumulations that begins here.
            std::vector<Link> links;

            for (size_t i = start; i < insts.size(); ++i)
            {
                const auto& inst = insts[i];

                // Reading the accumulator part-way through would observe a
                // partial sum that the split no longer produces. A slot index
                // is not a value id, so it cannot be that read.
                const auto reads =
                    (isValueIdOperand (inst.op, 0) && inst.a == accumulator)
                    || (isValueIdOperand (inst.op, 1) && inst.b == accumulator)
                    || (isValueIdOperand (inst.op, 2) && inst.c == accumulator);

                if (! reads && inst.result != accumulator)
                    continue; // ordinary body work between two links

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
                    ++i; // the move is part of this link
                    continue;
                }

                break; // anything else ends the chain
            }

            if (links.size() < minChainLength)
                continue;

            // ---- Rewrite ----
            // Odd links move to a second accumulator, so the two run side by
            // side and the serial depth halves. Every link stays where it was,
            // so each addend is still computed before the add that reads it.
            const auto second = newAccumulator (accumulator);
            bool startedSecond = false;

            for (size_t k = 1; k < links.size(); k += 2)
            {
                auto& add = insts[links[k].addIndex];
                const auto addend = add.a == accumulator ? add.b : add.a;

                if (! startedSecond)
                {
                    // The second accumulator has no zero to start from - it
                    // simply *is* its first addend.
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

            // Deliberately not skipping past the chain. Two accumulators in one
            // loop interleave their links, so jumping to the end of the first
            // one's chain would step over every start position of the second.
            //
            // Rescanning also lets a long chain halve more than once. The scan
            // resumes one instruction along, so it lands inside the chain just
            // rewritten and picks up the *suffix* still on this accumulator:
            // for eight links that is the last three even ones plus the
            // combine, four in all, which halves again. Eight links therefore
            // end at depth four, not at the depth a balanced tree would reach -
            // restarting the block after each split would go further, and is
            // untried. It terminates because each pass leaves a shorter suffix.
        }
    }
}
} // namespace yup
