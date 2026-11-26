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

bool isSlpVectorMathOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::asinF:
        case YdspIrOp::acosF:
        case YdspIrOp::atanF:
        case YdspIrOp::sinhF:
        case YdspIrOp::coshF:
        case YdspIrOp::tanhF:
        case YdspIrOp::asinhF:
        case YdspIrOp::acoshF:
        case YdspIrOp::atanhF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::powF:
        case YdspIrOp::atan2F:
        case YdspIrOp::fmodF:
            return true;

        default:
            return false;
    }
}

bool isSlpFloatOp (YdspIrOp op, bool allowVectorMath, bool allowPackedFma, bool allowFastMath) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::negF:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::movF:
            return true;

        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
            return allowPackedFma;

        default:
            return (allowFastMath && isInlineFastMathTranscendental (op))
                || (allowVectorMath && isSlpVectorMathOp (op));
    }
}

bool isSlpBoolOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::ltF:
        case YdspIrOp::leF:
        case YdspIrOp::gtF:
        case YdspIrOp::geF:
        case YdspIrOp::andB:
        case YdspIrOp::orB:
        case YdspIrOp::notB:
        case YdspIrOp::movB:
            return true;

        default:
            return false;
    }
}

bool isSlpCommutativeOp (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
        case YdspIrOp::mulF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::andB:
        case YdspIrOp::orB:
            return true;

        default:
            return false;
    }
}

int slpValueOperandCount (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::selectB:
            return 3;

        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::powF:
        case YdspIrOp::atan2F:
        case YdspIrOp::fmodF:
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::ltF:
        case YdspIrOp::leF:
        case YdspIrOp::gtF:
        case YdspIrOp::geF:
        case YdspIrOp::andB:
        case YdspIrOp::orB:
            return 2;

        case YdspIrOp::negF:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::movF:
        case YdspIrOp::movB:
        case YdspIrOp::notB:
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::asinF:
        case YdspIrOp::acosF:
        case YdspIrOp::atanF:
        case YdspIrOp::sinhF:
        case YdspIrOp::coshF:
        case YdspIrOp::tanhF:
        case YdspIrOp::asinhF:
        case YdspIrOp::acoshF:
        case YdspIrOp::atanhF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
            return 1;

        default:
            return 0;
    }
}

bool isMemoryBarrierOp (YdspIrOp op) noexcept
{
    return op == YdspIrOp::emitEvent
        || op == YdspIrOp::traceInt
        || op == YdspIrOp::traceFloat
        || op == YdspIrOp::traceCommit;
}

bool isStateScalarMemoryOp (YdspIrOp op) noexcept
{
    return op == YdspIrOp::loadStateF || op == YdspIrOp::storeStateF
        || op == YdspIrOp::loadStateI || op == YdspIrOp::storeStateI
        || op == YdspIrOp::loadParam || op == YdspIrOp::storeParam
        || op == YdspIrOp::loadParamOut || op == YdspIrOp::storeParamOut
        || op == YdspIrOp::loadEventFieldF || op == YdspIrOp::storeEventFieldF
        || op == YdspIrOp::loadEventFieldI || op == YdspIrOp::storeEventFieldI;
}

int getOperand (const YdspIrInst& inst, int slot) noexcept
{
    return slot == 0 ? inst.a : (slot == 1 ? inst.b : inst.c);
}

void setOperand (YdspIrInst& inst, int slot, int value) noexcept
{
    if (slot == 0)
        inst.a = value;
    else if (slot == 1)
        inst.b = value;
    else
        inst.c = value;
}

//==============================================================================

struct SlpMemAddr
{
    enum class Kind
    {
        none,
        barrier,
        stateArray,
        stream,
        scalarSlot
    };

    Kind kind = Kind::none;
    bool isStore = false;
    bool isInput = false;
    YdspValueType elemType = YdspValueType::float32Type;
    int memIndex = -1;
    int baseReg = -1;
    int baseDef = -1;
    int64_t offset = 0;
    int lanes = 1;
};

bool findStateArrayIndex (const YdspIrFunction& fn, YdspValueType type, int memIndex, int& outIndex) noexcept
{
    for (size_t i = 0; i < fn.stateArrays.size(); ++i)
    {
        const auto& arr = fn.stateArrays[i];

        if (arr.type == type && memIndex >= arr.base && memIndex < arr.base + arr.length)
        {
            outIndex = static_cast<int> (i);
            return true;
        }
    }

    return false;
}

bool mayAliasMemory (const YdspIrFunction& fn, const SlpMemAddr& a, const SlpMemAddr& b) noexcept
{
    if (a.kind == SlpMemAddr::Kind::none || b.kind == SlpMemAddr::Kind::none)
        return false;

    if (a.kind == SlpMemAddr::Kind::barrier || b.kind == SlpMemAddr::Kind::barrier)
        return true;

    if (! a.isStore && ! b.isStore)
        return false;

    if (a.kind != b.kind)
        return false;

    if (a.kind == SlpMemAddr::Kind::scalarSlot)
        return a.elemType == b.elemType && a.memIndex == b.memIndex;

    if (a.kind == SlpMemAddr::Kind::stream)
    {
        if (a.isInput || b.isInput)
            return false;

        if (a.memIndex != b.memIndex)
            return false;

        if (a.baseReg == b.baseReg && (a.baseReg < 0 || a.baseDef == b.baseDef))
            return a.offset < b.offset + b.lanes && b.offset < a.offset + a.lanes;

        return true;
    }

    // stateArray
    if (a.elemType != b.elemType)
        return false;

    if (a.memIndex == b.memIndex)
    {
        if (a.baseReg == b.baseReg && (a.baseReg < 0 || a.baseDef == b.baseDef))
            return a.offset < b.offset + b.lanes && b.offset < a.offset + a.lanes;

        return true;
    }

    if (a.baseReg < 0 && b.baseReg < 0)
    {
        const int64_t aStart = static_cast<int64_t> (a.memIndex) + a.offset;
        const int64_t bStart = static_cast<int64_t> (b.memIndex) + b.offset;
        return aStart < bStart + b.lanes && bStart < aStart + a.lanes;
    }

    if (! fn.stateArrays.empty())
    {
        int idxA = -1;
        int idxB = -1;

        if (findStateArrayIndex (fn, a.elemType, a.memIndex, idxA)
            && findStateArrayIndex (fn, b.elemType, b.memIndex, idxB))
            return idxA == idxB;
    }

    return false;
}

//==============================================================================

struct SlpFunctionInfo
{
    std::vector<int> globalDefCount;
    std::vector<int> globalDefBlock;
    std::vector<std::optional<int64_t>> globalConstInt;
    std::vector<std::optional<double>> globalConstFloat;
    detail::YdspDomTree dom;

    explicit SlpFunctionInfo (const YdspIrFunction& fn)
        : dom (fn)
    {
        rebuild (fn);
    }

    void rebuild (const YdspIrFunction& fn)
    {
        const size_t numValues = fn.valueTypes.size();
        globalDefCount.assign (numValues, 0);
        globalDefBlock.assign (numValues, -1);
        globalConstInt.assign (numValues, std::nullopt);
        globalConstFloat.assign (numValues, std::nullopt);

        for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
        {
            for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
            {
                if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues)
                    continue;

                const auto id = static_cast<size_t> (inst.result);
                ++globalDefCount[id];
                globalDefBlock[id] = b;
            }
        }

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result < 0 || static_cast<size_t> (inst.result) >= numValues)
                    continue;

                const auto id = static_cast<size_t> (inst.result);

                if (globalDefCount[id] != 1)
                    continue;

                if (inst.op == YdspIrOp::constI && fn.laneCountOf (inst.result) == 1)
                    globalConstInt[id] = inst.ivalue;
                else if (inst.op == YdspIrOp::constF && fn.laneCountOf (inst.result) == 1)
                    globalConstFloat[id] = inst.fvalue;
            }
        }
    }
};

//==============================================================================

struct ExistingGather
{
    int instIndex = -1;
    int vectorValue = -1;
    std::vector<int> scalars;
    std::vector<int> reachingDefs;
};

struct SlpBlockContext
{
    YdspIrFunction& fn;
    const SlpFunctionInfo& fnInfo;
    int blockIndex = -1;
    int lanes = 4;

    int numInsts = 0;
    int numValues = 0;
    int numBitWords = 0;

    std::vector<std::array<int, 3>> reachingDef;
    std::vector<std::array<int, 3>> nextDef;
    std::vector<int> resultNextDef;
    std::vector<std::vector<std::pair<int, int>>> resultUsesInBlock;

    std::vector<int> usesOutsideBlock;
    std::vector<bool> usedInTermCond;
    std::vector<SlpMemAddr> memAddrs;

    std::vector<std::vector<int>> directSuccs;
    std::vector<std::vector<uint64_t>> reach;

    std::vector<ExistingGather> existingGathers;
    std::vector<std::pair<int, int>> existingSplats; // (scalarValue, vectorValue) when globalDefCount == 1
    std::unordered_map<int, int> preheaderSplats;

    SlpBlockContext (YdspIrFunction& function, const SlpFunctionInfo& info, int bIdx, int width)
        : fn (function)
        , fnInfo (info)
        , blockIndex (bIdx)
        , lanes (width)
    {
        build();
    }

    void build()
    {
        const auto& block = fn.blocks[static_cast<size_t> (blockIndex)];
        numInsts = static_cast<int> (block.insts.size());
        numValues = static_cast<int> (fn.valueTypes.size());
        numBitWords = (numInsts + 63) / 64;

        reachingDef.assign (static_cast<size_t> (numInsts), { -1, -1, -1 });
        nextDef.assign (static_cast<size_t> (numInsts), { -1, -1, -1 });
        resultNextDef.assign (static_cast<size_t> (numInsts), -1);
        resultUsesInBlock.assign (static_cast<size_t> (numInsts), {});
        usesOutsideBlock.assign (static_cast<size_t> (numValues), 0);
        usedInTermCond.assign (static_cast<size_t> (numValues), false);
        memAddrs.assign (static_cast<size_t> (numInsts), {});
        directSuccs.assign (static_cast<size_t> (numInsts), {});

        forEachValueUse (fn, [this] (int b, int instIdx, int, int value)
        {
            if (value < 0 || value >= numValues)
                return;

            if (b != blockIndex)
                ++usesOutsideBlock[static_cast<size_t> (value)];
            else if (instIdx < 0)
                usedInTermCond[static_cast<size_t> (value)] = true;
        });

        std::vector<int> currentDef (static_cast<size_t> (numValues), -1);
        std::vector<std::vector<std::pair<int, int>>> readersSinceDef (static_cast<size_t> (numValues));
        std::vector<int> memOps;

        const auto addEdge = [this] (int u, int v)
        {
            if (u < 0 || v < 0 || u >= v)
                return;

            auto& succs = directSuccs[static_cast<size_t> (u)];

            if (std::find (succs.begin(), succs.end(), v) == succs.end())
                succs.push_back (v);
        };

        for (int i = 0; i < numInsts; ++i)
        {
            const auto& inst = block.insts[static_cast<size_t> (i)];

            for (int opSlot = 0; opSlot < 3; ++opSlot)
            {
                if (! isValueIdOperand (inst.op, opSlot))
                    continue;

                const int val = getOperand (inst, opSlot);

                if (val < 0 || val >= numValues)
                    continue;

                const int def = currentDef[static_cast<size_t> (val)];
                reachingDef[static_cast<size_t> (i)][static_cast<size_t> (opSlot)] = def;

                if (def >= 0)
                {
                    resultUsesInBlock[static_cast<size_t> (def)].emplace_back (i, opSlot);
                    addEdge (def, i);
                }

                readersSinceDef[static_cast<size_t> (val)].emplace_back (i, opSlot);
            }

            memAddrs[static_cast<size_t> (i)] = resolveMemAddr (i);

            if (memAddrs[static_cast<size_t> (i)].kind != SlpMemAddr::Kind::none)
            {
                for (const int prev : memOps)
                    if (mayAliasMemory (fn, memAddrs[static_cast<size_t> (prev)], memAddrs[static_cast<size_t> (i)]))
                        addEdge (prev, i);

                memOps.push_back (i);
            }

            if (inst.result >= 0 && inst.result < numValues)
            {
                const int r = inst.result;
                const int prevDef = currentDef[static_cast<size_t> (r)];

                if (prevDef >= 0)
                {
                    resultNextDef[static_cast<size_t> (prevDef)] = i;
                    addEdge (prevDef, i);
                }

                for (const auto& [readerInst, readerSlot] : readersSinceDef[static_cast<size_t> (r)])
                {
                    if (readerInst < i)
                    {
                        nextDef[static_cast<size_t> (readerInst)][static_cast<size_t> (readerSlot)] = i;
                        addEdge (readerInst, i);
                    }
                }

                readersSinceDef[static_cast<size_t> (r)].clear();
                currentDef[static_cast<size_t> (r)] = i;
            }
        }

        reach.assign (static_cast<size_t> (numInsts), std::vector<uint64_t> (static_cast<size_t> (numBitWords), 0));

        for (int u = numInsts - 1; u >= 0; --u)
        {
            auto& uReach = reach[static_cast<size_t> (u)];

            for (const int v : directSuccs[static_cast<size_t> (u)])
            {
                uReach[static_cast<size_t> (v >> 6)] |= (uint64_t (1) << (v & 63));
                const auto& vReach = reach[static_cast<size_t> (v)];

                for (int w = 0; w < numBitWords; ++w)
                    uReach[static_cast<size_t> (w)] |= vReach[static_cast<size_t> (w)];
            }
        }

        collectExistingVectorForms();
    }

