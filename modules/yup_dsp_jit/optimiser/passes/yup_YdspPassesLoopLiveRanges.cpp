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

void YdspOptimizer::localizeHighPressureStateIndices (YdspIrFunction& fn)
{
    if (! fn.isSampleMode || fn.isEventHandler || fn.isInit || fn.blocks.empty())
        return;

    const auto numBlocks = static_cast<int> (fn.blocks.size());
    const size_t keepCount = (targetHasFusedMultiplyAdd && vectorWidth <= 4) ? 14u : 6u;

    const auto sampleLoop = std::find_if (fn.loops.begin(), fn.loops.end(), [numBlocks] (const YdspIrLoop& loop)
    {
        return loop.bound.kind == YdspLoopBoundKind::blockSize
            && ! loop.unrolled
            && loop.headerBlock >= 1
            && loop.exitBlock == loop.headerBlock + 2
            && loop.exitBlock < numBlocks;
    });

    if (sampleLoop == fn.loops.end())
        return;

    const int header = sampleLoop->headerBlock;
    const int body = header + 1;
    const int exit = sampleLoop->exitBlock;

    if (fn.blocks[static_cast<size_t> (body)].term != YdspIrTerm::branch
        || fn.blocks[static_cast<size_t> (body)].termTarget != header)
        return;

    // Collect loop-carried int32 state indices loaded before the loop and
    // stored back at exit.
    struct CarriedIndex
    {
        int reg = -1;
        int slot = -1;
        int loadBlock = -1;
        size_t loadInstIdx = 0;
        size_t storeExitInstIdx = 0;
        size_t firstUseInBody = 0;
        size_t lastDefInBody = 0;
    };

    std::unordered_map<int, int> totalDefs;
    for (int b = 0; b < numBlocks; ++b)
        for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
            if (inst.result >= 0)
                ++totalDefs[inst.result];

    auto& exitInsts = fn.blocks[static_cast<size_t> (exit)].insts;
    std::vector<CarriedIndex> candidates;

    for (int b = 0; b < header; ++b)
    {
        const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;

        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& inst = insts[i];
            if (inst.op != YdspIrOp::loadStateI || inst.result < 0)
                continue;

            const int reg = inst.result;
            const int slot = inst.a;

            if (reg == sampleLoop->induction || static_cast<size_t> (reg) >= fn.valueTypes.size()
                || fn.valueTypes[static_cast<size_t> (reg)] != YdspValueType::int32Type)
                continue;

            // Find matching storeStateI in exit block.
            int storeIdx = -1;
            int storeCount = 0;
            for (size_t s = 0; s < exitInsts.size(); ++s)
            {
                if (exitInsts[s].op == YdspIrOp::storeStateI && exitInsts[s].a == reg && exitInsts[s].memIndex == slot)
                {
                    storeIdx = static_cast<int> (s);
                    ++storeCount;
                }
            }

            if (storeCount != 1 || storeIdx < 0)
                continue;

            // Ensure reg is not used outside { preheader load, body, exit store }.
            bool usedElsewhere = false;
            for (int ob = 0; ob < numBlocks && ! usedElsewhere; ++ob)
            {
                if (ob == body)
                    continue;

                const auto& oBlock = fn.blocks[static_cast<size_t> (ob)];
                if (oBlock.term == YdspIrTerm::branchIf && oBlock.termCond == reg)
                {
                    usedElsewhere = true;
                    break;
                }

                for (size_t oi = 0; oi < oBlock.insts.size(); ++oi)
                {
                    if (ob == b && oi == i)
                        continue;
                    if (ob == exit && oi == static_cast<size_t> (storeIdx))
                        continue;

                    const auto& oInst = oBlock.insts[oi];
                    if (oInst.result == reg
                        || (isValueIdOperand (oInst.op, 0) && oInst.a == reg)
                        || (isValueIdOperand (oInst.op, 1) && oInst.b == reg)
                        || (isValueIdOperand (oInst.op, 2) && oInst.c == reg))
                    {
                        usedElsewhere = true;
                        break;
                    }
                }
            }

            if (usedElsewhere)
                continue;

            // Inspect body span for reg.
            const auto& bodyInsts = fn.blocks[static_cast<size_t> (body)].insts;
            size_t firstUse = bodyInsts.size();
            size_t lastTouch = 0;
            int lastDef = -1;
            bool slotConflict = false;

            for (size_t bi = 0; bi < bodyInsts.size(); ++bi)
            {
                const auto& bInst = bodyInsts[bi];
                if ((bInst.op == YdspIrOp::loadStateI && bInst.a == slot)
                    || (bInst.op == YdspIrOp::storeStateI && bInst.memIndex == slot))
                    slotConflict = true;

                const bool readsReg = (isValueIdOperand (bInst.op, 0) && bInst.a == reg)
                                   || (isValueIdOperand (bInst.op, 1) && bInst.b == reg)
                                   || (isValueIdOperand (bInst.op, 2) && bInst.c == reg);
                const bool writesReg = bInst.result == reg;

                if (readsReg || writesReg)
                {
                    if (firstUse == bodyInsts.size())
                    {
                        if (! readsReg)
                            slotConflict = true; // must read before overwriting
                        firstUse = bi;
                    }

                    lastTouch = bi;
                    if (writesReg)
                        lastDef = static_cast<int> (bi);
                }
            }

            if (slotConflict || firstUse >= bodyInsts.size() || lastDef < 0
                || static_cast<size_t> (lastDef) != lastTouch
                || (lastTouch - firstUse) > 16)
                continue;

            candidates.push_back ({ reg, slot, b, i, static_cast<size_t> (storeIdx), firstUse, static_cast<size_t> (lastDef) });
        }
    }

    if (candidates.size() <= keepCount)
        return;

    // Keep the first `keepCount` indices in registers across the loop and
    // localize the excess slots around their short in-body spans.
    std::vector<CarriedIndex> toLocalize (candidates.begin() + static_cast<ptrdiff_t> (keepCount), candidates.end());

    // Remove preheader loads and exit stores (sort descending by index per block).
    std::unordered_map<int, std::vector<size_t>> loadRemovals;
    std::vector<size_t> exitRemovals;

    for (const auto& c : toLocalize)
    {
        loadRemovals[c.loadBlock].push_back (c.loadInstIdx);
        exitRemovals.push_back (c.storeExitInstIdx);
    }

    for (auto& [b, idxs] : loadRemovals)
    {
        std::sort (idxs.rbegin(), idxs.rend());
        auto& insts = fn.blocks[static_cast<size_t> (b)].insts;
        for (const size_t idx : idxs)
            insts.erase (insts.begin() + static_cast<ptrdiff_t> (idx));
    }

    std::sort (exitRemovals.rbegin(), exitRemovals.rend());
    for (const size_t idx : exitRemovals)
        exitInsts.erase (exitInsts.begin() + static_cast<ptrdiff_t> (idx));

    // Rewrite body in one linear pass, inserting local load before firstUse
    // and store after lastDef.
    std::unordered_map<size_t, std::vector<const CarriedIndex*>> loadsBefore;
    std::unordered_map<size_t, std::vector<const CarriedIndex*>> storesAfter;

    for (const auto& c : toLocalize)
    {
        loadsBefore[c.firstUseInBody].push_back (&c);
        storesAfter[c.lastDefInBody].push_back (&c);
    }

    auto& bodyInsts = fn.blocks[static_cast<size_t> (body)].insts;
    std::vector<YdspIrInst> updatedBody;
    updatedBody.reserve (bodyInsts.size() + toLocalize.size() * 2);

    for (size_t bi = 0; bi < bodyInsts.size(); ++bi)
    {
        if (const auto it = loadsBefore.find (bi); it != loadsBefore.end())
            for (const auto* c : it->second)
                updatedBody.push_back ({ YdspIrOp::loadStateI, c->reg, c->slot });

        updatedBody.push_back (bodyInsts[bi]);

        if (const auto it = storesAfter.find (bi); it != storesAfter.end())
            for (const auto* c : it->second)
                updatedBody.push_back ({ YdspIrOp::storeStateI, -1, c->reg, -1, -1, c->slot });
    }

    bodyInsts = std::move (updatedBody);
}

} // namespace yup
