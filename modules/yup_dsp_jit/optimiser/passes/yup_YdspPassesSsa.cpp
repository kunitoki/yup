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

int newSsaValue (YdspIrFunction& fn, YdspValueType type, int lanes)
{
    const int id = static_cast<int> (fn.valueTypes.size());
    fn.valueTypes.push_back (type);

    if (! fn.valueLanes.empty())
        fn.valueLanes.push_back (lanes);

    return id;
}

int cloneSsaValue (YdspIrFunction& fn, int originalValue)
{
    const auto type = (originalValue >= 0 && static_cast<size_t> (originalValue) < fn.valueTypes.size())
                          ? fn.valueTypes[static_cast<size_t> (originalValue)]
                          : YdspValueType::float32Type;

    return newSsaValue (fn, type, fn.laneCountOf (originalValue));
}

void replaceValueUses (YdspIrFunction& fn, int fromValue, int toValue)
{
    for (auto& block : fn.blocks)
    {
        for (auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::phi)
            {
                for (auto& incoming : inst.phiIncoming)
                    if (incoming.second == fromValue)
                        incoming.second = toValue;

                continue;
            }

            if (isValueIdOperand (inst.op, 0) && inst.a == fromValue)
                inst.a = toValue;
            if (isValueIdOperand (inst.op, 1) && inst.b == fromValue)
                inst.b = toValue;
            if (isValueIdOperand (inst.op, 2) && inst.c == fromValue)
                inst.c = toValue;
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond == fromValue)
            block.termCond = toValue;
    }

    for (auto& loop : fn.loops)
        if (! loop.unrolled && loop.induction == fromValue)
            loop.induction = toValue;
}

bool simplifyTrivialPhis (YdspIrFunction& fn)
{
    bool anyChanged = false;

    for (bool changed = true; changed;)
    {
        changed = false;

        for (auto& block : fn.blocks)
        {
            for (size_t i = 0; i < block.insts.size();)
            {
                const auto& inst = block.insts[i];

                if (inst.op != YdspIrOp::phi)
                {
                    ++i;
                    continue;
                }

                int uniqueIncoming = -1;
                bool trivial = inst.result >= 0 && ! inst.phiIncoming.empty();

                for (const auto& [pred, val] : inst.phiIncoming)
                {
                    (void) pred;

                    if (val == inst.result)
                        continue;

                    if (val < 0)
                    {
                        trivial = false;
                        break;
                    }

                    if (uniqueIncoming < 0)
                    {
                        uniqueIncoming = val;
                    }
                    else if (uniqueIncoming != val)
                    {
                        trivial = false;
                        break;
                    }
                }

                if (trivial && uniqueIncoming >= 0)
                {
                    const int phiResult = inst.result;
                    block.insts.erase (block.insts.begin() + static_cast<std::ptrdiff_t> (i));
                    replaceValueUses (fn, phiResult, uniqueIncoming);
                    changed = true;
                    anyChanged = true;
                    continue;
                }

                ++i;
            }
        }
    }

    return anyChanged;
}

struct SsaEdgeCopy
{
    int dst = -1;
    int src = -1;
    YdspValueType type = YdspValueType::float32Type;
    int lanes = 1;
};

void emitParallelCopies (YdspIrFunction& fn, std::vector<SsaEdgeCopy> copies, std::vector<YdspIrInst>& out)
{
    copies.erase (std::remove_if (copies.begin(), copies.end(), [] (const SsaEdgeCopy& copy)
    {
        return copy.dst < 0 || copy.src < 0 || copy.dst == copy.src;
    }), copies.end());

    while (! copies.empty())
    {
        size_t readyIndex = copies.size();

        for (size_t i = 0; i < copies.size(); ++i)
        {
            const int dst = copies[i].dst;
            const bool usedAsSource = std::any_of (copies.begin(), copies.end(), [dst] (const SsaEdgeCopy& other)
            {
                return other.src == dst;
            });

            if (! usedAsSource)
            {
                readyIndex = i;
                break;
            }
        }

        if (readyIndex < copies.size())
        {
            const auto copy = copies[readyIndex];
            out.push_back ({ moveOpcodeFor (copy.type), copy.dst, copy.src });
            copies.erase (copies.begin() + static_cast<std::ptrdiff_t> (readyIndex));
        }
        else
        {
            const auto cycle = copies[0];
            const int temp = newSsaValue (fn, cycle.type, cycle.lanes);
            out.push_back ({ moveOpcodeFor (cycle.type), temp, cycle.dst });

            for (auto& entry : copies)
                if (entry.src == cycle.dst)
                    entry.src = temp;
        }
    }
}

//==============================================================================

bool isGvnPureOp (YdspIrOp op, const std::unordered_set<int>& storedParamSlots, const YdspIrInst& inst) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadParam:
            return storedParamSlots.count (inst.a) == 0;

        case YdspIrOp::loadBlockSize:
        case YdspIrOp::loadSampleRate:
        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadEventFieldI:
        case YdspIrOp::loadInput:
        case YdspIrOp::constF:
        case YdspIrOp::constI:
        case YdspIrOp::constB:
        case YdspIrOp::truncI:
        case YdspIrOp::extF:
        case YdspIrOp::truncF:
        case YdspIrOp::asinhF:
        case YdspIrOp::acoshF:
        case YdspIrOp::atanhF:
        case YdspIrOp::roundF:
        case YdspIrOp::copysignF:
            return true;

        case YdspIrOp::loadOutput:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::movF:
        case YdspIrOp::movI:
        case YdspIrOp::movB:
        case YdspIrOp::phi:
            return false;

        default:
            return hasValueResult (op);
    }
}