    bool canReach (int u, int v) const noexcept
    {
        if (u == v)
            return true;

        if (u < 0 || v < 0 || u >= v)
            return false;

        return ((reach[static_cast<size_t> (u)][static_cast<size_t> (v >> 6)] >> (v & 63)) & 1) != 0;
    }

    bool areMutuallyIndependent (const std::vector<int>& group) const noexcept
    {
        for (size_t a = 0; a < group.size(); ++a)
        {
            for (size_t b = a + 1; b < group.size(); ++b)
            {
                if (group[a] == group[b])
                    return false;

                if (canReach (group[a], group[b]) || canReach (group[b], group[a]))
                    return false;
            }
        }

        return true;
    }

    bool isLiveOutAfter (int instIdx) const noexcept
    {
        const int r = fn.blocks[static_cast<size_t> (blockIndex)].insts[static_cast<size_t> (instIdx)].result;

        if (r < 0 || r >= numValues)
            return false;

        if (resultNextDef[static_cast<size_t> (instIdx)] >= 0)
            return false;

        return usedInTermCond[static_cast<size_t> (r)]
            || usesOutsideBlock[static_cast<size_t> (r)] > 0
            || fnInfo.globalDefCount[static_cast<size_t> (r)] > 1;
    }

    std::pair<int, int> unwrapCopy (int value, int rDef, int atInst) const noexcept
    {
        const auto& insts = fn.blocks[static_cast<size_t> (blockIndex)].insts;

        while (rDef >= 0)
        {
            const auto& defInst = insts[static_cast<size_t> (rDef)];

            if (defInst.op != YdspIrOp::movF || defInst.a < 0)
                break;

            const int src = defInst.a;
            const int srcDef = reachingDef[static_cast<size_t> (rDef)][0];
            const int srcNext = nextDef[static_cast<size_t> (rDef)][0];

            // Only look through the copy if src holds the same value at atInst.
            if (srcNext >= 0 && srcNext < atInst)
                break;

            value = src;
            rDef = srcDef;
            atInst = rDef >= 0 ? rDef : atInst;
        }

        return { value, rDef };
    }

    std::optional<int64_t> resolveConstIntAt (int instIdx, int opSlot) const noexcept
    {
        const auto& insts = fn.blocks[static_cast<size_t> (blockIndex)].insts;
        const int val = getOperand (insts[static_cast<size_t> (instIdx)], opSlot);

        if (val < 0 || val >= numValues)
            return std::nullopt;

        const int def = reachingDef[static_cast<size_t> (instIdx)][static_cast<size_t> (opSlot)];

        if (def < 0)
            return fnInfo.globalConstInt[static_cast<size_t> (val)];

        const auto& defInst = insts[static_cast<size_t> (def)];

        if (defInst.op == YdspIrOp::constI)
            return defInst.ivalue;

        if (defInst.op == YdspIrOp::movI)
        {
            const int srcNext = nextDef[static_cast<size_t> (def)][0];

            if (srcNext < 0 || srcNext > instIdx)
                return resolveConstIntAt (def, 0);
        }

        return std::nullopt;
    }

    void resolveBaseOffsetAt (int instIdx, int opSlot, int& outBaseReg, int& outBaseDef, int64_t& outOffset) const noexcept
    {
        const auto& insts = fn.blocks[static_cast<size_t> (blockIndex)].insts;
        const int val = getOperand (insts[static_cast<size_t> (instIdx)], opSlot);

        if (const auto c = resolveConstIntAt (instIdx, opSlot); c.has_value())
        {
            outBaseReg = -1;
            outBaseDef = -1;
            outOffset = *c;
            return;
        }

        const int def = reachingDef[static_cast<size_t> (instIdx)][static_cast<size_t> (opSlot)];

        if (def >= 0)
        {
            const auto& defInst = insts[static_cast<size_t> (def)];

            if ((defInst.op == YdspIrOp::addI || defInst.op == YdspIrOp::subI) && ! defInst.saturatingIntegerArithmetic)
            {
                if (const auto rhs = resolveConstIntAt (def, 1); rhs.has_value())
                {
                    const int baseNext = nextDef[static_cast<size_t> (def)][0];

                    if (baseNext < 0 || baseNext > instIdx)
                    {
                        outBaseReg = defInst.a;
                        outBaseDef = reachingDef[static_cast<size_t> (def)][0];
                        outOffset = defInst.op == YdspIrOp::addI ? *rhs : -(*rhs);
                        return;
                    }
                }

                if (defInst.op == YdspIrOp::addI)
                {
                    if (const auto lhs = resolveConstIntAt (def, 0); lhs.has_value())
                    {
                        const int baseNext = nextDef[static_cast<size_t> (def)][1];

                        if (baseNext < 0 || baseNext > instIdx)
                        {
                            outBaseReg = defInst.b;
                            outBaseDef = reachingDef[static_cast<size_t> (def)][1];
                            outOffset = *lhs;
                            return;
                        }
                    }
                }
            }
        }

        outBaseReg = val;
        outBaseDef = def;
        outOffset = 0;
    }

    SlpMemAddr resolveMemAddr (int instIdx) const noexcept
    {
        const auto& inst = fn.blocks[static_cast<size_t> (blockIndex)].insts[static_cast<size_t> (instIdx)];
        SlpMemAddr addr;

        if (isMemoryBarrierOp (inst.op))
        {
            addr.kind = SlpMemAddr::Kind::barrier;
            addr.isStore = true;
            return addr;
        }

        if (isStateScalarMemoryOp (inst.op))
        {
            addr.kind = SlpMemAddr::Kind::scalarSlot;
            addr.isStore = inst.op == YdspIrOp::storeStateF || inst.op == YdspIrOp::storeStateI
                        || inst.op == YdspIrOp::storeParam || inst.op == YdspIrOp::storeParamOut
                        || inst.op == YdspIrOp::storeEventFieldF || inst.op == YdspIrOp::storeEventFieldI;
            const int valId = addr.isStore ? inst.a : inst.result;
            addr.elemType = (valId >= 0 && valId < numValues) ? fn.valueTypes[static_cast<size_t> (valId)] : inferTypeFromOp (inst.op);
            addr.memIndex = addr.isStore ? inst.memIndex : (ydspFirstOperandIsSlot (inst.op) ? inst.a : inst.memIndex);
            return addr;
        }

        if (inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::storeStateArrayF
            || inst.op == YdspIrOp::loadStateArrayI || inst.op == YdspIrOp::storeStateArrayI)
        {
            addr.kind = SlpMemAddr::Kind::stateArray;
            addr.isStore = inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeStateArrayI;
            const int valId = addr.isStore ? inst.b : inst.result;
            addr.elemType = (valId >= 0 && valId < numValues) ? fn.valueTypes[static_cast<size_t> (valId)] : inferTypeFromOp (inst.op);
            addr.memIndex = inst.memIndex;
            addr.lanes = fn.laneCountOf (valId);
            resolveBaseOffsetAt (instIdx, 0, addr.baseReg, addr.baseDef, addr.offset);
            return addr;
        }

        if (inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadOutput || inst.op == YdspIrOp::storeOutput)
        {
            addr.kind = SlpMemAddr::Kind::stream;
            addr.isStore = inst.op == YdspIrOp::storeOutput;
            addr.isInput = inst.op == YdspIrOp::loadInput;
            const int valId = addr.isStore ? inst.b : inst.result;
            addr.elemType = (valId >= 0 && valId < numValues) ? fn.valueTypes[static_cast<size_t> (valId)] : YdspValueType::float32Type;
            addr.memIndex = inst.memIndex;
            addr.lanes = fn.laneCountOf (valId);
            resolveBaseOffsetAt (instIdx, 0, addr.baseReg, addr.baseDef, addr.offset);
            return addr;
        }

        return addr;
    }

    int findPreheaderForBlock() const noexcept
    {
        const YdspIrLoop* innermost = nullptr;

        for (const auto& loop : fn.loops)
        {
            if (loop.unrolled || loop.headerBlock < 1)
                continue;

            if (loop.headerBlock < blockIndex && blockIndex < loop.exitBlock)
            {
                if (innermost == nullptr || loop.headerBlock > innermost->headerBlock)
                    innermost = &loop;
            }
        }

        if (innermost == nullptr)
            return -1;

        const int preheader = innermost->headerBlock - 1;
        const auto& preBlock = fn.blocks[static_cast<size_t> (preheader)];

        if (preBlock.term != YdspIrTerm::fallthrough
            && ! (preBlock.term == YdspIrTerm::branch && preBlock.termTarget == innermost->headerBlock))
            return -1;

        return preheader;
    }

    bool canHoistSplatToPreheader (int scalarValue, int preheader) const noexcept
    {
        if (preheader < 0 || scalarValue < 0 || scalarValue >= numValues)
            return false;

        if (fnInfo.globalDefCount[static_cast<size_t> (scalarValue)] != 1)
            return false;

        const int defBlock = fnInfo.globalDefBlock[static_cast<size_t> (scalarValue)];

        if (defBlock < 0 || defBlock == blockIndex)
            return false;

        return fnInfo.dom.dominates (defBlock, preheader);
    }

