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

int scheduleLatencyOf (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::constI:
        case YdspIrOp::constB:
        case YdspIrOp::constF:
        case YdspIrOp::movF:
        case YdspIrOp::movI:
        case YdspIrOp::movB:
            return 0;

        case YdspIrOp::mulF:
        case YdspIrOp::mulI:
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadEventFieldI:
            return 3;

        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::vreduceAddF:
            return 4;

        case YdspIrOp::divF:
        case YdspIrOp::sqrtF:
            return 10;

        case YdspIrOp::modF:
        case YdspIrOp::divI:
        case YdspIrOp::modI:
            return 12;

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
        case YdspIrOp::fmodF:
        case YdspIrOp::atan2F:
        case YdspIrOp::roundF:
        case YdspIrOp::copysignF:
            return 20;

        default:
            return 2;
    }
}

bool isScheduleBarrierOp (YdspIrOp op) noexcept
{
    if (isCallBoundary (op))
        return true;

    switch (op)
    {
        case YdspIrOp::phi:
        case YdspIrOp::expF:
        case YdspIrOp::traceInt:
        case YdspIrOp::traceFloat:
        case YdspIrOp::traceCommit:
        case YdspIrOp::emitEvent:
            return true;

        default:
            return false;
    }
}

enum class ScheduleMemFamily
{
    none,
    stream,
    stateF,
    stateI,
    stateArrayF,
    stateArrayI,
    param,
    paramOut,
    eventField
};

struct ScheduleMemInfo
{
    ScheduleMemFamily family = ScheduleMemFamily::none;
    bool isWrite = false;
    bool is64Bit = false;
    bool hasIndexedOffset = false;
    int slotOrBase = -1;
    int accessValueId = -1;
};

YdspValueType valueTypeOrFallback (const std::vector<YdspValueType>& valueTypes, int valueId, YdspIrOp op) noexcept
{
    if (valueId >= 0 && static_cast<size_t> (valueId) < valueTypes.size())
        return valueTypes[static_cast<size_t> (valueId)];

    return inferTypeFromOp (op);
}

ScheduleMemInfo classifyScheduleMemory (const YdspIrInst& inst, const std::vector<YdspValueType>& valueTypes) noexcept
{
    ScheduleMemInfo info;

    switch (inst.op)
    {
        case YdspIrOp::loadInput:
            info.family = ScheduleMemFamily::stream;
            info.isWrite = false;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::loadOutput:
            info.family = ScheduleMemFamily::stream;
            info.isWrite = false;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::storeOutput:
            info.family = ScheduleMemFamily::stream;
            info.isWrite = true;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.b;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.b, inst.op));
            break;

        case YdspIrOp::loadStateF:
            info.family = ScheduleMemFamily::stateF;
            info.isWrite = false;
            info.slotOrBase = inst.a;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::storeStateF:
            info.family = ScheduleMemFamily::stateF;
            info.isWrite = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.a;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.a, inst.op));
            break;

        case YdspIrOp::loadStateI:
            info.family = ScheduleMemFamily::stateI;
            info.isWrite = false;
            info.slotOrBase = inst.a;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::storeStateI:
            info.family = ScheduleMemFamily::stateI;
            info.isWrite = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.a;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.a, inst.op));
            break;

        case YdspIrOp::loadStateArrayF:
            info.family = ScheduleMemFamily::stateArrayF;
            info.isWrite = false;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::storeStateArrayF:
            info.family = ScheduleMemFamily::stateArrayF;
            info.isWrite = true;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.b;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.b, inst.op));
            break;

        case YdspIrOp::loadStateArrayI:
            info.family = ScheduleMemFamily::stateArrayI;
            info.isWrite = false;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.result;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.result, inst.op));
            break;

        case YdspIrOp::storeStateArrayI:
            info.family = ScheduleMemFamily::stateArrayI;
            info.isWrite = true;
            info.hasIndexedOffset = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.b;
            info.is64Bit = is64BitValueType (valueTypeOrFallback (valueTypes, inst.b, inst.op));
            break;

        case YdspIrOp::loadParam:
            info.family = ScheduleMemFamily::param;
            info.isWrite = false;
            info.slotOrBase = inst.a;
            info.accessValueId = inst.result;
            break;

        case YdspIrOp::storeParam:
            info.family = ScheduleMemFamily::param;
            info.isWrite = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.a;
            break;

        case YdspIrOp::loadParamOut:
            info.family = ScheduleMemFamily::paramOut;
            info.isWrite = false;
            info.slotOrBase = inst.a;
            info.accessValueId = inst.result;
            break;

        case YdspIrOp::storeParamOut:
            info.family = ScheduleMemFamily::paramOut;
            info.isWrite = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.a;
            break;

        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadEventFieldI:
            info.family = ScheduleMemFamily::eventField;
            info.isWrite = false;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.result;
            break;

        case YdspIrOp::storeEventFieldF:
        case YdspIrOp::storeEventFieldI:
            info.family = ScheduleMemFamily::eventField;
            info.isWrite = true;
            info.slotOrBase = inst.memIndex;
            info.accessValueId = inst.a;
            break;

        default:
            break;
    }

    return info;
}