bool isCommutativeGvnOp (YdspIrOp op, bool fastMath) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
        case YdspIrOp::mulF:
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::addI:
        case YdspIrOp::mulI:
        case YdspIrOp::minI:
        case YdspIrOp::maxI:
        case YdspIrOp::andI:
        case YdspIrOp::orI:
        case YdspIrOp::xorI:
        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::andB:
        case YdspIrOp::orB:
            return true;

        case YdspIrOp::minF:
        case YdspIrOp::maxF:
            return fastMath;

        default:
            return false;
    }
}

struct GvnKey
{
    YdspIrOp op = YdspIrOp::constF;
    YdspValueType type = YdspValueType::boolType;
    int lanes = 1;
    int a = -1;
    int b = -1;
    int c = -1;
    int memIndex = -1;
    uint64_t fbits = 0;
    int64_t ivalue = 0;
    bool bvalue = false;
    bool boundedFloatToInt = false;
    bool saturating = false;
    bool overflowFree = false;

    bool operator== (const GvnKey& other) const noexcept = default;
};

struct GvnKeyHash
{
    size_t operator() (const GvnKey& k) const noexcept
    {
        uint64_t h = static_cast<uint64_t> (k.op) * 0x9e3779b97f4a7c15ULL;
        const auto mix = [&h] (uint64_t v) noexcept
        {
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };

        mix (static_cast<uint64_t> (k.type));
        mix (static_cast<uint64_t> (k.lanes));
        mix (static_cast<uint64_t> (k.a));
        mix (static_cast<uint64_t> (k.b));
        mix (static_cast<uint64_t> (k.c));
        mix (static_cast<uint64_t> (k.memIndex));
        mix (k.fbits);
        mix (static_cast<uint64_t> (k.ivalue));
        mix (static_cast<uint64_t> ((k.bvalue ? 1 : 0) | (k.boundedFloatToInt ? 2 : 0) | (k.saturating ? 4 : 0) | (k.overflowFree ? 8 : 0)));
        return static_cast<size_t> (h);
    }
};

template <typename LeaderFn>
GvnKey makeGvnKey (const YdspIrInst& inst, YdspValueType type, int lanes, bool fastMath, LeaderFn&& leaderOf)
{
    GvnKey key;
    key.op = inst.op;
    key.type = type;
    key.lanes = lanes;
    key.a = isValueIdOperand (inst.op, 0) ? leaderOf (inst.a) : inst.a;
    key.b = isValueIdOperand (inst.op, 1) ? leaderOf (inst.b) : inst.b;
    key.c = isValueIdOperand (inst.op, 2) ? leaderOf (inst.c) : inst.c;
    key.memIndex = inst.memIndex;
    key.fbits = inst.op == YdspIrOp::constF ? std::bit_cast<uint64_t> (inst.fvalue) : 0ULL;
    key.ivalue = (inst.op == YdspIrOp::constI || inst.op == YdspIrOp::advanceWrapI) ? inst.ivalue : 0;
    key.bvalue = inst.op == YdspIrOp::constB ? inst.bvalue : false;
    key.boundedFloatToInt = inst.op == YdspIrOp::ftoi ? inst.boundedFloatToInt : false;
    key.saturating = inst.saturatingIntegerArithmetic;
    key.overflowFree = inst.overflowFreeIntegerArithmetic;

    switch (key.op)
    {
        case YdspIrOp::gtF:
            key.op = YdspIrOp::ltF;
            std::swap (key.a, key.b);
            break;

        case YdspIrOp::geF:
            key.op = YdspIrOp::leF;
            std::swap (key.a, key.b);
            break;

        case YdspIrOp::gtI:
            key.op = YdspIrOp::ltI;
            std::swap (key.a, key.b);
            break;

        case YdspIrOp::geI:
            key.op = YdspIrOp::leI;
            std::swap (key.a, key.b);
            break;

        default:
            break;
    }

    if (isCommutativeGvnOp (key.op, fastMath) && key.a > key.b)
        std::swap (key.a, key.b);

    return key;
}

struct PhiKey
{
    YdspValueType type = YdspValueType::boolType;
    int lanes = 1;
    std::vector<std::pair<int, int>> incoming;

    bool operator== (const PhiKey& other) const noexcept = default;
};

struct PhiKeyHash
{
    size_t operator() (const PhiKey& k) const noexcept
    {
        uint64_t h = static_cast<uint64_t> (k.type) * 0x9e3779b97f4a7c15ULL;
        const auto mix = [&h] (uint64_t v) noexcept
        {
            h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };

        mix (static_cast<uint64_t> (k.lanes));

        for (const auto& [pred, val] : k.incoming)
        {
            mix (static_cast<uint64_t> (pred));
            mix (static_cast<uint64_t> (val));
        }

        return static_cast<size_t> (h);
    }
};

} // namespace

//==============================================================================