    void collectExistingVectorForms()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (blockIndex)].insts;

        for (int i = 0; i < numInsts; ++i)
        {
            const auto& inst = insts[static_cast<size_t> (i)];

            if (inst.op == YdspIrOp::vsplat && inst.result >= 0 && fn.laneCountOf (inst.result) == lanes
                && inst.a >= 0 && inst.a < numValues && fnInfo.globalDefCount[static_cast<size_t> (inst.a)] == 1)
            {
                existingSplats.emplace_back (inst.a, inst.result);
            }

            if (inst.op == YdspIrOp::vinsertF && inst.memIndex == lanes - 1 && inst.result >= 0
                && fn.laneCountOf (inst.result) == lanes)
            {
                ExistingGather gather;
                gather.instIndex = i;
                gather.vectorValue = inst.result;
                gather.scalars.assign (static_cast<size_t> (lanes), -1);
                gather.reachingDefs.assign (static_cast<size_t> (lanes), -1);

                int cur = i;
                bool ok = true;

                for (int lane = lanes - 1; lane >= 0; --lane)
                {
                    if (cur < 0 || cur >= numInsts)
                    {
                        ok = false;
                        break;
                    }

                    const auto& step = insts[static_cast<size_t> (cur)];

                    if (step.op != YdspIrOp::vinsertF || step.memIndex != lane || step.b < 0)
                    {
                        ok = false;
                        break;
                    }

                    gather.scalars[static_cast<size_t> (lane)] = step.b;
                    gather.reachingDefs[static_cast<size_t> (lane)] = reachingDef[static_cast<size_t> (cur)][1];

                    if (lane == 0)
                    {
                        if (step.a != -1)
                            ok = false;
                    }
                    else
                    {
                        cur = reachingDef[static_cast<size_t> (cur)][0];
                    }
                }

                if (ok)
                    existingGathers.push_back (std::move (gather));
            }
        }

        // Also recognize contiguous sets of W vextractF instructions from the same vector.
        std::unordered_map<int, std::vector<int>> extractsByVec;

        for (int i = 0; i < numInsts; ++i)
        {
            const auto& inst = insts[static_cast<size_t> (i)];

            if (inst.op == YdspIrOp::vextractF && inst.a >= 0 && inst.result >= 0
                && inst.memIndex >= 0 && inst.memIndex < lanes
                && fn.laneCountOf (inst.a) == lanes)
            {
                auto& list = extractsByVec[inst.a];

                if (list.empty())
                    list.assign (static_cast<size_t> (lanes), -1);

                if (list[static_cast<size_t> (inst.memIndex)] < 0)
                    list[static_cast<size_t> (inst.memIndex)] = i;
            }
        }

        for (const auto& [vecId, idxList] : extractsByVec)
        {
            bool complete = true;
            int maxIdx = -1;

            for (int lane = 0; lane < lanes; ++lane)
            {
                if (idxList[static_cast<size_t> (lane)] < 0)
                {
                    complete = false;
                    break;
                }

                maxIdx = std::max (maxIdx, idxList[static_cast<size_t> (lane)]);
            }

            if (! complete)
                continue;

            ExistingGather gather;
            gather.instIndex = maxIdx;
            gather.vectorValue = vecId;
            gather.scalars.resize (static_cast<size_t> (lanes));
            gather.reachingDefs.resize (static_cast<size_t> (lanes));

            for (int lane = 0; lane < lanes; ++lane)
            {
                const int idx = idxList[static_cast<size_t> (lane)];
                gather.scalars[static_cast<size_t> (lane)] = insts[static_cast<size_t> (idx)].result;
                gather.reachingDefs[static_cast<size_t> (lane)] = idx;
            }

            existingGathers.push_back (std::move (gather));
        }

        const int preheader = findPreheaderForBlock();

        if (preheader >= 0)
        {
            for (const auto& inst : fn.blocks[static_cast<size_t> (preheader)].insts)
                if (inst.op == YdspIrOp::vsplat && inst.result >= 0 && fn.laneCountOf (inst.result) == lanes
                    && inst.a >= 0 && inst.a < numValues && fnInfo.globalDefCount[static_cast<size_t> (inst.a)] == 1)
                    preheaderSplats[inst.a] = inst.result;
        }
    }
};

//==============================================================================

enum class SlpNodeKind
{
    vectorOp,
    contiguousLoad,
    contiguousStore,
    splat,
    gather,
    reuseExisting,
    reduceAdd
};

struct SlpNode
{
    SlpNodeKind kind = SlpNodeKind::vectorOp;
    YdspIrOp op = YdspIrOp::addF;
    YdspValueType resultType = YdspValueType::float32Type;

    std::vector<int> insts;
    std::vector<int> children;

    // splat
    int splatScalar = -1;
    int splatReachingDef = -1;
    bool hoistSplat = false;

    // gather / reuseExisting
    std::vector<int> gatherScalars;
    std::vector<int> gatherReachingDefs;
    bool hoistGather = false;
    int existingVectorId = -1;
    int existingVectorDefInst = -1;

    // external scalar extraction
    std::vector<bool> needsExtract;
    std::vector<int> absorbedMoves; // -1 or movF inst index in block

    // reduceAdd (supports slicing W leaves out of a larger N-leaf addF tree)
    int reduceRootInst = -1;
    std::vector<std::pair<int, int>> remainingReduceLeaves; // (scalarValue, reachingDef)
};

struct SlpTree
{
    int lanes = 4;
    int rootNode = -1;
    std::vector<SlpNode> nodes;

    // Non-contiguous store / movF seeds whose operands come from rootNode
    bool isScatterStoreSeed = false;
    bool isScatterMovSeed = false;
    std::vector<int> scatterInsts;

    int preheaderBlock = -1;

    // Memoization within this tree for DAG sharing
    std::map<std::vector<std::pair<int, int>>, int> memo;
    std::unordered_set<int> usedInsts;

    // Cost accounting
    int oldCost = 0;
    int newCost = 0;
    int vectorOpCount = 0;
    bool hasContiguousMem = false;
    bool hasReduction = false;
};

//==============================================================================

class SlpTreeBuilder
{
public:
    SlpTreeBuilder (const SlpBlockContext& context, const std::vector<bool>& blockConsumed)
        : ctx (context)
        , consumed (blockConsumed)
    {
        tree.lanes = ctx.lanes;
        tree.preheaderBlock = ctx.findPreheaderForBlock();
    }

    bool buildForContiguousStores (const std::vector<int>& storeInsts)
    {
        const auto& blockInsts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        const auto firstOp = blockInsts[static_cast<size_t> (storeInsts[0])].op;
        const int valSlot = (firstOp == YdspIrOp::storeStateF) ? 0 : 1;

        std::vector<int> scalars (static_cast<size_t> (ctx.lanes));
        std::vector<int> rDefs (static_cast<size_t> (ctx.lanes));

        for (int k = 0; k < ctx.lanes; ++k)
        {
            const int idx = storeInsts[static_cast<size_t> (k)];
            scalars[static_cast<size_t> (k)] = getOperand (blockInsts[static_cast<size_t> (idx)], valSlot);
            rDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (idx)][static_cast<size_t> (valSlot)];
        }

        for (const int idx : storeInsts)
            tree.usedInsts.insert (idx);

        const int childNode = buildNode (scalars, rDefs, YdspValueType::float32Type, 0);

        if (childNode < 0)
            return false;

        SlpNode storeNode;
        storeNode.kind = SlpNodeKind::contiguousStore;
        storeNode.op = firstOp;
        storeNode.resultType = YdspValueType::float32Type;
        storeNode.insts = storeInsts;
        storeNode.children.push_back (childNode);

        tree.rootNode = addNode (std::move (storeNode));
        tree.hasContiguousMem = true;

        return finalizeAndCheckProfitability();
    }

    bool buildForScatterStores (const std::vector<int>& storeInsts)
    {
        const auto& blockInsts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        const auto firstOp = blockInsts[static_cast<size_t> (storeInsts[0])].op;
        const int valSlot = (firstOp == YdspIrOp::storeStateF) ? 0 : 1;

        std::vector<int> scalars (static_cast<size_t> (ctx.lanes));
        std::vector<int> rDefs (static_cast<size_t> (ctx.lanes));

        for (int k = 0; k < ctx.lanes; ++k)
        {
            const int idx = storeInsts[static_cast<size_t> (k)];
            scalars[static_cast<size_t> (k)] = getOperand (blockInsts[static_cast<size_t> (idx)], valSlot);
            rDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (idx)][static_cast<size_t> (valSlot)];
        }

        const int childNode = buildNode (scalars, rDefs, YdspValueType::float32Type, 0);

        if (childNode < 0)
            return false;

        if (tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::gather
            || tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::splat
            || tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::reuseExisting)
            return false;

        tree.rootNode = childNode;
        tree.isScatterStoreSeed = true;
        tree.scatterInsts = storeInsts;

        return finalizeAndCheckProfitability();
    }

    bool buildForScatterMoves (const std::vector<int>& movInsts)
    {
        const auto& blockInsts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;

        std::vector<int> scalars (static_cast<size_t> (ctx.lanes));
        std::vector<int> rDefs (static_cast<size_t> (ctx.lanes));

        for (int k = 0; k < ctx.lanes; ++k)
        {
            const int idx = movInsts[static_cast<size_t> (k)];
            scalars[static_cast<size_t> (k)] = blockInsts[static_cast<size_t> (idx)].a;
            rDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (idx)][0];
            tree.usedInsts.insert (idx);
        }

        const int childNode = buildNode (scalars, rDefs, YdspValueType::float32Type, 0);

        if (childNode < 0)
            return false;

        if (tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::gather
            || tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::splat
            || tree.nodes[static_cast<size_t> (childNode)].kind == SlpNodeKind::reuseExisting)
            return false;

        tree.rootNode = childNode;
        tree.isScatterMovSeed = true;
        tree.scatterInsts = movInsts;

        return finalizeAndCheckProfitability();
    }

    bool buildForReduction (int rootAddInst,
                            const std::vector<int>& internalAddInsts,
                            const std::vector<std::pair<int, int>>& leafWindow,
                            const std::vector<std::pair<int, int>>& remainingLeaves)
    {
        for (const int idx : internalAddInsts)
            tree.usedInsts.insert (idx);

        std::vector<int> scalars (static_cast<size_t> (ctx.lanes));
        std::vector<int> rDefs (static_cast<size_t> (ctx.lanes));

        for (int k = 0; k < ctx.lanes; ++k)
        {
            scalars[static_cast<size_t> (k)] = leafWindow[static_cast<size_t> (k)].first;
            rDefs[static_cast<size_t> (k)] = leafWindow[static_cast<size_t> (k)].second;
        }

        const int childNode = buildNode (scalars, rDefs, YdspValueType::float32Type, 0);

        if (childNode < 0)
            return false;

        SlpNode redNode;
        redNode.kind = SlpNodeKind::reduceAdd;
        redNode.op = YdspIrOp::vreduceAddF;
        redNode.resultType = YdspValueType::float32Type;
        redNode.insts = internalAddInsts;
        redNode.children.push_back (childNode);
        redNode.reduceRootInst = rootAddInst;
        redNode.remainingReduceLeaves = remainingLeaves;

        tree.rootNode = addNode (std::move (redNode));
        tree.hasReduction = true;

        return finalizeAndCheckProfitability();
    }

    const SlpTree& getTree() const noexcept { return tree; }

