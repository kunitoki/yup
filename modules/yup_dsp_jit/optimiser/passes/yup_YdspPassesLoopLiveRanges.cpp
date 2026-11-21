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

void YdspOptimizer::splitLiveRangesAtLoops (YdspIrFunction& fn)
{
    if (fn.isEventHandler || fn.isInit)
        return;

    const auto numBlocks = static_cast<int> (fn.blocks.size());

    const auto mayCall = [] (const YdspIrBlock& block)
    {
        return std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst)
        {
            return isCallBoundary (inst.op) || inst.op == YdspIrOp::expF;
        });
    };

    const auto isRecomputable = [] (const YdspIrInst& inst)
    {
        return isConstantOp (inst.op) || inst.op == YdspIrOp::loadSampleRate || inst.op == YdspIrOp::loadBlockSize;
    };

    for (size_t loopIndex = 0; loopIndex < fn.loops.size(); ++loopIndex)
    {
        const auto loop = fn.loops[loopIndex];
        const int header = loop.headerBlock;
        const int exit = loop.exitBlock;
        const int preheader = header - 1;

        if (loop.unrolled || preheader < 0 || exit <= header || exit >= numBlocks)
            continue;

        const auto inside = [header, exit] (int block) { return block >= header && block < exit; };

        const bool nested = std::any_of (fn.loops.begin(), fn.loops.end(), [&] (const YdspIrLoop& other)
        {
            return ! other.unrolled && other.id != loop.id
                && other.headerBlock <= header && exit <= other.exitBlock
                && (other.headerBlock != header || other.exitBlock != exit);
        });

        if (nested)
            continue;

        // Only worth it when a call ahead of the loop forces what is live
        // across it into the few registers a call preserves, and the loop
        // itself has no call that would do the same to the copies.
        bool callBefore = false, callInside = false;

        for (int b = 0; b < exit; ++b)
            (inside (b) ? callInside : callBefore) |= mayCall (fn.blocks[static_cast<size_t> (b)]);

        if (! callBefore || callInside)
            continue;

        // Entered from the preheader only and left through the header only,
        // so the copies made on the way in and out are always executed.
        bool wellFormed = true;

        for (int b = 0; b < numBlocks && wellFormed; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];

            int targets[2] { -1, -1 };

            if (block.term == YdspIrTerm::fallthrough)
                targets[0] = b + 1 < numBlocks ? b + 1 : -1;
            else
                targets[0] = block.termTarget;

            if (block.term == YdspIrTerm::branchIf)
                targets[1] = block.termTarget2;

            for (const int target : targets)
            {
                if (target < 0)
                    continue;

                if (inside (b))
                    wellFormed &= inside (target) || (b == header && target == exit);
                else
                    wellFormed &= (! inside (target) && target != exit) || (b == preheader && target == header);
            }
        }

        if (! wellFormed)
            continue;

        const auto numValues = fn.valueTypes.size();
        fn.valueLanes.resize (numValues, 1);

        std::vector<int> defsInside (numValues, 0), defsBefore (numValues, 0), defsAfter (numValues, 0);
        std::vector<char> usedInside (numValues, 0), usedOutside (numValues, 0);
        std::vector<const YdspIrInst*> definition (numValues, nullptr);

        for (int b = 0; b < numBlocks; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];

            for (const auto& inst : block.insts)
            {
                if (inst.result < 0)
                    continue;

                const auto result = static_cast<size_t> (inst.result);
                ++(inside (b) ? defsInside : (b < header ? defsBefore : defsAfter))[result];
                definition[result] = &inst;
            }
        }

        forEachValueUse (fn, [&] (int block, int, int, int value)
        {
            (inside (block) ? usedInside : usedOutside)[static_cast<size_t> (value)] = 1;
        });

        std::vector<int> replacement (numValues, -1);
        std::vector<YdspIrInst> entering, leaving;

        for (size_t value = 0; value < numValues; ++value)
        {
            const bool crossesIn = defsBefore[value] > 0 && (usedInside[value] || defsInside[value] > 0);
            const bool crossesOut = defsInside[value] > 0 && (usedOutside[value] || defsBefore[value] > 0 || defsAfter[value] > 0);

            if ((! crossesIn && ! crossesOut) || static_cast<int> (value) == loop.induction)
                continue;

            const auto type = fn.valueTypes[value];
            const auto lanes = fn.valueLanes[value];

            // A value first written after the loop has nothing to bring in,
            // and packed masks have no move.
            if ((defsInside[value] == 0 && (defsBefore[value] != 1 || defsAfter[value] != 0))
                || (lanes > 1 && ! isFloatValueType (type)))
                continue;

            const auto fresh = static_cast<int> (fn.valueTypes.size());
            fn.valueTypes.push_back (type);
            fn.valueLanes.push_back (lanes);
            replacement[value] = fresh;

            const auto move = moveOpcodeFor (type);

            if (defsInside[value] == 0 && lanes == 1 && isRecomputable (*definition[value]))
            {
                auto again = *definition[value];
                again.result = fresh;
                entering.push_back (again);
            }
            else if (defsBefore[value] > 0)
            {
                entering.push_back ({ move, fresh, static_cast<int> (value) });
            }

            if (crossesOut)
                leaving.push_back ({ move, static_cast<int> (value), fresh });
        }

        if (entering.empty() && leaving.empty())
            continue;

        const auto renamed = [&replacement] (int value)
        {
            return value >= 0 && static_cast<size_t> (value) < replacement.size() && replacement[static_cast<size_t> (value)] >= 0
                     ? replacement[static_cast<size_t> (value)]
                     : value;
        };

        for (int b = header; b < exit; ++b)
        {
            auto& block = fn.blocks[static_cast<size_t> (b)];

            for (auto& inst : block.insts)
            {
                if (isValueIdOperand (inst.op, 0))
                    inst.a = renamed (inst.a);

                if (isValueIdOperand (inst.op, 1))
                    inst.b = renamed (inst.b);

                if (isValueIdOperand (inst.op, 2))
                    inst.c = renamed (inst.c);

                inst.result = renamed (inst.result);
            }

            block.termCond = renamed (block.termCond);
        }

        auto& before = fn.blocks[static_cast<size_t> (preheader)].insts;
        before.insert (before.end(), entering.begin(), entering.end());

        auto& after = fn.blocks[static_cast<size_t> (exit)].insts;
        after.insert (after.begin(), leaving.begin(), leaving.end());
    }
}

} // namespace yup