bool YdspOptimizer::constructSsa (YdspIrFunction& fn)
{
    if (fn.blocks.empty())
        return false;

    const detail::YdspDomTree dom (fn);
    const size_t numValues = fn.valueTypes.size();

    if (numValues == 0)
        return false;

    std::vector<int> defCount (numValues, 0);
    std::vector<std::vector<int>> defBlocks (numValues);
    std::vector<std::pair<int, int>> firstDefPos (numValues, { -1, -1 });
    std::vector<char> needsRename (numValues, 0);

    for (const int b : dom.rpo)
    {
        const auto& block = fn.blocks[static_cast<size_t> (b)];

        for (int i = 0; i < static_cast<int> (block.insts.size()); ++i)
        {
            const auto& inst = block.insts[static_cast<size_t> (i)];

            if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues)
                continue;

            const auto id = static_cast<size_t> (inst.result);
            ++defCount[id];

            if (defCount[id] == 1)
                firstDefPos[id] = { b, i };
            else
                needsRename[id] = 1;

            if (defBlocks[id].empty() || defBlocks[id].back() != b)
                defBlocks[id].push_back (b);
        }
    }

    // Also rename any single-definition value whose use is not dominated by its
    // definition (for instance, defined in one branch and read downstream).
    for (const int b : dom.rpo)
    {
        const auto& block = fn.blocks[static_cast<size_t> (b)];

        const auto checkUse = [&] (int value, int instIndex)
        {
            if (value < 0 || static_cast<size_t> (value) >= numValues)
                return;

            const auto id = static_cast<size_t> (value);

            if (defCount[id] != 1)
                return;

            const auto [defBlock, defInst] = firstDefPos[id];

            if (! dom.dominates (defBlock, b) || (defBlock == b && instIndex >= 0 && defInst >= instIndex))
                needsRename[id] = 1;
        };

        for (int i = 0; i < static_cast<int> (block.insts.size()); ++i)
        {
            const auto& inst = block.insts[static_cast<size_t> (i)];

            if (inst.op == YdspIrOp::phi)
            {
                for (const auto& [pred, val] : inst.phiIncoming)
                {
                    if (val < 0 || static_cast<size_t> (val) >= numValues)
                        continue;

                    const auto id = static_cast<size_t> (val);

                    if (defCount[id] == 1 && ! dom.dominates (firstDefPos[id].first, pred))
                        needsRename[id] = 1;
                }

                continue;
            }

            if (isValueIdOperand (inst.op, 0))
                checkUse (inst.a, i);
            if (isValueIdOperand (inst.op, 1))
                checkUse (inst.b, i);
            if (isValueIdOperand (inst.op, 2))
                checkUse (inst.c, i);
        }

        if (block.term == YdspIrTerm::branchIf)
            checkUse (block.termCond, -1);
    }

    std::vector<int> varsToRename;
    std::vector<int> varToDense (numValues, -1);

    for (size_t v = 0; v < numValues; ++v)
    {
        if (needsRename[v])
        {
            varToDense[v] = static_cast<int> (varsToRename.size());
            varsToRename.push_back (static_cast<int> (v));
        }
    }

    if (varsToRename.empty())
        return false;

    // Pruned SSA: compute liveIn for varsToRename only.
    const size_t numVars = varsToRename.size();
    const size_t numWords = (numVars + 63) / 64;
    const size_t numBlocks = fn.blocks.size();

    std::vector<uint64_t> upwardUses (numBlocks * numWords, 0);
    std::vector<uint64_t> killedDefs (numBlocks * numWords, 0);
    std::vector<uint64_t> liveIn (numBlocks * numWords, 0);
    std::vector<uint64_t> liveOut (numBlocks * numWords, 0);

    const auto setBit = [] (std::vector<uint64_t>& bits, size_t block, size_t words, int dense)
    {
        const auto d = static_cast<size_t> (dense);
        bits[block * words + (d >> 6)] |= (uint64_t { 1 } << (d & 63));
    };

    const auto testBit = [] (const std::vector<uint64_t>& bits, size_t block, size_t words, int dense) -> bool
    {
        const auto d = static_cast<size_t> (dense);
        return (bits[block * words + (d >> 6)] & (uint64_t { 1 } << (d & 63))) != 0;
    };

    for (const int b : dom.rpo)
    {
        const auto bu = static_cast<size_t> (b);
        const auto& block = fn.blocks[bu];

        const auto recordUse = [&] (int value)
        {
            if (value < 0 || static_cast<size_t> (value) >= numValues)
                return;

            const int dense = varToDense[static_cast<size_t> (value)];

            if (dense >= 0 && ! testBit (killedDefs, bu, numWords, dense))
                setBit (upwardUses, bu, numWords, dense);
        };

        for (const auto& inst : block.insts)
        {
            if (inst.op != YdspIrOp::phi)
            {
                if (isValueIdOperand (inst.op, 0))
                    recordUse (inst.a);
                if (isValueIdOperand (inst.op, 1))
                    recordUse (inst.b);
                if (isValueIdOperand (inst.op, 2))
                    recordUse (inst.c);
            }

            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
            {
                const int dense = varToDense[static_cast<size_t> (inst.result)];

                if (dense >= 0)
                    setBit (killedDefs, bu, numWords, dense);
            }
        }

        if (block.term == YdspIrTerm::branchIf)
            recordUse (block.termCond);
    }

    for (bool changed = true; changed;)
    {
        changed = false;

        for (auto it = dom.rpo.rbegin(); it != dom.rpo.rend(); ++it)
        {
            const auto bu = static_cast<size_t> (*it);
            const size_t base = bu * numWords;

            for (size_t w = 0; w < numWords; ++w)
                liveOut[base + w] = 0;

            for (const int succ : dom.succs[bu])
            {
                const size_t sBase = static_cast<size_t> (succ) * numWords;

                for (size_t w = 0; w < numWords; ++w)
                    liveOut[base + w] |= liveIn[sBase + w];
            }

            for (size_t w = 0; w < numWords; ++w)
            {
                const uint64_t nextIn = upwardUses[base + w] | (liveOut[base + w] & ~killedDefs[base + w]);

                if (nextIn != liveIn[base + w])
                {
                    liveIn[base + w] = nextIn;
                    changed = true;
                }
            }
        }
    }

    // Cytron-style iterated dominance frontier phi placement.
    const auto df = dom.computeDominanceFrontiers();
    std::vector<std::vector<int>> blockPhiVars (numBlocks);
    std::vector<int> hasPhiStamp (numBlocks, 0);
    std::vector<int> workStamp (numBlocks, 0);
    int stamp = 0;

    for (const int var : varsToRename)
    {
        ++stamp;
        const int dense = varToDense[static_cast<size_t> (var)];
        std::vector<int> worklist;

        for (const int defB : defBlocks[static_cast<size_t> (var)])
        {
            workStamp[static_cast<size_t> (defB)] = stamp;
            worklist.push_back (defB);
        }

        while (! worklist.empty())
        {
            const int b = worklist.back();
            worklist.pop_back();

            for (const int y : df[static_cast<size_t> (b)])
            {
                if (y == 0 || hasPhiStamp[static_cast<size_t> (y)] == stamp)
                    continue;

                hasPhiStamp[static_cast<size_t> (y)] = stamp;

                if (! testBit (liveIn, static_cast<size_t> (y), numWords, dense))
                    continue;

                blockPhiVars[static_cast<size_t> (y)].push_back (var);

                if (workStamp[static_cast<size_t> (y)] != stamp)
                {
                    workStamp[static_cast<size_t> (y)] = stamp;
                    worklist.push_back (y);
                }
            }
        }
    }

    // Insert placeholder phi instructions at the front of each block (recording
    // their original variable id alongside them for renaming).
    std::vector<std::vector<int>> phiOriginalVars (numBlocks);

    for (size_t b = 0; b < numBlocks; ++b)
    {
        if (blockPhiVars[b].empty())
            continue;

        std::vector<YdspIrInst> phis;
        phis.reserve (blockPhiVars[b].size());
        phiOriginalVars[b] = blockPhiVars[b];

        for (const int var : blockPhiVars[b])
        {
            YdspIrInst phi;
            phi.op = YdspIrOp::phi;
            phi.result = cloneSsaValue (fn, var);
            phis.push_back (std::move (phi));
        }

        fn.blocks[b].insts.insert (fn.blocks[b].insts.begin(), phis.begin(), phis.end());
    }

    // Rename variables via iterative DFS over the dominator tree.
    std::vector<std::vector<int>> stacks (numVars);
    std::vector<int> loopInductionUpdate (fn.loops.size(), -1);

    struct RenameFrame
    {
        int block = 0;
        size_t nextChild = 0;
        std::vector<int> pushedDense;
        bool entered = false;
    };

    std::vector<RenameFrame> renameStack;
    renameStack.push_back ({ 0, 0, {}, false });

    while (! renameStack.empty())
    {
        auto& frame = renameStack.back();
        const int b = frame.block;

        if (! frame.entered)
        {
            frame.entered = true;
            auto& block = fn.blocks[static_cast<size_t> (b)];
            const size_t numNewPhis = phiOriginalVars[static_cast<size_t> (b)].size();

            for (size_t p = 0; p < numNewPhis; ++p)
            {
                const int origVar = phiOriginalVars[static_cast<size_t> (b)][p];
                const int dense = varToDense[static_cast<size_t> (origVar)];
                const int phiResult = block.insts[p].result;

                stacks[static_cast<size_t> (dense)].push_back (phiResult);
                frame.pushedDense.push_back (dense);

                for (size_t l = 0; l < fn.loops.size(); ++l)
                    if (! fn.loops[l].unrolled && fn.loops[l].headerBlock == b && fn.loops[l].induction == origVar)
                        loopInductionUpdate[l] = phiResult;
            }

            const auto rewriteUse = [&] (int& val)
            {
                if (val < 0 || static_cast<size_t> (val) >= numValues)
                    return;

                const int dense = varToDense[static_cast<size_t> (val)];

                if (dense >= 0 && ! stacks[static_cast<size_t> (dense)].empty())
                    val = stacks[static_cast<size_t> (dense)].back();
            };

            for (size_t i = numNewPhis; i < block.insts.size(); ++i)
            {
                auto& inst = block.insts[i];

                if (inst.op == YdspIrOp::phi)
                {
                    if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                    {
                        const int dense = varToDense[static_cast<size_t> (inst.result)];

                        if (dense >= 0)
                        {
                            const int fresh = cloneSsaValue (fn, inst.result);
                            inst.result = fresh;
                            stacks[static_cast<size_t> (dense)].push_back (fresh);
                            frame.pushedDense.push_back (dense);
                        }
                    }

                    continue;
                }

                if (isValueIdOperand (inst.op, 0))
                    rewriteUse (inst.a);
                if (isValueIdOperand (inst.op, 1))
                    rewriteUse (inst.b);
                if (isValueIdOperand (inst.op, 2))
                    rewriteUse (inst.c);

                if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                {
                    const int dense = varToDense[static_cast<size_t> (inst.result)];

                    if (dense >= 0)
                    {
                        const int fresh = cloneSsaValue (fn, inst.result);
                        inst.result = fresh;
                        stacks[static_cast<size_t> (dense)].push_back (fresh);
                        frame.pushedDense.push_back (dense);
                    }
                }
            }

            if (block.term == YdspIrTerm::branchIf)
                rewriteUse (block.termCond);

            for (const int succ : dom.succs[static_cast<size_t> (b)])
            {
                auto& succBlock = fn.blocks[static_cast<size_t> (succ)];
                const size_t succNewPhis = phiOriginalVars[static_cast<size_t> (succ)].size();

                for (size_t p = 0; p < succNewPhis; ++p)
                {
                    const int origVar = phiOriginalVars[static_cast<size_t> (succ)][p];
                    const int dense = varToDense[static_cast<size_t> (origVar)];
                    const int incoming = ! stacks[static_cast<size_t> (dense)].empty()
                                             ? stacks[static_cast<size_t> (dense)].back()
                                             : origVar;

                    succBlock.insts[p].phiIncoming.emplace_back (b, incoming);
                }
            }
        }

        const auto& children = dom.domChildren[static_cast<size_t> (b)];

        if (frame.nextChild < children.size())
        {
            const int child = children[frame.nextChild++];
            renameStack.push_back ({ child, 0, {}, false });
        }
        else
        {
            for (auto it = frame.pushedDense.rbegin(); it != frame.pushedDense.rend(); ++it)
                stacks[static_cast<size_t> (*it)].pop_back();

            renameStack.pop_back();
        }
    }

    for (size_t l = 0; l < fn.loops.size(); ++l)
        if (loopInductionUpdate[l] >= 0)
            fn.loops[l].induction = loopInductionUpdate[l];

    return true;
}