private:
    int addNode (SlpNode&& node)
    {
        const int id = static_cast<int> (tree.nodes.size());
        tree.nodes.push_back (std::move (node));
        return id;
    }

    bool isFloat32Scalar (int value) const noexcept
    {
        return value >= 0 && value < ctx.numValues
            && ctx.fn.valueTypes[static_cast<size_t> (value)] == YdspValueType::float32Type
            && ctx.fn.laneCountOf (value) == 1;
    }

    bool isBoolScalar (int value) const noexcept
    {
        return value >= 0 && value < ctx.numValues
            && ctx.fn.valueTypes[static_cast<size_t> (value)] == YdspValueType::boolType
            && ctx.fn.laneCountOf (value) == 1;
    }

    std::optional<int> findExistingGatherOrExtract (const std::vector<int>& scalars,
                                                    const std::vector<int>& rDefs) const noexcept
    {
        for (const auto& g : ctx.existingGathers)
        {
            bool match = true;

            for (int k = 0; k < ctx.lanes; ++k)
            {
                if (g.scalars[static_cast<size_t> (k)] != scalars[static_cast<size_t> (k)]
                    || g.reachingDefs[static_cast<size_t> (k)] != rDefs[static_cast<size_t> (k)])
                {
                    match = false;
                    break;
                }
            }

            if (match)
                return g.vectorValue;
        }

        return std::nullopt;
    }

    int findExistingGatherDefInst (int vecId) const noexcept
    {
        for (const auto& g : ctx.existingGathers)
            if (g.vectorValue == vecId)
                return g.instIndex;

        return -1;
    }

    bool isContiguousLoadGroup (const std::vector<int>& defs) const noexcept
    {
        const auto& insts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        const auto op0 = insts[static_cast<size_t> (defs[0])].op;

        if (op0 != YdspIrOp::loadStateArrayF && op0 != YdspIrOp::loadInput && op0 != YdspIrOp::loadOutput)
            return false;

        const auto& addr0 = ctx.memAddrs[static_cast<size_t> (defs[0])];

        if (op0 == YdspIrOp::loadStateArrayF && static_cast<int64_t> (addr0.memIndex) + addr0.offset < 0)
            return false;

        for (int k = 1; k < ctx.lanes; ++k)
        {
            const auto& addrK = ctx.memAddrs[static_cast<size_t> (defs[static_cast<size_t> (k)])];

            if (addrK.kind != addr0.kind
                || addrK.memIndex != addr0.memIndex
                || addrK.baseReg != addr0.baseReg
                || (addrK.baseReg >= 0 && addrK.baseDef != addr0.baseDef)
                || addrK.offset != addr0.offset + k)
                return false;
        }

        return true;
    }

    void alignCommutativeOperands (const std::vector<int>& defs,
                                   YdspIrOp op,
                                   std::vector<int>& opA,
                                   std::vector<int>& defA,
                                   std::vector<int>& opB,
                                   std::vector<int>& defB) const
    {
        const auto& insts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        const int W = ctx.lanes;
        opA.resize (static_cast<size_t> (W));
        defA.resize (static_cast<size_t> (W));
        opB.resize (static_cast<size_t> (W));
        defB.resize (static_cast<size_t> (W));

        for (int k = 0; k < W; ++k)
        {
            const int d = defs[static_cast<size_t> (k)];
            const auto [ua, da] = ctx.unwrapCopy (insts[static_cast<size_t> (d)].a, ctx.reachingDef[static_cast<size_t> (d)][0], d);
            const auto [ub, db] = ctx.unwrapCopy (insts[static_cast<size_t> (d)].b, ctx.reachingDef[static_cast<size_t> (d)][1], d);
            opA[static_cast<size_t> (k)] = ua;
            defA[static_cast<size_t> (k)] = da;
            opB[static_cast<size_t> (k)] = ub;
            defB[static_cast<size_t> (k)] = db;
        }

        if (! isSlpCommutativeOp (op))
            return;

        const auto isConstEqual = [this] (int v1, int v2) noexcept
        {
            if (v1 < 0 || v2 < 0 || v1 >= ctx.numValues || v2 >= ctx.numValues)
                return false;

            const auto c1 = ctx.fnInfo.globalConstFloat[static_cast<size_t> (v1)];
            const auto c2 = ctx.fnInfo.globalConstFloat[static_cast<size_t> (v2)];
            return c1.has_value() && c2.has_value() && std::bit_cast<uint64_t> (*c1) == std::bit_cast<uint64_t> (*c2);
        };

        const auto matchesSlot = [&] (int v1, int d1, int v2, int d2) noexcept
        {
            if (v1 == v2 && d1 == d2)
                return true;

            if (isConstEqual (v1, v2))
                return true;

            if (d1 >= 0 && d2 >= 0)
                return insts[static_cast<size_t> (d1)].op == insts[static_cast<size_t> (d2)].op;

            return false;
        };

        // Check if aligning against lane 0's slot A (or swapping lane 0 if B aligns better) improves consistency.
        auto tryAlign = [&] (bool swapZero) -> int
        {
            int score = 0;
            const int refA = swapZero ? opB[0] : opA[0];
            const int refDefA = swapZero ? defB[0] : defA[0];
            const int refB = swapZero ? opA[0] : opB[0];
            const int refDefB = swapZero ? defA[0] : defB[0];

            for (int k = 1; k < W; ++k)
            {
                const int keep = (matchesSlot (opA[static_cast<size_t> (k)], defA[static_cast<size_t> (k)], refA, refDefA) ? 1 : 0)
                               + (matchesSlot (opB[static_cast<size_t> (k)], defB[static_cast<size_t> (k)], refB, refDefB) ? 1 : 0);
                const int swap = (matchesSlot (opB[static_cast<size_t> (k)], defB[static_cast<size_t> (k)], refA, refDefA) ? 1 : 0)
                               + (matchesSlot (opA[static_cast<size_t> (k)], defA[static_cast<size_t> (k)], refB, refDefB) ? 1 : 0);
                score += std::max (keep, swap);
            }

            return score;
        };

        const int keepScore = tryAlign (false);
        const int swapZeroScore = tryAlign (true);
        const bool swapZero = swapZeroScore > keepScore;

        if (swapZero)
        {
            std::swap (opA[0], opB[0]);
            std::swap (defA[0], defB[0]);
        }

        for (int k = 1; k < W; ++k)
        {
            const int keep = (matchesSlot (opA[static_cast<size_t> (k)], defA[static_cast<size_t> (k)], opA[0], defA[0]) ? 1 : 0)
                           + (matchesSlot (opB[static_cast<size_t> (k)], defB[static_cast<size_t> (k)], opB[0], defB[0]) ? 1 : 0);
            const int swap = (matchesSlot (opB[static_cast<size_t> (k)], defB[static_cast<size_t> (k)], opA[0], defA[0]) ? 1 : 0)
                           + (matchesSlot (opA[static_cast<size_t> (k)], defA[static_cast<size_t> (k)], opB[0], defB[0]) ? 1 : 0);

            if (swap > keep)
            {
                std::swap (opA[static_cast<size_t> (k)], opB[static_cast<size_t> (k)]);
                std::swap (defA[static_cast<size_t> (k)], defB[static_cast<size_t> (k)]);
            }
        }
    }

    int buildNode (const std::vector<int>& rawScalars,
                   const std::vector<int>& rawRDefs,
                   YdspValueType expectedType,
                   int depth)
    {
        const int W = ctx.lanes;
        std::vector<int> scalars (static_cast<size_t> (W));
        std::vector<int> rDefs (static_cast<size_t> (W));

        for (int k = 0; k < W; ++k)
        {
            if (expectedType == YdspValueType::float32Type)
            {
                const auto [us, ud] = ctx.unwrapCopy (rawScalars[static_cast<size_t> (k)], rawRDefs[static_cast<size_t> (k)], ctx.numInsts);
                scalars[static_cast<size_t> (k)] = us;
                rDefs[static_cast<size_t> (k)] = ud;

                if (! isFloat32Scalar (us))
                    return -1;
            }
            else
            {
                scalars[static_cast<size_t> (k)] = rawScalars[static_cast<size_t> (k)];
                rDefs[static_cast<size_t> (k)] = rawRDefs[static_cast<size_t> (k)];

                if (! isBoolScalar (scalars[static_cast<size_t> (k)]))
                    return -1;
            }
        }

        std::vector<std::pair<int, int>> key (static_cast<size_t> (W));

        for (int k = 0; k < W; ++k)
            key[static_cast<size_t> (k)] = { scalars[static_cast<size_t> (k)], rDefs[static_cast<size_t> (k)] };

        if (const auto it = tree.memo.find (key); it != tree.memo.end())
            return it->second;

        auto makeGatherFallback = [&]() -> int
        {
            if (expectedType != YdspValueType::float32Type)
                return -1;

            if (const auto existing = findExistingGatherOrExtract (scalars, rDefs); existing.has_value())
            {
                SlpNode reuse;
                reuse.kind = SlpNodeKind::reuseExisting;
                reuse.resultType = YdspValueType::float32Type;
                reuse.gatherScalars = scalars;
                reuse.gatherReachingDefs = rDefs;
                reuse.existingVectorId = *existing;
                reuse.existingVectorDefInst = findExistingGatherDefInst (*existing);
                const int id = addNode (std::move (reuse));
                tree.memo[key] = id;
                return id;
            }

            SlpNode gather;
            gather.kind = SlpNodeKind::gather;
            gather.resultType = YdspValueType::float32Type;
            gather.gatherScalars = scalars;
            gather.gatherReachingDefs = rDefs;
            gather.hoistGather = tree.preheaderBlock >= 0
                              && std::all_of (scalars.begin(), scalars.end(), [&] (int s)
                                 {
                                     return ctx.canHoistSplatToPreheader (s, tree.preheaderBlock);
                                 });
            const int id = addNode (std::move (gather));
            tree.memo[key] = id;
            return id;
        };

        // Check splat (float32 only - bool masks cannot be safely splatted as 0/1 vs 0/-1).
        if (expectedType == YdspValueType::float32Type)
        {
            bool allSameValue = true;
            bool allSameConst = true;
            const auto c0 = ctx.fnInfo.globalConstFloat[static_cast<size_t> (scalars[0])];

            for (int k = 1; k < W; ++k)
            {
                if (scalars[static_cast<size_t> (k)] != scalars[0] || rDefs[static_cast<size_t> (k)] != rDefs[0])
                    allSameValue = false;

                const auto ck = ctx.fnInfo.globalConstFloat[static_cast<size_t> (scalars[static_cast<size_t> (k)])];

                if (! c0.has_value() || ! ck.has_value()
                    || std::bit_cast<uint64_t> (*c0) != std::bit_cast<uint64_t> (*ck))
                    allSameConst = false;
            }

            if (allSameValue || allSameConst)
            {
                SlpNode splat;
                splat.kind = SlpNodeKind::splat;
                splat.op = YdspIrOp::vsplat;
                splat.resultType = YdspValueType::float32Type;
                splat.splatScalar = scalars[0];
                splat.splatReachingDef = rDefs[0];
                splat.hoistSplat = ctx.canHoistSplatToPreheader (scalars[0], tree.preheaderBlock);
                const int id = addNode (std::move (splat));
                tree.memo[key] = id;
                return id;
            }
        }

        if (depth >= 32)
            return makeGatherFallback();

        // Check if all lanes are distinct, unconsumed, mutually independent instructions in this block with matching op.
        const auto& blockInsts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        bool allLocal = true;

        for (int k = 0; k < W; ++k)
        {
            const int d = rDefs[static_cast<size_t> (k)];

            if (d < 0 || consumed[static_cast<size_t> (d)] || tree.usedInsts.count (d) != 0)
            {
                allLocal = false;
                break;
            }
        }

        if (! allLocal || ! ctx.areMutuallyIndependent (rDefs))
            return makeGatherFallback();

        const auto op0 = blockInsts[static_cast<size_t> (rDefs[0])].op;

        for (int k = 1; k < W; ++k)
        {
            if (blockInsts[static_cast<size_t> (rDefs[static_cast<size_t> (k)])].op != op0)
                return makeGatherFallback();
        }

        // Contiguous memory load (float32).
        if (expectedType == YdspValueType::float32Type
            && (op0 == YdspIrOp::loadStateArrayF || op0 == YdspIrOp::loadInput || op0 == YdspIrOp::loadOutput))
        {
            if (isContiguousLoadGroup (rDefs))
            {
                for (const int d : rDefs)
                    tree.usedInsts.insert (d);

                SlpNode loadNode;
                loadNode.kind = SlpNodeKind::contiguousLoad;
                loadNode.op = op0;
                loadNode.resultType = YdspValueType::float32Type;
                loadNode.insts = rDefs;
                const int id = addNode (std::move (loadNode));
                tree.memo[key] = id;
                return id;
            }

            return makeGatherFallback();
        }

        const bool validOp = expectedType == YdspValueType::float32Type
                               ? (isSlpFloatOp (op0, ctx.fn.vectorMathEnabled, ctx.fn.packedFmaAvailable, ctx.fn.fastMath) || op0 == YdspIrOp::selectB)
                               : isSlpBoolOp (op0);

        if (! validOp)
            return makeGatherFallback();

        // Reserve instructions in usedInsts before recursing (rollback on failure).
        for (const int d : rDefs)
            tree.usedInsts.insert (d);

        auto rollback = [&]()
        {
            for (const int d : rDefs)
                tree.usedInsts.erase (d);
        };

        std::vector<int> childNodes;
        const int numOps = slpValueOperandCount (op0);

        if (op0 == YdspIrOp::selectB)
        {
            std::vector<int> condScalars (static_cast<size_t> (W));
            std::vector<int> condDefs (static_cast<size_t> (W));
            std::vector<int> bScalars (static_cast<size_t> (W));
            std::vector<int> bDefs (static_cast<size_t> (W));
            std::vector<int> cScalars (static_cast<size_t> (W));
            std::vector<int> cDefs (static_cast<size_t> (W));

            for (int k = 0; k < W; ++k)
            {
                const int d = rDefs[static_cast<size_t> (k)];
                condScalars[static_cast<size_t> (k)] = blockInsts[static_cast<size_t> (d)].a;
                condDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (d)][0];
                bScalars[static_cast<size_t> (k)] = blockInsts[static_cast<size_t> (d)].b;
                bDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (d)][1];
                cScalars[static_cast<size_t> (k)] = blockInsts[static_cast<size_t> (d)].c;
                cDefs[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (d)][2];
            }

            const int condChild = buildNode (condScalars, condDefs, YdspValueType::boolType, depth + 1);
            const int bChild = condChild >= 0 ? buildNode (bScalars, bDefs, YdspValueType::float32Type, depth + 1) : -1;
            const int cChild = bChild >= 0 ? buildNode (cScalars, cDefs, YdspValueType::float32Type, depth + 1) : -1;

            if (condChild < 0 || bChild < 0 || cChild < 0)
            {
                rollback();
                return makeGatherFallback();
            }

            childNodes = { condChild, bChild, cChild };
        }
        else if (numOps == 2)
        {
            std::vector<int> opA, defA, opB, defB;
            alignCommutativeOperands (rDefs, op0, opA, defA, opB, defB);

            const auto operandType = (expectedType == YdspValueType::boolType && isFloatComparison (op0))
                                       ? YdspValueType::float32Type
                                       : expectedType;

            const int c0 = buildNode (opA, defA, operandType, depth + 1);
            const int c1 = c0 >= 0 ? buildNode (opB, defB, operandType, depth + 1) : -1;

            if (c0 < 0 || c1 < 0)
            {
                rollback();
                return makeGatherFallback();
            }

            childNodes = { c0, c1 };
        }
        else
        {
            for (int opSlot = 0; opSlot < numOps; ++opSlot)
            {
                std::vector<int> s (static_cast<size_t> (W));
                std::vector<int> d (static_cast<size_t> (W));

                for (int k = 0; k < W; ++k)
                {
                    const int instIdx = rDefs[static_cast<size_t> (k)];
                    s[static_cast<size_t> (k)] = getOperand (blockInsts[static_cast<size_t> (instIdx)], opSlot);
                    d[static_cast<size_t> (k)] = ctx.reachingDef[static_cast<size_t> (instIdx)][static_cast<size_t> (opSlot)];
                }

                const int c = buildNode (s, d, expectedType, depth + 1);

                if (c < 0)
                {
                    rollback();
                    return makeGatherFallback();
                }

                childNodes.push_back (c);
            }
        }

        SlpNode vecNode;
        vecNode.kind = SlpNodeKind::vectorOp;
        vecNode.op = op0;
        vecNode.resultType = expectedType;
        vecNode.insts = rDefs;
        vecNode.children = std::move (childNodes);

        const int id = addNode (std::move (vecNode));
        tree.memo[key] = id;
        return id;
    }

    bool finalizeAndCheckProfitability()
    {
        const auto& blockInsts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;
        const int W = ctx.lanes;

        std::set<std::pair<int, int>> uniqueSplats;
        std::set<std::pair<int, int>> hoistedSplats;
        std::unordered_set<int> absorbedMovInsts;

        for (auto& node : tree.nodes)
        {
            if (node.kind == SlpNodeKind::splat)
            {
                const std::pair<int, int> key { node.splatScalar, node.splatReachingDef };

                if (node.hoistSplat)
                {
                    if (ctx.preheaderSplats.count (node.splatScalar) == 0)
                        hoistedSplats.insert (key);
                }
                else
                {
                    bool alreadyExists = false;

                    if (node.splatReachingDef < 0 && ctx.fnInfo.globalDefCount[static_cast<size_t> (node.splatScalar)] == 1)
                    {
                        for (const auto& [sv, vv] : ctx.existingSplats)
                        {
                            if (sv == node.splatScalar)
                            {
                                alreadyExists = true;
                                break;
                            }
                        }
                    }

                    if (! alreadyExists)
                        uniqueSplats.insert (key);
                }

                continue;
            }

            if (node.kind == SlpNodeKind::vectorOp || node.kind == SlpNodeKind::contiguousLoad)
            {
                node.needsExtract.assign (static_cast<size_t> (W), false);
                node.absorbedMoves.assign (static_cast<size_t> (W), -1);

                for (int k = 0; k < W; ++k)
                {
                    const int u = node.insts[static_cast<size_t> (k)];
                    const int r = blockInsts[static_cast<size_t> (u)].result;

                    std::vector<int> extUses;

                    for (const auto& [useInst, useSlot] : ctx.resultUsesInBlock[static_cast<size_t> (u)])
                    {
                        (void) useSlot;

                        if (tree.usedInsts.count (useInst) == 0)
                            extUses.push_back (useInst);
                    }

                    const bool liveOut = ctx.isLiveOutAfter (u);

                    if (extUses.empty() && ! liveOut)
                        continue;

                    if (node.resultType == YdspValueType::boolType)
                        return false; // No vextractB exists.

                    // Absorb a single `movF dst = r` external use directly into `vextractF dst = vec, k`.
                    if (extUses.size() == 1 && ! liveOut)
                    {
                        const int movIdx = extUses[0];
                        const auto& movInst = blockInsts[static_cast<size_t> (movIdx)];

                        if (movInst.op == YdspIrOp::movF && movInst.a == r && movInst.result >= 0
                            && ! consumed[static_cast<size_t> (movIdx)]
                            && absorbedMovInsts.count (movIdx) == 0)
                        {
                            bool dstSafe = true;

                            for (int mid = u + 1; mid < movIdx; ++mid)
                            {
                                const auto& between = blockInsts[static_cast<size_t> (mid)];

                                if (between.result == movInst.result)
                                {
                                    dstSafe = false;
                                    break;
                                }

                                for (int slot = 0; slot < 3; ++slot)
                                {
                                    if (isValueIdOperand (between.op, slot)
                                        && getOperand (between, slot) == movInst.result
                                        && tree.usedInsts.count (mid) == 0)
                                    {
                                        dstSafe = false;
                                        break;
                                    }
                                }

                                if (! dstSafe)
                                    break;
                            }

                            if (dstSafe)
                            {
                                node.absorbedMoves[static_cast<size_t> (k)] = movIdx;
                                absorbedMovInsts.insert (movIdx);
                                continue;
                            }
                        }
                    }

                    node.needsExtract[static_cast<size_t> (k)] = true;
                }
            }
        }

        int oldCost = 0;
        int newCost = static_cast<int> (uniqueSplats.size());
        int vecOpCount = 0;

        for (const auto& node : tree.nodes)
        {
            switch (node.kind)
            {
                case SlpNodeKind::vectorOp:
                    oldCost += W;
                    newCost += 1;
                    ++vecOpCount;
                    break;

                case SlpNodeKind::contiguousLoad:
                case SlpNodeKind::contiguousStore:
                    oldCost += W;
                    newCost += 1;
                    break;

                case SlpNodeKind::gather:
                    if (! node.hoistGather)
                        newCost += W;
                    break;

                case SlpNodeKind::reduceAdd:
                    oldCost += W - 1;
                    newCost += 1;
                    break;

                case SlpNodeKind::splat:
                case SlpNodeKind::reuseExisting:
                    break;
            }

            for (size_t k = 0; k < node.needsExtract.size(); ++k)
            {
                if (node.needsExtract[k])
                    newCost += 1;
                else if (node.absorbedMoves[k] >= 0)
                {
                    oldCost += 1;
                    newCost += 1;
                }
            }
        }

        if (tree.isScatterStoreSeed)
        {
            for (const int sIdx : tree.scatterInsts)
                if (absorbedMovInsts.count (sIdx) == 0)
                    newCost += 1; // vextractF for store operand
        }
        else if (tree.isScatterMovSeed)
        {
            oldCost += W;
            newCost += W; // each movF replaced 1-for-1 by vextractF
        }

        tree.oldCost = oldCost;
        tree.newCost = newCost;
        tree.vectorOpCount = vecOpCount;

        if (newCost >= oldCost)
            return false;

        // Avoid pure load->store copy trees (no vector arithmetic or reduction)
        // when the touched state array also has unvectorized scalar accesses in
        // the same block: a 128-bit vector store feeding 32-bit scalar loads on
        // the next sample causes store-to-load forwarding stalls.
        if (vecOpCount == 0 && ! tree.hasReduction)
        {
            for (const auto& node : tree.nodes)
            {
                if (node.kind != SlpNodeKind::contiguousLoad && node.kind != SlpNodeKind::contiguousStore)
                    continue;

                const int sampleIdx = node.insts[0];
                const auto& sampleInst = blockInsts[static_cast<size_t> (sampleIdx)];

                if (sampleInst.op != YdspIrOp::loadStateArrayF && sampleInst.op != YdspIrOp::storeStateArrayF)
                    continue;

                const int mem = sampleInst.memIndex;

                for (int idx = 0; idx < ctx.numInsts; ++idx)
                {
                    if (tree.usedInsts.count (idx) != 0)
                        continue;

                    const auto& other = blockInsts[static_cast<size_t> (idx)];
                    if ((other.op == YdspIrOp::loadStateArrayF || other.op == YdspIrOp::storeStateArrayF)
                        && other.memIndex == mem)
                        return false;
                }
            }
        }

        return tree.hasContiguousMem || tree.hasReduction || vecOpCount >= 2;
    }

    const SlpBlockContext& ctx;
    const std::vector<bool>& consumed;
    SlpTree tree;
};

