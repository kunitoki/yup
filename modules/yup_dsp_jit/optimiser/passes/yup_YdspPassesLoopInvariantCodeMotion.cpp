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

namespace
{

/** The blocks control can reach directly from `index`. */
void collectSuccessors (const YdspIrFunction& fn, int index, std::vector<int>& out)
{
    out.clear();

    const auto& block = fn.blocks[static_cast<size_t> (index)];

    switch (block.term)
    {
        case YdspIrTerm::fallthrough:
            if (index + 1 < static_cast<int> (fn.blocks.size()))
                out.push_back (index + 1);

            break;

        case YdspIrTerm::branch:
            if (block.termTarget >= 0)
                out.push_back (block.termTarget);

            break;

        case YdspIrTerm::branchIf:
            if (block.termTarget >= 0)
                out.push_back (block.termTarget);

            if (block.termTarget2 >= 0)
                out.push_back (block.termTarget2);

            break;
    }
}

/** Computes the dominator sets of every block: `dominators[b][d]` is true when
    every path from the entry to `b` passes through `d`.

    Plain iterate-to-fixpoint over `dom(b) = {b} u (intersection of dom(p))`.
    Kernels have tens of blocks, so the quadratic representation is far cheaper
    than the bookkeeping a dominator tree would need. */
std::vector<std::vector<char>> computeDominators (const YdspIrFunction& fn)
{
    const auto count = fn.blocks.size();

    std::vector<std::vector<int>> predecessors (count);
    std::vector<int> successors;

    for (size_t b = 0; b < count; ++b)
    {
        collectSuccessors (fn, static_cast<int> (b), successors);

        for (const int successor : successors)
            predecessors[static_cast<size_t> (successor)].push_back (static_cast<int> (b));
    }

    // Everything starts dominated by everything, except the entry, which is
    // dominated only by itself; the intersections then whittle it down.
    std::vector<std::vector<char>> dominators (count, std::vector<char> (count, 1));

    if (count > 0)
    {
        std::fill (dominators[0].begin(), dominators[0].end(), 0);
        dominators[0][0] = 1;
    }

    for (bool changed = true; changed;)
    {
        changed = false;

        for (size_t b = 1; b < count; ++b)
        {
            std::vector<char> updated (count, 0);

            if (! predecessors[b].empty())
            {
                std::fill (updated.begin(), updated.end(), 1);

                for (const int predecessor : predecessors[b])
                    for (size_t d = 0; d < count; ++d)
                        updated[d] = static_cast<char> (updated[d] && dominators[static_cast<size_t> (predecessor)][d]);
            }

            updated[b] = 1;

            if (updated != dominators[b])
            {
                dominators[b] = std::move (updated);
                changed = true;
            }
        }
    }

    return dominators;
}

} // namespace

//==============================================================================

void YdspOptimizer::loopInvariantCodeMotion (YdspIrFunction& fn)
{
    // Event-handler functions run once per event, so hoisting inside them buys
    // nothing measurable and is left alone.
    if (fn.isEventHandler || fn.blocks.empty())
        return;

    // Parameter slots this function writes: a load from one of those is not
    // immutable across the call and must stay where it is.
    std::unordered_set<int> storedParamSlots;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::storeParam)
                storedParamSlots.insert (inst.memIndex);

    std::unordered_map<int, int> definitionCount;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0)
                ++definitionCount[inst.result];

    const auto dominators = computeDominators (fn);

    // Innermost loops come first in fn.loops (a nested loop finishes lowering
    // before its parent), so hoisting in order lets a value climb out of an
    // inner loop and then out of the enclosing one on the same pass.
    for (const auto& loop : fn.loops)
    {
        const int loopStart = loop.headerBlock;
        const int loopEnd = loop.exitBlock;

        // Every loop is preceded by its own preheader: the sample loop by the
        // function prologue, a `for` by the block holding its induction
        // initialiser. Construction is CFG-linear, so that block is always the
        // one immediately before the header - no block has to be inserted, and
        // the layout the wasm backend depends on is untouched.
        const int preheader = loopStart - 1;

        if (preheader < 0 || loopEnd > static_cast<int> (fn.blocks.size()))
            continue;

        const auto& preheaderDominators = dominators[static_cast<size_t> (preheader)];

        // A value can be read at the end of the preheader when every one of its
        // definitions sits in a block dominating it. Collect the complement
        // once: anything defined anywhere else, which by construction includes
        // everything assigned inside the loop (no loop block dominates its own
        // preheader).
        std::unordered_set<int> unavailable;

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            if (preheaderDominators[b])
                continue;

            for (const auto& inst : fn.blocks[b].insts)
                if (inst.result >= 0)
                    unavailable.insert (inst.result);
        }

        const auto isAvailableAtPreheader = [&unavailable] (int value)
        {
            return value < 0 || unavailable.count (value) == 0;
        };

        for (bool changed = true; changed;)
        {
            changed = false;

            for (int block = loopStart; block < loopEnd; ++block)
            {
                auto& insts = fn.blocks[static_cast<size_t> (block)].insts;

                for (size_t i = 0; i < insts.size(); ++i)
                {
                    const auto inst = insts[i];

                    if (inst.result < 0 || ! hasValueResult (inst.op))
                        continue;

                    // Hoisting one of several definitions of a register would
                    // let it win unconditionally. This is also what keeps a
                    // promoted state register - deliberately written by both the
                    // prologue load and the per-sample mov - pinned inside the
                    // sample loop.
                    if (definitionCount[inst.result] != 1)
                        continue;

                    // Memory the loop body can write behind our back.
                    if (inst.op == YdspIrOp::loadInput
                        || inst.op == YdspIrOp::loadOutput
                        || inst.op == YdspIrOp::loadParamOut
                        || inst.op == YdspIrOp::loadStateF || inst.op == YdspIrOp::loadStateI
                        || inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI)
                        continue;

                    // Parameters are immutable within a kernel call unless this
                    // body writes them: automation lands between calls.
                    if (inst.op == YdspIrOp::loadParam && storedParamSlots.count (inst.a) != 0)
                        continue;

                    // A slot index is not a value id, so it is never looked up.
                    const auto operandAvailable = [&] (int operand, int value)
                    {
                        return ! isValueIdOperand (inst.op, operand) || isAvailableAtPreheader (value);
                    };

                    if (! operandAvailable (0, inst.a) || ! operandAvailable (1, inst.b) || ! operandAvailable (2, inst.c))
                        continue;

                    // Append, so it lands after everything the preheader already
                    // defines (including anything hoisted just before it).
                    fn.blocks[static_cast<size_t> (preheader)].insts.push_back (inst);

                    // Its result is now defined in the preheader, so anything in
                    // the loop that was waiting on it can hoist next round -
                    // and, because it only becomes available once it has moved,
                    // never lands ahead of it.
                    unavailable.erase (inst.result);

                    insts.erase (insts.begin() + static_cast<std::ptrdiff_t> (i));
                    --i;

                    changed = true;
                }
            }
        }
    }
}
} // namespace yup