//==============================================================================

bool YdspOptimizer::globalValueNumbering (YdspIrFunction& fn)
{
    if (fn.blocks.empty())
        return false;

    bool anyChanged = false;

    for (int round = 0; round < 4; ++round)
    {
        const detail::YdspDomTree dom (fn);
        const auto types = resolveValueTypes (fn);
        const size_t numValues = fn.valueTypes.size();

        std::vector<int> defCount (numValues, 0);
        std::unordered_set<int> storedParamSlots;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                    ++defCount[static_cast<size_t> (inst.result)];

                if (inst.op == YdspIrOp::storeParam)
                    storedParamSlots.insert (inst.memIndex);
            }
        }

        std::vector<int> leader (numValues);

        for (size_t i = 0; i < numValues; ++i)
            leader[i] = static_cast<int> (i);

        std::vector<char> replaced (numValues, 0);

        const auto leaderOf = [&leader] (int v) noexcept -> int
        {
            if (v < 0 || static_cast<size_t> (v) >= leader.size())
                return v;

            int cur = v;

            while (leader[static_cast<size_t> (cur)] != cur)
                cur = leader[static_cast<size_t> (cur)];

            return cur;
        };

        std::unordered_map<GvnKey, int, GvnKeyHash> exprTable;
        std::vector<GvnKey> undoStack;

        struct DfsFrame
        {
            int block = 0;
            size_t nextChild = 0;
            size_t undoMark = 0;
            bool entered = false;
        };

        std::vector<DfsFrame> stack;
        stack.push_back ({ 0, 0, 0, false });

        while (! stack.empty())
        {
            auto& frame = stack.back();
            const int b = frame.block;

            if (! frame.entered)
            {
                frame.entered = true;
                frame.undoMark = undoStack.size();

                const auto& block = fn.blocks[static_cast<size_t> (b)];
                std::unordered_map<PhiKey, int, PhiKeyHash> localPhiTable;

                for (const auto& inst : block.insts)
                {
                    if (inst.op == YdspIrOp::phi)
                    {
                        if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues
                            || defCount[static_cast<size_t> (inst.result)] != 1)
                            continue;

                        int uniqueVal = -1;
                        bool trivial = ! inst.phiIncoming.empty();
                        PhiKey phiKey;
                        phiKey.type = types[static_cast<size_t> (inst.result)];
                        phiKey.lanes = fn.laneCountOf (inst.result);
                        phiKey.incoming.reserve (inst.phiIncoming.size());

                        for (const auto& [pred, rawVal] : inst.phiIncoming)
                        {
                            const int val = leaderOf (rawVal);
                            phiKey.incoming.emplace_back (pred, val);

                            if (val == inst.result)
                                continue;

                            if (val < 0)
                            {
                                trivial = false;
                            }
                            else if (uniqueVal < 0)
                            {
                                uniqueVal = val;
                            }
                            else if (uniqueVal != val)
                            {
                                trivial = false;
                            }
                        }

                        if (trivial && uniqueVal >= 0
                            && types[static_cast<size_t> (uniqueVal)] == phiKey.type
                            && fn.laneCountOf (uniqueVal) == phiKey.lanes)
                        {
                            leader[static_cast<size_t> (inst.result)] = uniqueVal;
                            replaced[static_cast<size_t> (inst.result)] = 1;
                            continue;
                        }

                        std::sort (phiKey.incoming.begin(), phiKey.incoming.end(), [] (const auto& lhs, const auto& rhs)
                        {
                            return lhs.first < rhs.first;
                        });

                        const auto it = localPhiTable.find (phiKey);

                        if (it != localPhiTable.end())
                        {
                            leader[static_cast<size_t> (inst.result)] = it->second;
                            replaced[static_cast<size_t> (inst.result)] = 1;
                        }
                        else
                        {
                            localPhiTable.emplace (std::move (phiKey), inst.result);
                        }

                        continue;
                    }

                    if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues
                        || defCount[static_cast<size_t> (inst.result)] != 1)
                        continue;

                    if (isMoveOpcode (inst.op) && inst.a >= 0
                        && static_cast<size_t> (inst.a) < numValues
                        && defCount[static_cast<size_t> (inst.a)] == 1)
                    {
                        const int src = leaderOf (inst.a);

                        if (src >= 0 && static_cast<size_t> (src) < numValues
                            && types[static_cast<size_t> (src)] == types[static_cast<size_t> (inst.result)]
                            && fn.laneCountOf (src) == fn.laneCountOf (inst.result))
                        {
                            leader[static_cast<size_t> (inst.result)] = src;
                            replaced[static_cast<size_t> (inst.result)] = 1;
                            continue;
                        }
                    }

                    if (! isGvnPureOp (inst.op, storedParamSlots, inst))
                        continue;

                    const GvnKey key = makeGvnKey (inst,
                                                   types[static_cast<size_t> (inst.result)],
                                                   fn.laneCountOf (inst.result),
                                                   fn.fastMath,
                                                   leaderOf);

                    const auto it = exprTable.find (key);

                    if (it != exprTable.end())
                    {
                        const int existing = it->second;

                        if (types[static_cast<size_t> (existing)] == key.type
                            && fn.laneCountOf (existing) == key.lanes)
                        {
                            leader[static_cast<size_t> (inst.result)] = existing;
                            replaced[static_cast<size_t> (inst.result)] = 1;
                            continue;
                        }
                    }

                    exprTable.emplace (key, inst.result);
                    undoStack.push_back (key);
                }
            }

            const auto& children = dom.domChildren[static_cast<size_t> (b)];

            if (frame.nextChild < children.size())
            {
                const int child = children[frame.nextChild++];
                stack.push_back ({ child, 0, 0, false });
            }
            else
            {
                while (undoStack.size() > frame.undoMark)
                {
                    exprTable.erase (undoStack.back());
                    undoStack.pop_back();
                }

                stack.pop_back();
            }
        }

        const bool roundEliminated = std::any_of (replaced.begin(), replaced.end(), [] (char flag)
        {
            return flag != 0;
        });

        if (! roundEliminated)
            break;

        anyChanged = true;

        for (auto& block : fn.blocks)
        {
            for (auto& inst : block.insts)
            {
                if (inst.op == YdspIrOp::phi)
                {
                    for (auto& incoming : inst.phiIncoming)
                        if (incoming.second >= 0)
                            incoming.second = leaderOf (incoming.second);

                    continue;
                }

                if (isValueIdOperand (inst.op, 0) && inst.a >= 0)
                    inst.a = leaderOf (inst.a);
                if (isValueIdOperand (inst.op, 1) && inst.b >= 0)
                    inst.b = leaderOf (inst.b);
                if (isValueIdOperand (inst.op, 2) && inst.c >= 0)
                    inst.c = leaderOf (inst.c);
            }

            if (block.term == YdspIrTerm::branchIf && block.termCond >= 0)
                block.termCond = leaderOf (block.termCond);

            block.insts.erase (std::remove_if (block.insts.begin(), block.insts.end(), [&] (const YdspIrInst& inst)
            {
                return inst.result >= 0
                    && static_cast<size_t> (inst.result) < replaced.size()
                    && replaced[static_cast<size_t> (inst.result)] != 0;
            }), block.insts.end());
        }

        for (auto& loop : fn.loops)
            if (! loop.unrolled && loop.induction >= 0)
                loop.induction = leaderOf (loop.induction);
    }

    return anyChanged;
}