//==============================================================================

bool scheduleAndEmitTree (YdspIrFunction& fn,
                          const SlpFunctionInfo& fnInfo,
                          const SlpBlockContext& ctx,
                          const SlpTree& tree,
                          std::vector<bool>& consumed)
{
    const int N = ctx.numInsts;
    const int W = tree.lanes;
    const auto& blockInsts = fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;

    // Assign quotient node IDs.
    std::vector<int> instToQ (static_cast<size_t> (N), -1);

    struct QNode
    {
        enum class Kind
        {
            scalar,
            treeNode
        };

        Kind kind = Kind::scalar;
        int scalarInst = -1;
        int treeNodeIdx = -1;
        int priority = 0;
    };

    std::vector<QNode> qNodes;
    std::vector<int> nodeToQ (tree.nodes.size(), -1);
    std::map<std::pair<int, int>, int> splatKeyToQ;

    // Track absorbed movF rewrites and scatter store extracts.
    std::unordered_map<int, std::pair<int, int>> movRewrite; // instIdx -> (treeNodeIdx, lane)
    std::unordered_map<int, int> scatterStoreLane;           // storeInstIdx -> lane

    if (tree.isScatterStoreSeed)
    {
        for (int k = 0; k < W; ++k)
            scatterStoreLane[tree.scatterInsts[static_cast<size_t> (k)]] = k;
    }
    else if (tree.isScatterMovSeed)
    {
        for (int k = 0; k < W; ++k)
            movRewrite[tree.scatterInsts[static_cast<size_t> (k)]] = { tree.rootNode, k };
    }

    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const auto& node = tree.nodes[ni];

        if (node.kind == SlpNodeKind::vectorOp
            || node.kind == SlpNodeKind::contiguousLoad
            || node.kind == SlpNodeKind::contiguousStore
            || node.kind == SlpNodeKind::reduceAdd)
        {
            const int q = static_cast<int> (qNodes.size());
            int minIdx = N;

            for (const int idx : node.insts)
            {
                instToQ[static_cast<size_t> (idx)] = q;
                minIdx = std::min (minIdx, idx);
            }

            qNodes.push_back ({ QNode::Kind::treeNode, -1, static_cast<int> (ni), minIdx });
            nodeToQ[ni] = q;

            for (size_t k = 0; k < node.absorbedMoves.size(); ++k)
                if (node.absorbedMoves[k] >= 0)
                    movRewrite[node.absorbedMoves[k]] = { static_cast<int> (ni), static_cast<int> (k) };
        }
    }

    // Create quotient nodes for unmerged scalar instructions.
    for (int i = 0; i < N; ++i)
    {
        if (instToQ[static_cast<size_t> (i)] >= 0)
            continue;

        const int q = static_cast<int> (qNodes.size());
        instToQ[static_cast<size_t> (i)] = q;
        qNodes.push_back ({ QNode::Kind::scalar, i, -1, i });
    }

    // Create quotient nodes for non-hoisted splats and non-reused gathers.
    // Find minimum consumer priority for each child node.
    std::vector<int> childMinPriority (tree.nodes.size(), N);

    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const int parentPrio = nodeToQ[ni] >= 0 ? qNodes[static_cast<size_t> (nodeToQ[ni])].priority : N;

        for (const int ch : tree.nodes[ni].children)
            childMinPriority[static_cast<size_t> (ch)] = std::min (childMinPriority[static_cast<size_t> (ch)], parentPrio);
    }

    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const auto& node = tree.nodes[ni];

        if (node.kind == SlpNodeKind::gather && ! node.hoistGather)
        {
            const int q = static_cast<int> (qNodes.size());
            qNodes.push_back ({ QNode::Kind::treeNode, -1, static_cast<int> (ni), childMinPriority[ni] });
            nodeToQ[ni] = q;
        }
        else if (node.kind == SlpNodeKind::splat && ! node.hoistSplat)
        {
            bool alreadyInBlock = false;

            if (node.splatReachingDef < 0 && fnInfo.globalDefCount[static_cast<size_t> (node.splatScalar)] == 1)
            {
                for (const auto& [sv, vv] : ctx.existingSplats)
                {
                    if (sv == node.splatScalar)
                    {
                        alreadyInBlock = true;
                        break;
                    }
                }
            }

            if (alreadyInBlock)
                continue;

            const std::pair<int, int> key { node.splatScalar, node.splatReachingDef };

            if (const auto it = splatKeyToQ.find (key); it != splatKeyToQ.end())
            {
                nodeToQ[ni] = it->second;
            }
            else
            {
                const int q = static_cast<int> (qNodes.size());
                qNodes.push_back ({ QNode::Kind::treeNode, -1, static_cast<int> (ni), childMinPriority[ni] });
                nodeToQ[ni] = q;
                splatKeyToQ[key] = q;
            }
        }
    }

    const int numQ = static_cast<int> (qNodes.size());
    std::vector<std::vector<int>> qSuccs (static_cast<size_t> (numQ));
    std::vector<int> inDegree (static_cast<size_t> (numQ), 0);

    const auto addQEdge = [&] (int qu, int qv)
    {
        if (qu < 0 || qv < 0 || qu == qv)
            return;

        auto& s = qSuccs[static_cast<size_t> (qu)];

        if (std::find (s.begin(), s.end(), qv) == s.end())
        {
            s.push_back (qv);
            ++inDegree[static_cast<size_t> (qv)];
        }
    };

    // Original block DAG edges projected onto quotient nodes.
    for (int u = 0; u < N; ++u)
        for (const int v : ctx.directSuccs[static_cast<size_t> (u)])
            addQEdge (instToQ[static_cast<size_t> (u)], instToQ[static_cast<size_t> (v)]);

    // Edges for tree parent-child dependencies, gathers, splats, and reused vectors.
    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const auto& node = tree.nodes[ni];
        const int qParent = nodeToQ[ni];

        if (qParent >= 0)
        {
            for (const int ch : node.children)
            {
                if (nodeToQ[static_cast<size_t> (ch)] >= 0)
                    addQEdge (nodeToQ[static_cast<size_t> (ch)], qParent);
                else if (tree.nodes[static_cast<size_t> (ch)].kind == SlpNodeKind::reuseExisting
                         && tree.nodes[static_cast<size_t> (ch)].existingVectorDefInst >= 0)
                    addQEdge (instToQ[static_cast<size_t> (tree.nodes[static_cast<size_t> (ch)].existingVectorDefInst)], qParent);
            }
        }

        if (node.kind == SlpNodeKind::gather && qParent >= 0)
        {
            for (int k = 0; k < W; ++k)
            {
                const int def = node.gatherReachingDefs[static_cast<size_t> (k)];

                if (def >= 0)
                    addQEdge (instToQ[static_cast<size_t> (def)], qParent);
            }
        }
        else if (node.kind == SlpNodeKind::splat && ! node.hoistSplat && qParent >= 0)
        {
            if (node.splatReachingDef >= 0)
                addQEdge (instToQ[static_cast<size_t> (node.splatReachingDef)], qParent);
        }
    }

    // Add WAR edges for gathers/splats against any later redefinitions of their scalar inputs.
    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const auto& node = tree.nodes[ni];

        if ((node.kind != SlpNodeKind::gather || node.hoistGather)
            && (node.kind != SlpNodeKind::splat || node.hoistSplat))
            continue;

        const int qLeaf = nodeToQ[ni];

        if (qLeaf < 0)
            continue;

        // Inherit WAR edges for any consumer instruction that read the scalar before a later redefinition.
        for (size_t pi = 0; pi < tree.nodes.size(); ++pi)
        {
            const auto& parent = tree.nodes[pi];

            if (std::find (parent.children.begin(), parent.children.end(), static_cast<int> (ni)) == parent.children.end())
                continue;

            for (const int u : parent.insts)
            {
                for (int slot = 0; slot < 3; ++slot)
                {
                    if (! isValueIdOperand (blockInsts[static_cast<size_t> (u)].op, slot))
                        continue;

                    const int nd = ctx.nextDef[static_cast<size_t> (u)][static_cast<size_t> (slot)];

                    if (nd >= 0)
                    {
                        const int val = getOperand (blockInsts[static_cast<size_t> (u)], slot);

                        if (node.kind == SlpNodeKind::splat && val == node.splatScalar)
                            addQEdge (qLeaf, instToQ[static_cast<size_t> (nd)]);
                        else if (node.kind == SlpNodeKind::gather)
                            for (const int s : node.gatherScalars)
                                if (s == val)
                                    addQEdge (qLeaf, instToQ[static_cast<size_t> (nd)]);
                    }
                }
            }
        }
    }

    // Kahn's topological sort with min-heap priority = earliest original block position.
    std::vector<int> ready;

    for (int q = 0; q < numQ; ++q)
        if (inDegree[static_cast<size_t> (q)] == 0)
            ready.push_back (q);

    const auto heapCmp = [&qNodes] (int a, int b) noexcept
    {
        if (qNodes[static_cast<size_t> (a)].priority != qNodes[static_cast<size_t> (b)].priority)
            return qNodes[static_cast<size_t> (a)].priority > qNodes[static_cast<size_t> (b)].priority;

        return a > b;
    };

    std::make_heap (ready.begin(), ready.end(), heapCmp);

    std::vector<int> schedule;
    schedule.reserve (static_cast<size_t> (numQ));

    while (! ready.empty())
    {
        std::pop_heap (ready.begin(), ready.end(), heapCmp);
        const int q = ready.back();
        ready.pop_back();

        schedule.push_back (q);

        for (const int succ : qSuccs[static_cast<size_t> (q)])
        {
            if (--inDegree[static_cast<size_t> (succ)] == 0)
            {
                ready.push_back (succ);
                std::push_heap (ready.begin(), ready.end(), heapCmp);
            }
        }
    }

    if (static_cast<int> (schedule.size()) != numQ)
        return false; // Quotient cycle: abort safely without modifying IR.

    // Commit: allocate vector IDs and emit rewritten block.
    if (fn.valueLanes.size() < fn.valueTypes.size())
        fn.valueLanes.resize (fn.valueTypes.size(), 1);

    const auto newVal = [&fn] (YdspValueType type, int laneCount) -> int
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);
        fn.valueLanes.push_back (laneCount);
        return id;
    };

    std::vector<int> nodeVecId (tree.nodes.size(), -1);
    std::map<std::pair<int, int>, int> localSplatCache;
    std::unordered_map<int, int> hoistedSplatCache = ctx.preheaderSplats;

    for (size_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        const auto& node = tree.nodes[ni];

        if (node.kind == SlpNodeKind::reuseExisting)
        {
            nodeVecId[ni] = node.existingVectorId;
            continue;
        }

        if (node.kind == SlpNodeKind::splat)
        {
            if (node.hoistSplat)
            {
                auto it = hoistedSplatCache.find (node.splatScalar);

                if (it == hoistedSplatCache.end())
                {
                    const int vec = newVal (YdspValueType::float32Type, W);
                    YdspIrInst splatInst;
                    splatInst.op = YdspIrOp::vsplat;
                    splatInst.result = vec;
                    splatInst.a = node.splatScalar;
                    fn.blocks[static_cast<size_t> (tree.preheaderBlock)].insts.push_back (splatInst);
                    hoistedSplatCache[node.splatScalar] = vec;
                    nodeVecId[ni] = vec;
                }
                else
                {
                    nodeVecId[ni] = it->second;
                }

                continue;
            }

            if (node.splatReachingDef < 0 && fnInfo.globalDefCount[static_cast<size_t> (node.splatScalar)] == 1)
            {
                for (const auto& [sv, vv] : ctx.existingSplats)
                {
                    if (sv == node.splatScalar)
                    {
                        nodeVecId[ni] = vv;
                        break;
                    }
                }

                if (nodeVecId[ni] >= 0)
                    continue;
            }

            const std::pair<int, int> key { node.splatScalar, node.splatReachingDef };
            auto it = localSplatCache.find (key);

            if (it == localSplatCache.end())
            {
                const int vec = newVal (YdspValueType::float32Type, W);
                localSplatCache[key] = vec;
                nodeVecId[ni] = vec;
            }
            else
            {
                nodeVecId[ni] = it->second;
            }

            continue;
        }

        if (node.kind == SlpNodeKind::gather && node.hoistGather)
        {
            int prevVec = -1;
            const int finalVec = newVal (YdspValueType::float32Type, W);

            for (int k = 0; k < W; ++k)
            {
                const int stepVec = (k == W - 1) ? finalVec : newVal (YdspValueType::float32Type, W);
                YdspIrInst ins;
                ins.op = YdspIrOp::vinsertF;
                ins.result = stepVec;
                ins.a = prevVec;
                ins.b = node.gatherScalars[static_cast<size_t> (k)];
                ins.memIndex = k;
                fn.blocks[static_cast<size_t> (tree.preheaderBlock)].insts.push_back (ins);
                prevVec = stepVec;
            }

            nodeVecId[ni] = finalVec;
            continue;
        }

        if (node.kind == SlpNodeKind::gather
            || node.kind == SlpNodeKind::contiguousLoad
            || node.kind == SlpNodeKind::vectorOp)
        {
            nodeVecId[ni] = newVal (node.resultType, W);
        }
    }

    std::unordered_set<int> emittedSplatQ;
    std::vector<YdspIrInst> newInsts;
    newInsts.reserve (blockInsts.size() + 16);

    for (const int q : schedule)
    {
        const auto& qNode = qNodes[static_cast<size_t> (q)];

        if (qNode.kind == QNode::Kind::scalar)
        {
            const int i = qNode.scalarInst;
            auto inst = blockInsts[static_cast<size_t> (i)];

            if (const auto movIt = movRewrite.find (i); movIt != movRewrite.end())
            {
                YdspIrInst ext;
                ext.op = YdspIrOp::vextractF;
                ext.result = inst.result;
                ext.a = nodeVecId[static_cast<size_t> (movIt->second.first)];
                ext.memIndex = movIt->second.second;
                newInsts.push_back (ext);
                consumed[static_cast<size_t> (i)] = true;
                continue;
            }

            if (const auto stIt = scatterStoreLane.find (i); stIt != scatterStoreLane.end())
            {
                const int lane = stIt->second;
                const int tmp = newVal (YdspValueType::float32Type, 1);

                YdspIrInst ext;
                ext.op = YdspIrOp::vextractF;
                ext.result = tmp;
                ext.a = nodeVecId[static_cast<size_t> (tree.rootNode)];
                ext.memIndex = lane;
                newInsts.push_back (ext);

                if (inst.op == YdspIrOp::storeStateF)
                    inst.a = tmp;
                else
                    inst.b = tmp;

                newInsts.push_back (inst);
                consumed[static_cast<size_t> (i)] = true;
                continue;
            }

            newInsts.push_back (inst);
            continue;
        }

        const int ni = qNode.treeNodeIdx;
        const auto& node = tree.nodes[static_cast<size_t> (ni)];

        switch (node.kind)
        {
            case SlpNodeKind::splat:
            {
                if (emittedSplatQ.insert (q).second)
                {
                    YdspIrInst splatInst;
                    splatInst.op = YdspIrOp::vsplat;
                    splatInst.result = nodeVecId[static_cast<size_t> (ni)];
                    splatInst.a = node.splatScalar;
                    newInsts.push_back (splatInst);
                }
                break;
            }

            case SlpNodeKind::gather:
            {
                int prevVec = -1;

                for (int k = 0; k < W; ++k)
                {
                    const int stepVec = (k == W - 1)
                                          ? nodeVecId[static_cast<size_t> (ni)]
                                          : newVal (YdspValueType::float32Type, W);

                    YdspIrInst ins;
                    ins.op = YdspIrOp::vinsertF;
                    ins.result = stepVec;
                    ins.a = prevVec;
                    ins.b = node.gatherScalars[static_cast<size_t> (k)];
                    ins.memIndex = k;
                    newInsts.push_back (ins);
                    prevVec = stepVec;
                }
                break;
            }

            case SlpNodeKind::contiguousLoad:
            {
                const auto& inst0 = blockInsts[static_cast<size_t> (node.insts[0])];
                YdspIrInst vecLoad = inst0;
                vecLoad.result = nodeVecId[static_cast<size_t> (ni)];
                newInsts.push_back (vecLoad);

                for (int k = 0; k < W; ++k)
                {
                    if (node.needsExtract[static_cast<size_t> (k)])
                    {
                        YdspIrInst ext;
                        ext.op = YdspIrOp::vextractF;
                        ext.result = blockInsts[static_cast<size_t> (node.insts[static_cast<size_t> (k)])].result;
                        ext.a = vecLoad.result;
                        ext.memIndex = k;
                        newInsts.push_back (ext);
                    }

                    consumed[static_cast<size_t> (node.insts[static_cast<size_t> (k)])] = true;
                }
                break;
            }

            case SlpNodeKind::vectorOp:
            {
                const auto& inst0 = blockInsts[static_cast<size_t> (node.insts[0])];
                YdspIrInst vecInst;
                vecInst.op = node.op;
                vecInst.result = nodeVecId[static_cast<size_t> (ni)];

                if (node.children.size() > 0)
                    vecInst.a = nodeVecId[static_cast<size_t> (node.children[0])];
                if (node.children.size() > 1)
                    vecInst.b = nodeVecId[static_cast<size_t> (node.children[1])];
                if (node.children.size() > 2)
                    vecInst.c = nodeVecId[static_cast<size_t> (node.children[2])];

                vecInst.saturatingIntegerArithmetic = inst0.saturatingIntegerArithmetic;
                vecInst.overflowFreeIntegerArithmetic = inst0.overflowFreeIntegerArithmetic;
                newInsts.push_back (vecInst);

                for (int k = 0; k < W; ++k)
                {
                    if (node.needsExtract[static_cast<size_t> (k)])
                    {
                        YdspIrInst ext;
                        ext.op = YdspIrOp::vextractF;
                        ext.result = blockInsts[static_cast<size_t> (node.insts[static_cast<size_t> (k)])].result;
                        ext.a = vecInst.result;
                        ext.memIndex = k;
                        newInsts.push_back (ext);
                    }

                    consumed[static_cast<size_t> (node.insts[static_cast<size_t> (k)])] = true;
                }
                break;
            }

            case SlpNodeKind::contiguousStore:
            {
                const auto& inst0 = blockInsts[static_cast<size_t> (node.insts[0])];
                YdspIrInst vecStore = inst0;
                vecStore.result = -1;

                if (vecStore.op == YdspIrOp::storeStateF)
                    vecStore.a = nodeVecId[static_cast<size_t> (node.children[0])];
                else
                    vecStore.b = nodeVecId[static_cast<size_t> (node.children[0])];

                newInsts.push_back (vecStore);

                for (const int idx : node.insts)
                    consumed[static_cast<size_t> (idx)] = true;
                break;
            }

            case SlpNodeKind::reduceAdd:
            {
                const int vecIn = nodeVecId[static_cast<size_t> (node.children[0])];
                const int rootRes = blockInsts[static_cast<size_t> (node.reduceRootInst)].result;

                if (node.remainingReduceLeaves.empty())
                {
                    YdspIrInst red;
                    red.op = YdspIrOp::vreduceAddF;
                    red.result = rootRes;
                    red.a = vecIn;
                    newInsts.push_back (red);
                }
                else
                {
                    int acc = newVal (YdspValueType::float32Type, 1);

                    YdspIrInst red;
                    red.op = YdspIrOp::vreduceAddF;
                    red.result = acc;
                    red.a = vecIn;
                    newInsts.push_back (red);

                    for (size_t i = 0; i < node.remainingReduceLeaves.size(); ++i)
                    {
                        const bool isLast = (i + 1 == node.remainingReduceLeaves.size());
                        const int nextAcc = isLast ? rootRes : newVal (YdspValueType::float32Type, 1);

                        YdspIrInst addInst;
                        addInst.op = YdspIrOp::addF;
                        addInst.result = nextAcc;
                        addInst.a = acc;
                        addInst.b = node.remainingReduceLeaves[i].first;
                        newInsts.push_back (addInst);
                        acc = nextAcc;
                    }
                }

                for (const int idx : node.insts)
                    consumed[static_cast<size_t> (idx)] = true;
                break;
            }

            case SlpNodeKind::reuseExisting:
                break;
        }
    }

    fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts = std::move (newInsts);
    fn.vectorized = true;
    fn.vectorWidth = std::max (fn.vectorWidth, W);
    return true;
}

