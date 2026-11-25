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

bool isStateArrayStore (YdspIrOp op) noexcept
{
    return op == YdspIrOp::storeStateArrayF || op == YdspIrOp::storeStateArrayI;
}

YdspIrOp matchingStoreFor (YdspIrOp loadOp) noexcept
{
    return loadOp == YdspIrOp::loadStateArrayF ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI;
}

} // namespace

//==============================================================================

bool YdspOptimizer::storeToLoadForwarding (YdspIrFunction& fn)
{
    bool changed = false;
    const auto types = resolveValueTypes (fn);
    const size_t numValues = types.size();

    bool hasAnyStore = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (isStateArrayStore (inst.op))
                hasAnyStore = true;

    if (! hasAnyStore)
        return false;

    std::vector<int> defCount (numValues, 0);
    std::vector<int> defBlock (numValues, -1);
    std::vector<std::optional<int64_t>> globalConstants (numValues);

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        for (const auto& inst : fn.blocks[b].insts)
        {
            if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues)
                continue;

            const auto id = static_cast<size_t> (inst.result);
            ++defCount[id];
            defBlock[id] = static_cast<int> (b);
        }
    }

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues)
                continue;

            const auto id = static_cast<size_t> (inst.result);

            if (defCount[id] == 1 && inst.op == YdspIrOp::constI && fn.laneCountOf (inst.result) == 1
                && inst.ivalue >= 0 && inst.ivalue <= std::numeric_limits<int32_t>::max())
                globalConstants[id] = inst.ivalue;
        }
    }

    const detail::YdspDomTree dom (fn);

    for (size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex)
    {
        auto& insts = fn.blocks[blockIndex].insts;

        std::vector<std::optional<int64_t>> constants = globalConstants;
        std::vector<std::optional<int64_t>> indices (insts.size());

        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& inst = insts[i];

            if ((isStateArrayStore (inst.op) || inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI)
                && inst.a >= 0 && static_cast<size_t> (inst.a) < numValues)
                indices[i] = constants[static_cast<size_t> (inst.a)];

            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
            {
                const auto id = static_cast<size_t> (inst.result);

                if (defCount[id] != 1)
                {
                    auto& value = constants[id];
                    value.reset();

                    if (inst.op == YdspIrOp::constI && fn.laneCountOf (inst.result) == 1
                        && inst.ivalue >= 0 && inst.ivalue <= std::numeric_limits<int32_t>::max())
                        value = inst.ivalue;
                }
            }
        }

        for (size_t j = 0; j < insts.size(); ++j)
        {
            auto& load = insts[j];

            if (load.op != YdspIrOp::loadStateArrayF && load.op != YdspIrOp::loadStateArrayI)
                continue;

            if (load.result < 0 || load.a < 0)
                continue;

            const auto storeOp = matchingStoreFor (load.op);
            const auto type = types[static_cast<size_t> (load.result)];
            const auto lanes = fn.laneCountOf (load.result);

            size_t source = 0;
            bool foundLocal = false;
            bool blockedLocal = false;

            for (size_t k = j; k-- > 0;)
            {
                const auto& previous = insts[k];

                if (previous.result >= 0 && previous.result == load.a)
                {
                    if (indices[j].has_value() && previous.op == YdspIrOp::constI
                        && defCount[static_cast<size_t> (previous.result)] == 1)
                        continue;

                    blockedLocal = true;
                    break;
                }

                if (previous.op == YdspIrOp::emitEvent)
                {
                    blockedLocal = true;
                    break;
                }

                if (! isStateArrayStore (previous.op))
                    continue;

                if (previous.op != storeOp || previous.b < 0 || types[static_cast<size_t> (previous.b)] != type)
                {
                    blockedLocal = true;
                    break;
                }

                int64_t storeStart = previous.memIndex;
                int64_t loadStart = load.memIndex;

                if (indices[k].has_value() && indices[j].has_value())
                {
                    storeStart += *indices[k];
                    loadStart += *indices[j];
                }
                else if (previous.a != load.a)
                {
                    blockedLocal = true;
                    break;
                }

                const auto storedLanes = fn.laneCountOf (previous.b);

                if (storeStart == loadStart && storedLanes == lanes)
                {
                    source = k;
                    foundLocal = true;
                    break;
                }

                if (storeStart < loadStart + lanes && loadStart < storeStart + storedLanes)
                {
                    blockedLocal = true;
                    break;
                }
            }

            if (foundLocal)
            {
                const auto stored = insts[source].b;

                if (stored < 0 || stored == load.result)
                    continue;

                bool valueStable = true;

                for (size_t k = source + 1; k < j && valueStable; ++k)
                    valueStable = insts[k].result < 0 || insts[k].result != stored;

                if (! valueStable)
                    continue;

                changed = true;
                load.op = (load.op == YdspIrOp::loadStateArrayF) ? YdspIrOp::movF : YdspIrOp::movI;
                load.a = stored;
                load.b = -1;
                load.c = -1;
                load.memIndex = -1;
                continue;
            }

            if (blockedLocal || ! dom.isReachable (static_cast<int> (blockIndex)))
                continue;

            const auto loadConstIdx = static_cast<size_t> (load.a) < numValues
                                          ? globalConstants[static_cast<size_t> (load.a)]
                                          : std::nullopt;
            const bool loadIndexSingleDef = static_cast<size_t> (load.a) < numValues
                                         && defCount[static_cast<size_t> (load.a)] == 1;

            if (! loadConstIdx.has_value() && ! loadIndexSingleDef)
                continue;

            const auto checkStoreAgainstLoad = [&] (const YdspIrInst& prev, bool& exactMatch) -> bool
            {
                exactMatch = false;

                if (prev.op == YdspIrOp::emitEvent)
                    return false;

                if (! isStateArrayStore (prev.op))
                    return true;

                if (prev.op != storeOp || prev.b < 0 || static_cast<size_t> (prev.b) >= numValues
                    || types[static_cast<size_t> (prev.b)] != type)
                    return false;

                const auto prevConstIdx = (prev.a >= 0 && static_cast<size_t> (prev.a) < numValues)
                                              ? globalConstants[static_cast<size_t> (prev.a)]
                                              : std::nullopt;

                int64_t storeStart = prev.memIndex;
                int64_t loadStart = load.memIndex;

                if (prevConstIdx.has_value() && loadConstIdx.has_value())
                {
                    storeStart += *prevConstIdx;
                    loadStart += *loadConstIdx;
                }
                else if (! (prev.a == load.a && loadIndexSingleDef))
                {
                    return false;
                }

                const auto storedLanes = fn.laneCountOf (prev.b);

                if (storeStart == loadStart && storedLanes == lanes)
                {
                    exactMatch = true;
                    return true;
                }

                return ! (storeStart < loadStart + lanes && loadStart < storeStart + storedLanes);
            };

            const int curBlock = static_cast<int> (blockIndex);
            int d = dom.idom[blockIndex];

            if (d == curBlock)
                continue;

            while (d >= 0)
            {
                const auto& domInsts = fn.blocks[static_cast<size_t> (d)].insts;
                bool domBlocked = false;
                int foundStoreValue = -1;

                for (size_t k = domInsts.size(); k-- > 0;)
                {
                    const auto& prev = domInsts[k];

                    if (prev.result >= 0 && prev.result == load.a && ! loadIndexSingleDef)
                    {
                        domBlocked = true;
                        break;
                    }

                    bool exactMatch = false;

                    if (! checkStoreAgainstLoad (prev, exactMatch))
                    {
                        domBlocked = true;
                        break;
                    }

                    if (exactMatch)
                    {
                        foundStoreValue = prev.b;
                        break;
                    }
                }

                if (domBlocked)
                    break;

                if (foundStoreValue >= 0)
                {
                    const bool storedSafe = foundStoreValue != load.result
                                         && static_cast<size_t> (foundStoreValue) < numValues
                                         && defCount[static_cast<size_t> (foundStoreValue)] == 1
                                         && dom.dominates (defBlock[static_cast<size_t> (foundStoreValue)], curBlock);

                    if (! storedSafe)
                        break;

                    // Verify no intermediate CFG path from `d` to `curBlock` (including
                    // loop backedges) contains an aliasing store or event emission.
                    bool pathClear = true;
                    std::vector<char> visited (fn.blocks.size(), 0);
                    std::vector<int> worklist = dom.preds[static_cast<size_t> (curBlock)];

                    for (const int p : worklist)
                        visited[static_cast<size_t> (p)] = 1;

                    while (! worklist.empty() && pathClear)
                    {
                        const int m = worklist.back();
                        worklist.pop_back();

                        if (m == d)
                            continue;

                        const auto& midInsts = fn.blocks[static_cast<size_t> (m)].insts;
                        const size_t startIdx = (m == curBlock) ? (j + 1) : 0;

                        for (size_t k = startIdx; k < midInsts.size(); ++k)
                        {
                            bool dummyExact = false;

                            if (! checkStoreAgainstLoad (midInsts[k], dummyExact) || dummyExact)
                            {
                                pathClear = false;
                                break;
                            }
                        }

                        if (! pathClear)
                            break;

                        for (const int p : dom.preds[static_cast<size_t> (m)])
                        {
                            if (! visited[static_cast<size_t> (p)])
                            {
                                visited[static_cast<size_t> (p)] = 1;
                                worklist.push_back (p);
                            }
                        }
                    }

                    if (pathClear)
                    {
                        changed = true;
                        load.op = (load.op == YdspIrOp::loadStateArrayF) ? YdspIrOp::movF : YdspIrOp::movI;
                        load.a = foundStoreValue;
                        load.b = -1;
                        load.c = -1;
                        load.memIndex = -1;
                    }

                    break;
                }

                if (d == 0)
                    break;

                d = dom.idom[static_cast<size_t> (d)];
            }
        }
    }

    return changed;
}
} // namespace yup