//==============================================================================

bool YdspOptimizer::destructSsa (YdspIrFunction& fn)
{
    const auto hasAnyPhi = [&fn]
    {
        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.op == YdspIrOp::phi)
                    return true;

        return false;
    };

    if (! hasAnyPhi())
        return false;

    simplifyTrivialPhis (fn);

    if (! hasAnyPhi())
        return true;

    const detail::YdspDomTree dom (fn);
    const auto types = resolveValueTypes (fn);
    const size_t numValues = fn.valueTypes.size();
    const size_t numBlocks = fn.blocks.size();

    // Compute SSA liveness (liveInFromPred / liveOut) for interference-safe coalescing.
    std::vector<std::unordered_set<int>> phiDefs (numBlocks);
    std::vector<std::unordered_set<int>> uses (numBlocks);
    std::vector<std::unordered_set<int>> defs (numBlocks);
    std::vector<std::unordered_set<int>> liveInFromPred (numBlocks);
    std::vector<std::unordered_set<int>> liveOut (numBlocks);

    for (const int b : dom.rpo)
    {
        const auto bu = static_cast<size_t> (b);
        const auto& block = fn.blocks[bu];

        const auto recordUse = [&] (int v)
        {
            if (v >= 0 && defs[bu].count (v) == 0 && phiDefs[bu].count (v) == 0)
                uses[bu].insert (v);
        };

        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::phi)
            {
                if (inst.result >= 0)
                    phiDefs[bu].insert (inst.result);

                continue;
            }

            if (isValueIdOperand (inst.op, 0))
                recordUse (inst.a);
            if (isValueIdOperand (inst.op, 1))
                recordUse (inst.b);
            if (isValueIdOperand (inst.op, 2))
                recordUse (inst.c);

            if (inst.result >= 0)
                defs[bu].insert (inst.result);
        }

        if (block.term == YdspIrTerm::branchIf)
            recordUse (block.termCond);
    }

    // Collect phi incoming uses per (predBlock, succBlock).
    std::vector<std::vector<std::pair<int, int>>> predPhiIncoming (numBlocks);

    for (const int s : dom.rpo)
    {
        for (const auto& inst : fn.blocks[static_cast<size_t> (s)].insts)
        {
            if (inst.op != YdspIrOp::phi)
                continue;

            for (const auto& [pred, val] : inst.phiIncoming)
                if (pred >= 0 && static_cast<size_t> (pred) < numBlocks && val >= 0)
                    predPhiIncoming[static_cast<size_t> (pred)].emplace_back (s, val);
        }
    }

    for (bool changed = true; changed;)
    {
        changed = false;

        for (auto it = dom.rpo.rbegin(); it != dom.rpo.rend(); ++it)
        {
            const auto bu = static_cast<size_t> (*it);
            std::unordered_set<int> newOut;

            for (const int succ : dom.succs[bu])
                for (const int v : liveInFromPred[static_cast<size_t> (succ)])
                    newOut.insert (v);

            for (const auto& [succ, val] : predPhiIncoming[bu])
            {
                (void) succ;
                newOut.insert (val);
            }

            std::unordered_set<int> newIn = uses[bu];

            for (const int v : newOut)
                if (defs[bu].count (v) == 0 && phiDefs[bu].count (v) == 0)
                    newIn.insert (v);

            if (newOut != liveOut[bu] || newIn != liveInFromPred[bu])
            {
                liveOut[bu] = std::move (newOut);
                liveInFromPred[bu] = std::move (newIn);
                changed = true;
            }
        }
    }

    // Build definition and total-use info for non-interfering preheader coalescing.
    std::vector<int> defCount (numValues, 0);
    std::vector<int> defBlock (numValues, -1);
    std::vector<YdspIrInst*> defInst (numValues, nullptr);

    for (size_t b = 0; b < numBlocks; ++b)
    {
        for (auto& inst : fn.blocks[b].insts)
        {
            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
            {
                const auto id = static_cast<size_t> (inst.result);
                ++defCount[id];
                defBlock[id] = static_cast<int> (b);
                defInst[id] = &inst;
            }
        }
    }

    const YdspValueUseCounts useCounts (fn);

    std::unordered_set<int> loopHeaders;
    std::unordered_set<int> constBoundLoopInductions;

    for (const auto& loop : fn.loops)
    {
        if (loop.unrolled)
            continue;

        if (loop.headerBlock >= 0)
            loopHeaders.insert (loop.headerBlock);

        if (loop.bound.kind == YdspLoopBoundKind::constant && loop.induction >= 0)
            constBoundLoopInductions.insert (loop.induction);
    }

    std::vector<char> coalesced (numValues, 0);

    for (size_t b = 0; b < numBlocks; ++b)
    {
        if (loopHeaders.count (static_cast<int> (b)) == 0)
            continue;

        for (auto& inst : fn.blocks[b].insts)
        {
            if (inst.op != YdspIrOp::phi || inst.result < 0)
                continue;

            const int p = inst.result;

            if (static_cast<size_t> (p) >= numValues || coalesced[static_cast<size_t> (p)])
                continue;

            for (auto& [pred, v] : inst.phiIncoming)
            {
                (void) pred;

                if (v < 0 || v == p || static_cast<size_t> (v) >= numValues)
                    continue;

                if (coalesced[static_cast<size_t> (v)] || defCount[static_cast<size_t> (v)] != 1)
                    continue;

                if (useCounts.useCount (v) != 1 || defInst[static_cast<size_t> (v)] == nullptr
                    || defInst[static_cast<size_t> (v)]->op == YdspIrOp::phi)
                    continue;

                if (constBoundLoopInductions.count (p) != 0 && defInst[static_cast<size_t> (v)]->op == YdspIrOp::constI)
                    continue;

                if (types[static_cast<size_t> (p)] != types[static_cast<size_t> (v)]
                    || fn.laneCountOf (p) != fn.laneCountOf (v))
                    continue;

                const int dv = defBlock[static_cast<size_t> (v)];

                if (dv != pred || (isConstantOp (defInst[static_cast<size_t> (v)]->op) && constBoundLoopInductions.count (p) != 0))
                    continue;

                if (liveInFromPred[static_cast<size_t> (dv)].count (p) != 0
                    || liveOut[static_cast<size_t> (dv)].count (p) != 0)
                    continue;

                defInst[static_cast<size_t> (v)]->result = p;
                coalesced[static_cast<size_t> (v)] = 1;
                coalesced[static_cast<size_t> (p)] = 1;
                v = p;
                break;
            }
        }
    }

    // Collect edge copies per (pred, target) and erase phi nodes.
    struct EdgeKey
    {
        int pred = -1;
        int target = -1;

        bool operator== (const EdgeKey& other) const noexcept = default;
    };

    struct EdgeKeyHash
    {
        size_t operator() (const EdgeKey& k) const noexcept
        {
            return static_cast<size_t> ((static_cast<uint64_t> (k.pred) << 32) ^ static_cast<uint32_t> (k.target));
        }
    };

    std::unordered_map<EdgeKey, std::vector<SsaEdgeCopy>, EdgeKeyHash> edgeCopies;

    for (int b = 0; b < static_cast<int> (numBlocks); ++b)
    {
        auto& blockInsts = fn.blocks[static_cast<size_t> (b)].insts;

        for (const auto& inst : blockInsts)
        {
            if (inst.op != YdspIrOp::phi || inst.result < 0)
                continue;

            const auto type = types[static_cast<size_t> (inst.result)];
            const int lanes = fn.laneCountOf (inst.result);

            for (const auto& [pred, val] : inst.phiIncoming)
            {
                if (pred < 0 || static_cast<size_t> (pred) >= numBlocks || val < 0 || val == inst.result)
                    continue;

                edgeCopies[{ pred, b }].push_back ({ inst.result, val, type, lanes });
            }
        }

        blockInsts.erase (std::remove_if (blockInsts.begin(), blockInsts.end(), [] (const YdspIrInst& inst)
        {
            return inst.op == YdspIrOp::phi;
        }), blockInsts.end());
    }

    struct PendingEdge
    {
        int pred = -1;
        int target = -1;
        std::vector<SsaEdgeCopy> copies;
    };

    std::vector<PendingEdge> criticalEdges;

    std::unordered_map<int, int> loopInductionByHeader;
    for (const auto& loop : fn.loops)
        if (! loop.unrolled && loop.headerBlock >= 0 && loop.induction >= 0)
            loopInductionByHeader[loop.headerBlock] = loop.induction;

    for (auto& [edge, copies] : edgeCopies)
    {
        if (copies.empty())
            continue;

        if (const auto indIt = loopInductionByHeader.find (edge.target); indIt != loopInductionByHeader.end())
        {
            const int induction = indIt->second;
            std::stable_partition (copies.begin(), copies.end(), [induction] (const SsaEdgeCopy& c)
            {
                return c.dst != induction;
            });
        }

        auto& predBlock = fn.blocks[static_cast<size_t> (edge.pred)];
        const bool singleOutgoing = predBlock.term != YdspIrTerm::branchIf
                                 || predBlock.termTarget == predBlock.termTarget2;

        if (singleOutgoing)
            emitParallelCopies (fn, std::move (copies), predBlock.insts);
        else
            criticalEdges.push_back ({ edge.pred, edge.target, std::move (copies) });
    }

    // Split critical edges, preserving structured if/else/join layout for WASM
    // and keeping fn.loops indices consistent.
    for (size_t idx = 0; idx < criticalEdges.size(); ++idx)
    {
        auto edge = std::move (criticalEdges[idx]);
        const int pred = edge.pred;
        const int target = edge.target;

        const bool isStructuredElselessIf =
            pred >= 0 && target > pred + 1 && static_cast<size_t> (target) < fn.blocks.size()
            && loopHeaders.count (pred) == 0
            && fn.blocks[static_cast<size_t> (pred)].term == YdspIrTerm::branchIf
            && fn.blocks[static_cast<size_t> (pred)].termTarget == pred + 1
            && fn.blocks[static_cast<size_t> (pred)].termTarget2 == target
            && fn.blocks[static_cast<size_t> (target - 1)].term == YdspIrTerm::branch
            && fn.blocks[static_cast<size_t> (target - 1)].termTarget == target;

        if (isStructuredElselessIf)
        {
            const int insertPos = target;

            for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
            {
                if (b == pred)
                    continue;

                auto& block = fn.blocks[static_cast<size_t> (b)];

                if (block.termTarget >= insertPos)
                    ++block.termTarget;
                if (block.termTarget2 >= insertPos)
                    ++block.termTarget2;
            }

            for (auto& loop : fn.loops)
            {
                if (loop.headerBlock >= insertPos)
                    ++loop.headerBlock;
                if (loop.exitBlock >= insertPos)
                    ++loop.exitBlock;
            }

            std::unordered_set<int> shiftedHeaders;

            for (const int h : loopHeaders)
                shiftedHeaders.insert (h >= insertPos ? h + 1 : h);

            loopHeaders = std::move (shiftedHeaders);

            for (size_t rem = idx + 1; rem < criticalEdges.size(); ++rem)
            {
                if (criticalEdges[rem].pred >= insertPos)
                    ++criticalEdges[rem].pred;
                if (criticalEdges[rem].target >= insertPos)
                    ++criticalEdges[rem].target;
            }

            YdspIrBlock elseBlock;
            elseBlock.term = YdspIrTerm::fallthrough;
            emitParallelCopies (fn, std::move (edge.copies), elseBlock.insts);

            fn.blocks.insert (fn.blocks.begin() + static_cast<std::ptrdiff_t> (insertPos), std::move (elseBlock));
        }
        else
        {
            const int newBlockIndex = static_cast<int> (fn.blocks.size());
            YdspIrBlock edgeBlock;
            edgeBlock.term = YdspIrTerm::branch;
            edgeBlock.termTarget = target;
            emitParallelCopies (fn, std::move (edge.copies), edgeBlock.insts);

            fn.blocks.push_back (std::move (edgeBlock));

            auto& predBlock = fn.blocks[static_cast<size_t> (pred)];

            if (predBlock.termTarget == target)
                predBlock.termTarget = newBlockIndex;
            if (predBlock.termTarget2 == target)
                predBlock.termTarget2 = newBlockIndex;
        }
    }

    return true;
}

} // namespace yup