//==============================================================================

std::vector<std::pair<int, int>> buildStructuralBuckets (const SlpBlockContext& ctx,
                                                         const std::vector<int>& candidates,
                                                         bool isMoveSeed)
{
    const auto& insts = ctx.fn.blocks[static_cast<size_t> (ctx.blockIndex)].insts;

    std::function<uint64_t (int, int, int)> signatureOf = [&] (int instIdx, int slot, int depth) -> uint64_t
    {
        const int val = getOperand (insts[static_cast<size_t> (instIdx)], slot);
        const int rDef = ctx.reachingDef[static_cast<size_t> (instIdx)][static_cast<size_t> (slot)];
        const auto [uVal, uDef] = ctx.unwrapCopy (val, rDef, instIdx);

        if (uDef < 0 || depth <= 0)
            return uint64_t (0x9e3779b97f4a7c15ull);

        const auto& defInst = insts[static_cast<size_t> (uDef)];
        uint64_t h = static_cast<uint64_t> (defInst.op) + 1;
        const int nOps = slpValueOperandCount (defInst.op);

        if (nOps == 2 && isSlpCommutativeOp (defInst.op))
        {
            const uint64_t h0 = signatureOf (uDef, 0, depth - 1);
            const uint64_t h1 = signatureOf (uDef, 1, depth - 1);
            h = (h * 1315423911ull) ^ std::min (h0, h1);
            h = (h * 1315423911ull) ^ std::max (h0, h1);
        }
        else
        {
            for (int s = 0; s < nOps; ++s)
                h = (h * 1315423911ull) ^ signatureOf (uDef, s, depth - 1);
        }

        return h;
    };

    std::vector<std::pair<int, int>> result;
    result.reserve (candidates.size());

    for (const int idx : candidates)
    {
        const auto& inst = insts[static_cast<size_t> (idx)];
        const int valSlot = isMoveSeed ? 0 : (inst.op == YdspIrOp::storeStateF ? 0 : 1);
        const uint64_t sig = signatureOf (idx, valSlot, 3);
        result.emplace_back (idx, static_cast<int> (sig ^ (sig >> 32)));
    }

    return result;
}