bool memoryOpsConflict (const YdspIrFunction& fn,
                        const YdspIrInst& firstInst,
                        const ScheduleMemInfo& first,
                        std::optional<int64_t> firstConstIndex,
                        const YdspIrInst& secondInst,
                        const ScheduleMemInfo& second,
                        std::optional<int64_t> secondConstIndex) noexcept
{
    if (first.family == ScheduleMemFamily::none || first.family != second.family)
        return false;

    if (! first.isWrite && ! second.isWrite)
        return false;

    if (first.slotOrBase != second.slotOrBase)
        return false;

    if (first.family == ScheduleMemFamily::stateF || first.family == ScheduleMemFamily::stateI
        || first.family == ScheduleMemFamily::stateArrayF || first.family == ScheduleMemFamily::stateArrayI)
    {
        if (first.is64Bit != second.is64Bit)
            return false;
    }

    if (first.family == ScheduleMemFamily::stream)
    {
        // loadInput vs storeOutput only aliases when the same channel index is
        // passed in-place; loadOutput vs storeOutput aliases on the same stream.
        const bool firstIsInput = firstInst.op == YdspIrOp::loadInput;
        const bool secondIsInput = secondInst.op == YdspIrOp::loadInput;

        if (firstIsInput && secondIsInput)
            return false;
    }

    if (first.hasIndexedOffset && second.hasIndexedOffset
        && firstConstIndex.has_value() && secondConstIndex.has_value())
    {
        const int64_t firstLanes = fn.laneCountOf (first.accessValueId);
        const int64_t secondLanes = fn.laneCountOf (second.accessValueId);

        const int64_t firstStart = *firstConstIndex;
        const int64_t secondStart = *secondConstIndex;

        const bool disjoint = (firstStart + firstLanes <= secondStart)
                           || (secondStart + secondLanes <= firstStart);
        return ! disjoint;
    }

    return true;
}

bool isFpRegisterClass (const YdspIrFunction& fn,
                        const std::vector<YdspValueType>& valueTypes,
                        int valueId,
                        YdspIrOp fallbackOp) noexcept
{
    if (fn.laneCountOf (valueId) > 1)
        return true;

    return isFloatValueType (valueTypeOrFallback (valueTypes, valueId, fallbackOp));
}

struct ScheduleEdge
{
    int to = -1;
    int latency = 0;
    bool isRaw = false;
};

struct ScheduleWorkspace
{
    std::vector<int> lastDef;
    std::vector<int> liveInId;
    std::vector<std::vector<int>> readersSinceDef;
    std::vector<int> touchedValues;

    void prepare (size_t numValues)
    {
        if (lastDef.size() < numValues)
        {
            lastDef.assign (numValues, -1);
            liveInId.assign (numValues, -1);
            readersSinceDef.assign (numValues, {});
        }
        touchedValues.clear();
    }

    void resetTouched()
    {
        for (const int val : touchedValues)
        {
            const auto id = static_cast<size_t> (val);
            lastDef[id] = -1;
            liveInId[id] = -1;
            readersSinceDef[id].clear();
        }
        touchedValues.clear();
    }
};