bool tryVectorizeBlockAtWidth (YdspIrFunction& fn,
                               SlpFunctionInfo& fnInfo,
                               int blockIndex,
                               int lanes,
                               bool dryRunFirstOnly)
{
    bool changed = false;

    for (;;)
    {
        const auto& block = fn.blocks[static_cast<size_t> (blockIndex)];
        const int N = static_cast<int> (block.insts.size());

        if (N < lanes)
            break;

        // Skip blocks containing phi nodes.
        if (std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst) { return inst.op == YdspIrOp::phi; }))
            break;

        fnInfo.rebuild (fn);
        SlpBlockContext ctx (fn, fnInfo, blockIndex, lanes);
        std::vector<bool> consumed (static_cast<size_t> (N), false);

        for (int i = 0; i < N; ++i)
            if (block.insts[static_cast<size_t> (i)].result >= 0
                && fn.laneCountOf (block.insts[static_cast<size_t> (i)].result) > 1)
                consumed[static_cast<size_t> (i)] = true;

        bool stepChanged = false;

        // 1. Contiguous stores (storeStateArrayF, storeOutput).
        for (const auto targetOp : { YdspIrOp::storeStateArrayF, YdspIrOp::storeOutput })
        {
            if (stepChanged)
                break;

            struct StoreGroupKey
            {
                int memIndex = -1;
                int baseReg = -1;
                int baseDef = -1;

                bool operator< (const StoreGroupKey& other) const noexcept
                {
                    return std::tie (memIndex, baseReg, baseDef) < std::tie (other.memIndex, other.baseReg, other.baseDef);
                }
            };

            std::map<StoreGroupKey, std::vector<std::pair<int64_t, int>>> groups;

            for (int i = 0; i < N; ++i)
            {
                if (consumed[static_cast<size_t> (i)])
                    continue;

                const auto& inst = block.insts[static_cast<size_t> (i)];

                if (inst.op != targetOp || inst.b < 0 || inst.b >= ctx.numValues
                    || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type
                    || fn.laneCountOf (inst.b) != 1)
                    continue;

                const auto& addr = ctx.memAddrs[static_cast<size_t> (i)];

                if (targetOp == YdspIrOp::storeStateArrayF && static_cast<int64_t> (addr.memIndex) + addr.offset < 0)
                    continue;

                groups[{ addr.memIndex, addr.baseReg, addr.baseDef }].emplace_back (addr.offset, i);
            }

            for (auto& [key, entries] : groups)
            {
                (void) key;

                if (static_cast<int> (entries.size()) < lanes)
                    continue;

                std::sort (entries.begin(), entries.end());

                for (size_t start = 0; start + static_cast<size_t> (lanes) <= entries.size(); ++start)
                {
                    bool contiguous = true;
                    std::vector<int> window (static_cast<size_t> (lanes));
                    window[0] = entries[start].second;

                    for (int k = 1; k < lanes; ++k)
                    {
                        if (entries[start + static_cast<size_t> (k)].first != entries[start].first + k)
                        {
                            contiguous = false;
                            break;
                        }

                        window[static_cast<size_t> (k)] = entries[start + static_cast<size_t> (k)].second;
                    }

                    if (! contiguous || ! ctx.areMutuallyIndependent (window))
                        continue;

                    SlpTreeBuilder builder (ctx, consumed);

                    if (builder.buildForContiguousStores (window))
                    {
                        if (dryRunFirstOnly)
                            return true;

                        if (scheduleAndEmitTree (fn, fnInfo, ctx, builder.getTree(), consumed))
                        {
                            stepChanged = true;
                            break;
                        }
                    }
                }

                if (stepChanged)
                    break;
            }
        }

        // 2. Non-contiguous parallel store seeds (storeStateArrayF, storeOutput, storeStateF).
        if (! stepChanged)
        {
            for (const auto targetOp : { YdspIrOp::storeStateArrayF, YdspIrOp::storeOutput, YdspIrOp::storeStateF })
            {
                if (stepChanged)
                    break;

                std::vector<int> candidates;

                for (int i = 0; i < N; ++i)
                {
                    if (consumed[static_cast<size_t> (i)])
                        continue;

                    const auto& inst = block.insts[static_cast<size_t> (i)];

                    if (inst.op != targetOp)
                        continue;

                    const int valSlot = targetOp == YdspIrOp::storeStateF ? 0 : 1;
                    const int val = getOperand (inst, valSlot);
                    const int rDef = ctx.reachingDef[static_cast<size_t> (i)][static_cast<size_t> (valSlot)];
                    const auto [uVal, uDef] = ctx.unwrapCopy (val, rDef, i);

                    if (uVal < 0 || uVal >= ctx.numValues
                        || fn.valueTypes[static_cast<size_t> (uVal)] != YdspValueType::float32Type
                        || fn.laneCountOf (uVal) != 1
                        || uDef < 0)
                        continue;

                    candidates.push_back (i);
                }

                if (static_cast<int> (candidates.size()) < lanes)
                    continue;

                const auto tagged = buildStructuralBuckets (ctx, candidates, false);
                std::map<int, std::vector<int>> buckets;

                for (const auto& [idx, sig] : tagged)
                    buckets[sig].push_back (idx);

                for (const auto& [sig, bucket] : buckets)
                {
                    (void) sig;

                    if (static_cast<int> (bucket.size()) < lanes)
                        continue;

                    for (size_t start = 0; start + static_cast<size_t> (lanes) <= bucket.size(); ++start)
                    {
                        std::vector<int> window (bucket.begin() + static_cast<std::ptrdiff_t> (start),
                                                 bucket.begin() + static_cast<std::ptrdiff_t> (start + static_cast<size_t> (lanes)));

                        if (! ctx.areMutuallyIndependent (window))
                            continue;

                        SlpTreeBuilder builder (ctx, consumed);

                        if (builder.buildForScatterStores (window))
                        {
                            if (dryRunFirstOnly)
                                return true;

                            if (scheduleAndEmitTree (fn, fnInfo, ctx, builder.getTree(), consumed))
                            {
                                stepChanged = true;
                                break;
                            }
                        }
                    }

                    if (stepChanged)
                        break;
                }
            }
        }

        // 3. Horizontal addF reduction trees.
        if (! stepChanged)
        {
            std::vector<bool> usedInParentAdd (static_cast<size_t> (N), false);

            for (int i = 0; i < N; ++i)
            {
                const auto& inst = block.insts[static_cast<size_t> (i)];

                if (inst.op != YdspIrOp::addF || consumed[static_cast<size_t> (i)]
                    || inst.result < 0 || fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type
                    || fn.laneCountOf (inst.result) != 1)
                    continue;

                for (int slot = 0; slot < 2; ++slot)
                {
                    const int d = ctx.reachingDef[static_cast<size_t> (i)][static_cast<size_t> (slot)];

                    if (d >= 0 && block.insts[static_cast<size_t> (d)].op == YdspIrOp::addF
                        && ! consumed[static_cast<size_t> (d)]
                        && ctx.resultUsesInBlock[static_cast<size_t> (d)].size() == 1
                        && ! ctx.isLiveOutAfter (d))
                        usedInParentAdd[static_cast<size_t> (d)] = true;
                }
            }

            for (int i = 0; i < N && ! stepChanged; ++i)
            {
                const auto& rootInst = block.insts[static_cast<size_t> (i)];

                if (rootInst.op != YdspIrOp::addF || consumed[static_cast<size_t> (i)]
                    || usedInParentAdd[static_cast<size_t> (i)]
                    || rootInst.result < 0 || fn.valueTypes[static_cast<size_t> (rootInst.result)] != YdspValueType::float32Type
                    || fn.laneCountOf (rootInst.result) != 1)
                    continue;

                std::vector<std::pair<int, int>> leaves;
                std::vector<int> internalAdds;

                std::function<void (int, int, int)> collectLeaves = [&] (int val, int rDef, int consumerInst)
                {
                    if (rDef >= 0 && ! consumed[static_cast<size_t> (rDef)])
                    {
                        const auto& defInst = block.insts[static_cast<size_t> (rDef)];

                        if (defInst.op == YdspIrOp::addF
                            && defInst.result >= 0
                            && fn.valueTypes[static_cast<size_t> (defInst.result)] == YdspValueType::float32Type
                            && fn.laneCountOf (defInst.result) == 1
                            && (rDef == i || (ctx.resultUsesInBlock[static_cast<size_t> (rDef)].size() == 1 && ! ctx.isLiveOutAfter (rDef))))
                        {
                            const int n0 = ctx.nextDef[static_cast<size_t> (rDef)][0];
                            const int n1 = ctx.nextDef[static_cast<size_t> (rDef)][1];

                            if ((n0 < 0 || n0 > consumerInst) && (n1 < 0 || n1 > consumerInst)
                                && std::find (internalAdds.begin(), internalAdds.end(), rDef) == internalAdds.end())
                            {
                                internalAdds.push_back (rDef);
                                collectLeaves (defInst.a, ctx.reachingDef[static_cast<size_t> (rDef)][0], consumerInst);
                                collectLeaves (defInst.b, ctx.reachingDef[static_cast<size_t> (rDef)][1], consumerInst);
                                return;
                            }
                        }
                    }

                    leaves.emplace_back (val, rDef);
                };

                collectLeaves (rootInst.result, i, i);

                if (static_cast<int> (leaves.size()) < lanes || static_cast<int> (internalAdds.size()) != static_cast<int> (leaves.size()) - 1)
                    continue;

                for (size_t start = 0; start + static_cast<size_t> (lanes) <= leaves.size(); ++start)
                {
                    std::vector<std::pair<int, int>> window (leaves.begin() + static_cast<std::ptrdiff_t> (start),
                                                             leaves.begin() + static_cast<std::ptrdiff_t> (start + static_cast<size_t> (lanes)));
                    std::vector<std::pair<int, int>> remaining;

                    for (size_t idx = 0; idx < leaves.size(); ++idx)
                        if (idx < start || idx >= start + static_cast<size_t> (lanes))
                            remaining.push_back (leaves[idx]);

                    SlpTreeBuilder builder (ctx, consumed);

                    if (builder.buildForReduction (i, internalAdds, window, remaining))
                    {
                        if (dryRunFirstOnly)
                            return true;

                        if (scheduleAndEmitTree (fn, fnInfo, ctx, builder.getTree(), consumed))
                        {
                            stepChanged = true;
                            break;
                        }
                    }
                }
            }
        }

        // 4. Parallel movF state write-back seeds.
        if (! stepChanged)
        {
            std::vector<int> candidates;

            for (int i = 0; i < N; ++i)
            {
                if (consumed[static_cast<size_t> (i)])
                    continue;

                const auto& inst = block.insts[static_cast<size_t> (i)];

                if (inst.op != YdspIrOp::movF || inst.result < 0 || inst.a < 0)
                    continue;

                const int rDef = ctx.reachingDef[static_cast<size_t> (i)][0];
                const auto [uVal, uDef] = ctx.unwrapCopy (inst.a, rDef, i);

                if (uVal < 0 || uVal >= ctx.numValues
                    || fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type
                    || fn.laneCountOf (inst.result) != 1
                    || fn.valueTypes[static_cast<size_t> (uVal)] != YdspValueType::float32Type
                    || fn.laneCountOf (uVal) != 1
                    || uDef < 0)
                    continue;

                candidates.push_back (i);
            }

            if (static_cast<int> (candidates.size()) >= lanes)
            {
                const auto tagged = buildStructuralBuckets (ctx, candidates, true);
                std::map<int, std::vector<int>> buckets;

                for (const auto& [idx, sig] : tagged)
                    buckets[sig].push_back (idx);

                for (const auto& [sig, bucket] : buckets)
                {
                    (void) sig;

                    if (static_cast<int> (bucket.size()) < lanes)
                        continue;

                    for (size_t start = 0; start + static_cast<size_t> (lanes) <= bucket.size(); ++start)
                    {
                        std::vector<int> window (bucket.begin() + static_cast<std::ptrdiff_t> (start),
                                                 bucket.begin() + static_cast<std::ptrdiff_t> (start + static_cast<size_t> (lanes)));

                        // Distinct destination registers required.
                        bool distinctDst = true;

                        for (int a = 0; a < lanes && distinctDst; ++a)
                            for (int b = a + 1; b < lanes; ++b)
                                if (block.insts[static_cast<size_t> (window[static_cast<size_t> (a)])].result
                                    == block.insts[static_cast<size_t> (window[static_cast<size_t> (b)])].result)
                                    distinctDst = false;

                        if (! distinctDst || ! ctx.areMutuallyIndependent (window))
                            continue;

                        SlpTreeBuilder builder (ctx, consumed);

                        if (builder.buildForScatterMoves (window))
                        {
                            if (dryRunFirstOnly)
                                return true;

                            if (scheduleAndEmitTree (fn, fnInfo, ctx, builder.getTree(), consumed))
                            {
                                stepChanged = true;
                                break;
                            }
                        }
                    }

                    if (stepChanged)
                        break;
                }
            }
        }

        if (! stepChanged)
            break;

        changed = true;
    }

    return changed;
}

void pruneDeadVectorExtracts (YdspIrFunction& fn)
{
    bool changed = true;

    while (changed)
    {
        changed = false;
        const YdspValueUseCounts counts (fn);

        for (auto& block : fn.blocks)
        {
            const auto& termCond = block.term == YdspIrTerm::branchIf ? block.termCond : -1;

            const auto oldSize = block.insts.size();
            block.insts.erase (std::remove_if (block.insts.begin(), block.insts.end(), [&] (const YdspIrInst& inst)
            {
                if (inst.op != YdspIrOp::vextractF || inst.result < 0)
                    return false;

                return counts.definedOnce (inst.result)
                    && counts.useCount (inst.result) == 0
                    && inst.result != termCond;
            }),
                               block.insts.end());

            if (block.insts.size() != oldSize)
                changed = true;
        }
    }
}

} // namespace

//==============================================================================

bool YdspOptimizer::slpVectorize (YdspIrFunction& fn)
{
    if (vectorWidth < 4 || fn.blocks.empty())
        return false;

    int targetLanes = 0;

    if (fn.vectorized)
    {
        if (fn.vectorWidth != 4 && fn.vectorWidth != 8)
            return false;

        targetLanes = fn.vectorWidth;
    }

    const auto types = resolveValueTypes (fn);

    if (fn.valueTypes.size() < types.size())
        fn.valueTypes = types;

    detail::mergeStraightLineBlocks (fn);

    SlpFunctionInfo fnInfo (fn);

    if (targetLanes == 0)
    {
        if (vectorWidth >= 8)
        {
            bool hasEightLaneTree = false;

            for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
            {
                if (tryVectorizeBlockAtWidth (fn, fnInfo, b, 8, true))
                {
                    hasEightLaneTree = true;
                    break;
                }
            }

            targetLanes = hasEightLaneTree ? 8 : 4;
        }
        else
        {
            targetLanes = 4;
        }
    }

    bool changed = false;

    for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
        changed |= tryVectorizeBlockAtWidth (fn, fnInfo, b, targetLanes, false);

    if (changed)
    {
        pruneDeadVectorExtracts (fn);
        deadCodeElimination (fn);
    }

    return changed;
}

} // namespace yup