bool scheduleRegion (YdspIrFunction& fn,
                     size_t blockIndex,
                     size_t regionStart,
                     size_t regionEnd,
                     const std::vector<YdspValueType>& valueTypes,
                     const std::vector<int>& defCount,
                     const std::vector<std::optional<int64_t>>& globalIntConstants,
                     const std::vector<char>& usedInOtherBlock,
                     const std::vector<char>& nonUnrolledInductions,
                     ScheduleWorkspace& ws)
{
    if (regionEnd <= regionStart + 1)
        return false;

    const int n = static_cast<int> (regionEnd - regionStart);
    auto& block = fn.blocks[blockIndex];
    auto& insts = block.insts;
    const size_t numValues = valueTypes.size();

    // Track known constant integer indices up to regionEnd so disjoint
    // constant-indexed array/stream accesses can reorder freely.
    std::vector<std::optional<int64_t>> localIntConstants = globalIntConstants;
    std::vector<std::optional<int64_t>> constIndexOf (static_cast<size_t> (n));
    std::vector<ScheduleMemInfo> memInfoOf (static_cast<size_t> (n));

    for (size_t k = 0; k < regionEnd; ++k)
    {
        const auto& inst = insts[k];

        if (k >= regionStart)
        {
            const size_t localIdx = k - regionStart;
            const auto memInfo = classifyScheduleMemory (inst, valueTypes);
            memInfoOf[localIdx] = memInfo;

            if (memInfo.hasIndexedOffset && inst.a >= 0 && static_cast<size_t> (inst.a) < numValues)
                constIndexOf[localIdx] = localIntConstants[static_cast<size_t> (inst.a)];
        }

        if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
        {
            const auto id = static_cast<size_t> (inst.result);

            if (defCount[id] != 1)
            {
                if (inst.op == YdspIrOp::constI && fn.laneCountOf (inst.result) == 1
                    && inst.ivalue >= 0 && inst.ivalue <= std::numeric_limits<int32_t>::max())
                    localIntConstants[id] = inst.ivalue;
                else
                    localIntConstants[id].reset();
            }
        }
    }

    // Check which values read or defined in this region can escape past
    // regionEnd (read later in the same block, by termCond, in another block,
    // or carried across a loop iteration).
    std::vector<char> escapesAfterRegion (numValues, 0);
    {
        std::vector<char> redefinedAfter (numValues, 0);

        for (size_t k = regionEnd; k < insts.size(); ++k)
        {
            const auto& inst = insts[k];
            const int operands[] { inst.a, inst.b, inst.c };

            for (int opIdx = 0; opIdx < 3; ++opIdx)
            {
                if (! isValueIdOperand (inst.op, opIdx))
                    continue;

                const int val = operands[opIdx];
                if (val >= 0 && static_cast<size_t> (val) < numValues && ! redefinedAfter[static_cast<size_t> (val)])
                    escapesAfterRegion[static_cast<size_t> (val)] = 1;
            }

            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                redefinedAfter[static_cast<size_t> (inst.result)] = 1;
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond >= 0 && static_cast<size_t> (block.termCond) < numValues
            && ! redefinedAfter[static_cast<size_t> (block.termCond)])
            escapesAfterRegion[static_cast<size_t> (block.termCond)] = 1;

        for (size_t v = 0; v < numValues; ++v)
        {
            if (redefinedAfter[v])
                continue;

            if (usedInOtherBlock[v] || nonUnrolledInductions[v])
                escapesAfterRegion[v] = 1;
        }

        // A value read before its first definition in this block (or before
        // regionStart) may be loop-carried across a self/outer loop back-edge.
        std::vector<char> definedSoFar (numValues, 0);

        for (size_t k = 0; k < regionEnd; ++k)
        {
            const auto& inst = insts[k];
            const int operands[] { inst.a, inst.b, inst.c };

            for (int opIdx = 0; opIdx < 3; ++opIdx)
            {
                if (! isValueIdOperand (inst.op, opIdx))
                    continue;

                const int val = operands[opIdx];
                if (val >= 0 && static_cast<size_t> (val) < numValues && ! definedSoFar[static_cast<size_t> (val)])
                    escapesAfterRegion[static_cast<size_t> (val)] = 1;
            }

            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                definedSoFar[static_cast<size_t> (inst.result)] = 1;
        }
    }

    // Build DAG edges: RAW, WAR, WAW on register ids, plus memory aliasing edges.
    std::vector<std::vector<ScheduleEdge>> adj (static_cast<size_t> (n));
    std::vector<std::vector<int>> rawSuccs (static_cast<size_t> (n));
    std::vector<int> inDegree (static_cast<size_t> (n), 0);

    const auto addEdge = [&adj, &rawSuccs, &inDegree] (int from, int to, int latency, bool isRaw)
    {
        if (from == to)
            return;

        for (auto& existing : adj[static_cast<size_t> (from)])
        {
            if (existing.to == to)
            {
                existing.latency = std::max (existing.latency, latency);
                if (isRaw && ! existing.isRaw)
                {
                    existing.isRaw = true;
                    rawSuccs[static_cast<size_t> (from)].push_back (to);
                }
                return;
            }
        }

        adj[static_cast<size_t> (from)].push_back ({ to, latency, isRaw });
        ++inDegree[static_cast<size_t> (to)];

        if (isRaw)
            rawSuccs[static_cast<size_t> (from)].push_back (to);
    };

    // Value tracking (dense over numValues, reset via touched list to stay O(n)).
    ws.prepare (numValues);
    auto& lastDef = ws.lastDef;
    auto& liveInId = ws.liveInId;
    auto& readersSinceDef = ws.readersSinceDef;
    auto& touchedValues = ws.touchedValues;

    std::vector<int> liveInValueIds;
    std::vector<std::vector<int>> rawInputs (static_cast<size_t> (n));

    for (int i = 0; i < n; ++i)
    {
        const auto& inst = insts[regionStart + static_cast<size_t> (i)];
        const int operands[] { inst.a, inst.b, inst.c };

        int uniqueReads[3] { -1, -1, -1 };
        int numUniqueReads = 0;

        for (int opIdx = 0; opIdx < 3; ++opIdx)
        {
            if (! isValueIdOperand (inst.op, opIdx))
                continue;

            const int val = operands[opIdx];
            if (val < 0 || static_cast<size_t> (val) >= numValues)
                continue;

            bool duplicate = false;
            for (int r = 0; r < numUniqueReads; ++r)
                if (uniqueReads[r] == val)
                    duplicate = true;

            if (! duplicate)
                uniqueReads[numUniqueReads++] = val;
        }

        for (int r = 0; r < numUniqueReads; ++r)
        {
            const int val = uniqueReads[r];
            const auto id = static_cast<size_t> (val);

            if (lastDef[id] < 0 && liveInId[id] < 0 && readersSinceDef[id].empty())
                touchedValues.push_back (val);

            if (lastDef[id] >= 0)
            {
                const int defInst = lastDef[id];
                const int lat = scheduleLatencyOf (insts[regionStart + static_cast<size_t> (defInst)].op);
                addEdge (defInst, i, lat, true);
                rawInputs[static_cast<size_t> (i)].push_back (defInst);
            }
            else
            {
                if (liveInId[id] < 0)
                {
                    liveInId[id] = n + static_cast<int> (liveInValueIds.size());
                    liveInValueIds.push_back (val);
                }

                rawInputs[static_cast<size_t> (i)].push_back (liveInId[id]);
            }

            readersSinceDef[id].push_back (i);
        }

        if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
        {
            const int w = inst.result;
            const auto id = static_cast<size_t> (w);

            if (lastDef[id] < 0 && liveInId[id] < 0 && readersSinceDef[id].empty())
                touchedValues.push_back (w);

            if (lastDef[id] >= 0)
                addEdge (lastDef[id], i, 0, false);

            for (const int reader : readersSinceDef[id])
                if (reader != i)
                    addEdge (reader, i, 0, false);

            lastDef[id] = i;
            readersSinceDef[id].clear();
        }
    }

    // Remember which instruction in [0, n) is the final definition of each
    // modified value id so we know which definition can escape the region.
    std::vector<char> isFinalDefInRegion (static_cast<size_t> (n), 0);
    for (const int val : touchedValues)
    {
        const auto id = static_cast<size_t> (val);
        if (lastDef[id] >= 0)
            isFinalDefInRegion[static_cast<size_t> (lastDef[id])] = 1;
    }

    // Memory / side-effect dependency edges.
    for (int i = 0; i < n; ++i)
    {
        const auto& secondInfo = memInfoOf[static_cast<size_t> (i)];
        if (secondInfo.family == ScheduleMemFamily::none)
            continue;

        const auto& secondInst = insts[regionStart + static_cast<size_t> (i)];

        for (int prev = i - 1; prev >= 0; --prev)
        {
            const auto& firstInfo = memInfoOf[static_cast<size_t> (prev)];
            if (firstInfo.family != secondInfo.family)
                continue;

            const auto& firstInst = insts[regionStart + static_cast<size_t> (prev)];

            if (memoryOpsConflict (fn,
                                   firstInst,
                                   firstInfo,
                                   constIndexOf[static_cast<size_t> (prev)],
                                   secondInst,
                                   secondInfo,
                                   constIndexOf[static_cast<size_t> (i)]))
            {
                const int memLatency = firstInfo.isWrite && ! secondInfo.isWrite ? 1 : 0;
                addEdge (prev, i, memLatency, false);
            }
        }
    }

    // Compute critical-path height (longest latency path to any exit/sink).
    std::vector<int> selfLatency (static_cast<size_t> (n), 0);
    std::vector<int> height (static_cast<size_t> (n), 0);

    for (int i = n - 1; i >= 0; --i)
    {
        const auto& inst = insts[regionStart + static_cast<size_t> (i)];
        selfLatency[static_cast<size_t> (i)] = scheduleLatencyOf (inst.op);

        int h = selfLatency[static_cast<size_t> (i)];

        if (block.term == YdspIrTerm::branchIf && inst.result >= 0 && inst.result == block.termCond
            && isFinalDefInRegion[static_cast<size_t> (i)] && regionEnd == insts.size())
            h += 1;

        for (const auto& edge : adj[static_cast<size_t> (i)])
            h = std::max (h, height[static_cast<size_t> (edge.to)] + edge.latency);

        height[static_cast<size_t> (i)] = h;
    }

    // Live-register tracking per instance (instructions 0..n-1 + live-in nodes).
    const int numInstances = n + static_cast<int> (liveInValueIds.size());
    std::vector<int> remainingUses (static_cast<size_t> (numInstances), 0);
    std::vector<char> instanceIsFp (static_cast<size_t> (numInstances), 0);

    int liveFp = 0;
    int liveGp = 0;

    for (int i = 0; i < n; ++i)
    {
        const auto& inst = insts[regionStart + static_cast<size_t> (i)];
        if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
        {
            instanceIsFp[static_cast<size_t> (i)] = isFpRegisterClass (fn, valueTypes, inst.result, inst.op) ? 1 : 0;
            remainingUses[static_cast<size_t> (i)] = static_cast<int> (rawSuccs[static_cast<size_t> (i)].size());

            if (isFinalDefInRegion[static_cast<size_t> (i)] && escapesAfterRegion[static_cast<size_t> (inst.result)])
                ++remainingUses[static_cast<size_t> (i)];
        }
    }

    for (size_t k = 0; k < liveInValueIds.size(); ++k)
    {
        const int val = liveInValueIds[k];
        const int instId = n + static_cast<int> (k);
        const bool isFp = isFpRegisterClass (fn, valueTypes, val, YdspIrOp::constF);

        instanceIsFp[static_cast<size_t> (instId)] = isFp ? 1 : 0;

        if (lastDef[static_cast<size_t> (val)] < 0 && escapesAfterRegion[static_cast<size_t> (val)])
            ++remainingUses[static_cast<size_t> (instId)];

        if (isFp)
            ++liveFp;
        else
            ++liveGp;
    }

    for (int i = 0; i < n; ++i)
        for (const int srcInst : rawInputs[static_cast<size_t> (i)])
            if (srcInst >= n)
                ++remainingUses[static_cast<size_t> (srcInst)];

    // Reset touched workspace arrays before scheduling loop.
    ws.resetTouched();

    // Target soft register limits.
    const int softFpLimit = fn.vectorWidth >= 8 ? 14 : 16;
    const int softGpLimit = 12;

    std::vector<int> ready;
    ready.reserve (static_cast<size_t> (n));
    std::vector<int> earliestReady (static_cast<size_t> (n), 0);

    for (int i = 0; i < n; ++i)
        if (inDegree[static_cast<size_t> (i)] == 0)
            ready.push_back (i);

    std::vector<int> schedule;
    schedule.reserve (static_cast<size_t> (n));

    int currentCycle = 0;

    const auto computeDelta = [&] (int u, int& deltaFp, int& deltaGp)
    {
        deltaFp = 0;
        deltaGp = 0;

        for (const int srcInst : rawInputs[static_cast<size_t> (u)])
        {
            if (remainingUses[static_cast<size_t> (srcInst)] == 1)
            {
                if (instanceIsFp[static_cast<size_t> (srcInst)])
                    --deltaFp;
                else
                    --deltaGp;
            }
        }

        const auto& inst = insts[regionStart + static_cast<size_t> (u)];
        if (inst.result >= 0 && remainingUses[static_cast<size_t> (u)] > 0)
        {
            if (instanceIsFp[static_cast<size_t> (u)])
                ++deltaFp;
            else
                ++deltaGp;
        }
    };

    const auto unlocksReadyConsumer = [&] (int u) noexcept -> bool
    {
        for (const int succ : rawSuccs[static_cast<size_t> (u)])
            if (inDegree[static_cast<size_t> (succ)] == 1)
                return true;

        return false;
    };

    while (! ready.empty())
    {
        bool anyReadyNow = false;
        int minEarliest = std::numeric_limits<int>::max();

        for (const int u : ready)
        {
            const int er = earliestReady[static_cast<size_t> (u)];
            if (er <= currentCycle)
            {
                anyReadyNow = true;
                break;
            }

            minEarliest = std::min (minEarliest, er);
        }

        if (! anyReadyNow)
            currentCycle = minEarliest;

        const bool fpHigh = liveFp >= softFpLimit;
        const bool gpHigh = liveGp >= softGpLimit;
        const bool highPressure = fpHigh || gpHigh;

        struct CandidateScore
        {
            int idx = -1;
            int pressureDelta = 0;
            int netDelta = 0;
            bool unlocks = false;
            bool readyNow = false;
            int earliest = 0;
            int height = 0;
            int selfLat = 0;
            int origPos = 0;
        };

        auto scoreOf = [&] (int readyPos) -> CandidateScore
        {
            const int u = ready[static_cast<size_t> (readyPos)];
            int dFp = 0;
            int dGp = 0;
            computeDelta (u, dFp, dGp);

            CandidateScore s;
            s.idx = readyPos;
            s.pressureDelta = (fpHigh ? dFp : 0) + (gpHigh ? dGp : 0);
            s.netDelta = dFp + dGp;
            s.unlocks = highPressure && s.pressureDelta > 0 && unlocksReadyConsumer (u);
            s.readyNow = earliestReady[static_cast<size_t> (u)] <= currentCycle;
            s.earliest = earliestReady[static_cast<size_t> (u)];
            s.height = height[static_cast<size_t> (u)];
            s.selfLat = selfLatency[static_cast<size_t> (u)];
            s.origPos = u;
            return s;
        };

        const auto isBetter = [highPressure] (const CandidateScore& a, const CandidateScore& b) noexcept -> bool
        {
            if (highPressure)
            {
                if (a.pressureDelta != b.pressureDelta)
                    return a.pressureDelta < b.pressureDelta;

                if (a.unlocks != b.unlocks)
                    return a.unlocks;

                if (a.readyNow != b.readyNow)
                    return a.readyNow;

                if (a.height != b.height)
                    return a.height > b.height;

                if (a.netDelta != b.netDelta)
                    return a.netDelta < b.netDelta;

                if (a.earliest != b.earliest)
                    return a.earliest < b.earliest;

                return a.origPos < b.origPos;
            }

            if (a.readyNow != b.readyNow)
                return a.readyNow;

            if (! a.readyNow && a.earliest != b.earliest)
                return a.earliest < b.earliest;

            if (a.height != b.height)
                return a.height > b.height;

            if (a.netDelta != b.netDelta)
                return a.netDelta < b.netDelta;

            if (a.selfLat != b.selfLat)
                return a.selfLat > b.selfLat;

            return a.origPos < b.origPos;
        };

        CandidateScore best = scoreOf (0);

        for (int pos = 1; pos < static_cast<int> (ready.size()); ++pos)
        {
            const CandidateScore candidate = scoreOf (pos);
            if (isBetter (candidate, best))
                best = candidate;
        }

        const int chosen = ready[static_cast<size_t> (best.idx)];
        ready[static_cast<size_t> (best.idx)] = ready.back();
        ready.pop_back();

        const int issueCycle = std::max (currentCycle, earliestReady[static_cast<size_t> (chosen)]);
        currentCycle = issueCycle + 1;

        schedule.push_back (chosen);

        // Update live register counts.
        for (const int srcInst : rawInputs[static_cast<size_t> (chosen)])
        {
            auto& rem = remainingUses[static_cast<size_t> (srcInst)];
            --rem;
            if (rem == 0)
            {
                if (instanceIsFp[static_cast<size_t> (srcInst)])
                    --liveFp;
                else
                    --liveGp;
            }
        }

        if (insts[regionStart + static_cast<size_t> (chosen)].result >= 0
            && remainingUses[static_cast<size_t> (chosen)] > 0)
        {
            if (instanceIsFp[static_cast<size_t> (chosen)])
                ++liveFp;
            else
                ++liveGp;
        }

        // Release successors.
        for (const auto& edge : adj[static_cast<size_t> (chosen)])
        {
            const int succ = edge.to;
            earliestReady[static_cast<size_t> (succ)] = std::max (
                earliestReady[static_cast<size_t> (succ)],
                issueCycle + edge.latency);

            --inDegree[static_cast<size_t> (succ)];
            if (inDegree[static_cast<size_t> (succ)] == 0)
                ready.push_back (succ);
        }
    }

    if (static_cast<int> (schedule.size()) != n)
        return false;

    bool orderChanged = false;
    for (int i = 0; i < n; ++i)
    {
        if (schedule[static_cast<size_t> (i)] != i)
        {
            orderChanged = true;
            break;
        }
    }

    if (! orderChanged)
        return false;

    std::vector<YdspIrInst> reordered;
    reordered.reserve (static_cast<size_t> (n));

    for (const int idx : schedule)
        reordered.push_back (std::move (insts[regionStart + static_cast<size_t> (idx)]));

    for (int i = 0; i < n; ++i)
        insts[regionStart + static_cast<size_t> (i)] = std::move (reordered[static_cast<size_t> (i)]);

    return true;
}

} // namespace

//==============================================================================

bool YdspOptimizer::scheduleInstructions (YdspIrFunction& fn)
{
    if (fn.blocks.empty())
        return false;

    const auto valueTypes = resolveValueTypes (fn);
    const size_t numValues = valueTypes.size();

    if (numValues == 0)
        return false;

    std::vector<int> defCount (numValues, 0);
    std::vector<std::optional<int64_t>> globalIntConstants (numValues);

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                ++defCount[static_cast<size_t> (inst.result)];
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
                globalIntConstants[id] = inst.ivalue;
        }
    }

    // Track non-unrolled loop induction variables and smallest enclosing loop
    // per block so trailing induction updates stay pinned at loop latches.
    std::vector<char> nonUnrolledInductions (numValues, 0);
    std::vector<int> blockLoopInduction (fn.blocks.size(), -1);
    std::vector<int> blockLoopSpan (fn.blocks.size(), std::numeric_limits<int>::max());

    for (const auto& loop : fn.loops)
    {
        if (loop.unrolled)
            continue;

        if (loop.induction >= 0 && static_cast<size_t> (loop.induction) < numValues)
            nonUnrolledInductions[static_cast<size_t> (loop.induction)] = 1;

        const int start = std::max (0, loop.headerBlock);
        const int end = std::min (static_cast<int> (fn.blocks.size()), loop.exitBlock);
        const int span = end - start;

        for (int b = start; b < end; ++b)
        {
            if (span < blockLoopSpan[static_cast<size_t> (b)])
            {
                blockLoopSpan[static_cast<size_t> (b)] = span;
                blockLoopInduction[static_cast<size_t> (b)] = loop.induction;
            }
        }
    }

    // Per-block external-use check: value v is used outside block b if total
    // uses in the function exceed uses inside block b.
    std::vector<int> totalUses (numValues, 0);
    forEachValueUse (fn, [&totalUses, numValues] (int, int, int, int value)
    {
        if (value >= 0 && static_cast<size_t> (value) < numValues)
            ++totalUses[static_cast<size_t> (value)];
    });

    ScheduleWorkspace workspace;
    bool changed = false;

    for (size_t blockIndex = 0; blockIndex < fn.blocks.size(); ++blockIndex)
    {
        const auto& block = fn.blocks[blockIndex];
        if (block.insts.size() < 2)
            continue;

        size_t schedStart = 0;
        while (schedStart < block.insts.size() && block.insts[schedStart].op == YdspIrOp::phi)
            ++schedStart;

        size_t schedEnd = block.insts.size();
        const int induction = blockLoopInduction[blockIndex];

        if (induction >= 0 && schedEnd > schedStart)
        {
            const auto& back = block.insts[schedEnd - 1];
            if (schedEnd >= schedStart + 2
                && back.op == YdspIrOp::movI && back.result == induction
                && block.insts[schedEnd - 2].op == YdspIrOp::addI
                && block.insts[schedEnd - 2].result == back.a)
            {
                schedEnd -= 2;
            }
            else if ((back.op == YdspIrOp::addI || back.op == YdspIrOp::advanceWrapI)
                     && back.result == induction)
            {
                schedEnd -= 1;
            }
        }

        if (schedEnd <= schedStart + 1)
            continue;

        // Compute uses inside this block to identify values used in other blocks.
        std::vector<int> localUses (numValues, 0);
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::phi)
            {
                for (const auto& [pred, val] : inst.phiIncoming)
                {
                    (void) pred;
                    if (val >= 0 && static_cast<size_t> (val) < numValues)
                        ++localUses[static_cast<size_t> (val)];
                }
                continue;
            }

            const int operands[] { inst.a, inst.b, inst.c };
            for (int opIdx = 0; opIdx < 3; ++opIdx)
            {
                if (! isValueIdOperand (inst.op, opIdx))
                    continue;

                const int val = operands[opIdx];
                if (val >= 0 && static_cast<size_t> (val) < numValues)
                    ++localUses[static_cast<size_t> (val)];
            }
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond >= 0 && static_cast<size_t> (block.termCond) < numValues)
            ++localUses[static_cast<size_t> (block.termCond)];

        std::vector<char> usedInOtherBlock (numValues, 0);
        for (size_t v = 0; v < numValues; ++v)
            if (totalUses[v] > localUses[v])
                usedInOtherBlock[v] = 1;

        // Partition [schedStart, schedEnd) at call/event/trace boundaries so
        // values do not get hoisted across clobbering calls or past remat leaves.
        size_t regionStart = schedStart;

        for (size_t k = schedStart; k <= schedEnd; ++k)
        {
            const bool isBoundary = (k == schedEnd) || isScheduleBarrierOp (fn.blocks[blockIndex].insts[k].op);

            if (! isBoundary)
                continue;

            if (k > regionStart + 1)
            {
                changed |= scheduleRegion (fn,
                                           blockIndex,
                                           regionStart,
                                           k,
                                           valueTypes,
                                           defCount,
                                           globalIntConstants,
                                           usedInOtherBlock,
                                           nonUnrolledInductions,
                                           workspace);
            }

            regionStart = k + 1;
        }
    }

    return changed;
}

} // namespace yup
