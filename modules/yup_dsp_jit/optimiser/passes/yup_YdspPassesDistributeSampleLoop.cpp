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

namespace
{

class SampleLoopDistributor
{
public:
    explicit SampleLoopDistributor (YdspIrFunction& functionToDistribute)
        : fn (functionToDistribute)
    {
    }

    bool run()
    {
        if (! fn.isSampleMode || fn.isInit || fn.isEventHandler)
            return false;

        if (fn.loops.size() != 1 || fn.blocks.size() < 4)
            return false;

        const auto origLoop = fn.loops[0];

        if (origLoop.unrolled
            || origLoop.bound.kind != YdspLoopBoundKind::blockSize
            || origLoop.headerBlock != 1
            || origLoop.exitBlock != 3
            || origLoop.induction < 0)
            return false;

        const auto& preheader = fn.blocks[0];
        const auto& header = fn.blocks[1];
        const auto& body = fn.blocks[2];

        const bool validPreheaderTerm = preheader.term == YdspIrTerm::fallthrough
                                     || (preheader.term == YdspIrTerm::branch && preheader.termTarget == 1);

        if (! validPreheaderTerm
            || header.term != YdspIrTerm::branchIf || header.termTarget != 2 || header.termTarget2 != 3
            || body.term != YdspIrTerm::branch || body.termTarget != 1)
            return false;

        for (size_t b = 3; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];

            if ((block.termTarget >= 0 && block.termTarget < 3)
                || (block.termTarget2 >= 0 && block.termTarget2 < 3))
                return false;
        }

        if (header.insts.size() != 1)
            return false;

        const auto& headerCmp = header.insts[0];

        if (headerCmp.op != YdspIrOp::ltI
            || headerCmp.result != header.termCond
            || headerCmp.a != origLoop.induction
            || headerCmp.b < 0)
            return false;

        const int blockSizeVal = headerCmp.b;

        numValues = static_cast<int> (fn.valueTypes.size());

        if (numValues <= 0 || blockSizeVal >= numValues)
            return false;

        if (! verifyPreheaderZeroStart (origLoop.induction))
            return false;

        if (body.insts.size() < 3)
            return false;

        const auto& movInst = body.insts.back();

        if (movInst.op != YdspIrOp::movI || movInst.result != origLoop.induction || movInst.a < 0)
            return false;

        int incIndex = -1;

        for (size_t i = body.insts.size() - 1; i-- > 0;)
        {
            if (body.insts[i].result == movInst.a)
            {
                incIndex = static_cast<int> (i);
                break;
            }
        }

        if (incIndex < 0)
            return false;

        const auto& incInst = body.insts[static_cast<size_t> (incIndex)];

        if (incInst.op != YdspIrOp::addI
            || incInst.a != origLoop.induction
            || incInst.result < 0
            || ! isConstantInt (incInst.b, 1))
            return false;

        std::vector<size_t> payloadIndices;
        payloadIndices.reserve (body.insts.size() - 2);

        for (size_t i = 0; i + 1 < body.insts.size(); ++i)
            if (i != static_cast<size_t> (incIndex))
                payloadIndices.push_back (i);

        if (payloadIndices.empty())
            return false;

        if (! analyseBody (origLoop.induction, incInst.result, payloadIndices))
            return false;

        if (! partitionStages())
            return false;

        rewrite (origLoop, blockSizeVal);
        return true;
    }

private:
    static constexpr int chunkSize = 64;
    static constexpr int minVectorizableFloatOps = 4;
    static constexpr size_t maxBoundaryScalarsPerStage = 2;
    static constexpr size_t maxHeavyTranscendentalBoundary = 4;
    static constexpr size_t maxTotalScratchBuffers = 4;

    static bool boundaryFits (size_t boundaryCount, int transcendentalCount, int floatOpCount) noexcept
    {
        if (boundaryCount <= maxBoundaryScalarsPerStage)
            return transcendentalCount >= 1 || floatOpCount >= minVectorizableFloatOps;

        return boundaryCount <= maxHeavyTranscendentalBoundary
            && transcendentalCount > static_cast<int> (boundaryCount)
            && transcendentalCount >= 4;
    }

    struct StreamRef
    {
        bool isInput = false;
        int slot = -1;

        bool operator== (const StreamRef& other) const noexcept
        {
            return isInput == other.isInput && slot == other.slot;
        }
    };

    struct Node
    {
        YdspIrInst inst;
        int opNode[3] = { -1, -1, -1 };

        bool readsInitialCarried = false;
        bool writesCarried = false;
        bool isStatefulOp = false;
        bool isVectorizableOp = false;
        bool isVectorizableTranscendental = false;
        bool isVectorizableFloatOp = false;
        bool isLoadInput = false;
        bool isStoreOutput = false;

        bool dependsOnStateful = false;
        bool feedsStateful = false;
        bool dependsOnPerSample = false;
    };

    bool isConstantInt (int value, int64_t expected) const
    {
        if (value < 0 || value >= numValues)
            return false;

        int defs = 0;
        bool matches = false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result != value)
                    continue;

                ++defs;
                matches = (inst.op == YdspIrOp::constI && inst.ivalue == expected);
            }
        }

        return defs == 1 && matches;
    }

    bool verifyPreheaderZeroStart (int induction) const
    {
        const auto& preInsts = fn.blocks[0].insts;

        for (size_t i = preInsts.size(); i-- > 0;)
        {
            const auto& inst = preInsts[i];

            if (inst.result != induction)
                continue;

            if (inst.op == YdspIrOp::movI)
                return isConstantInt (inst.a, 0);

            if (inst.op == YdspIrOp::constI)
                return inst.ivalue == 0;

            return false;
        }

        return false;
    }

    static bool isTranscendentalOp (YdspIrOp op) noexcept
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

    void classifyVectorizability (Node& node, int induction) const
    {
        const auto& inst = node.inst;

        const auto isF32 = [this] (int v) noexcept
        {
            return v >= 0
                && static_cast<size_t> (v) < fn.valueTypes.size()
                && fn.valueTypes[static_cast<size_t> (v)] == YdspValueType::float32Type;
        };

        const auto isBool = [this] (int v) noexcept
        {
            return v >= 0
                && static_cast<size_t> (v) < fn.valueTypes.size()
                && fn.valueTypes[static_cast<size_t> (v)] == YdspValueType::boolType;
        };

        switch (inst.op)
        {
            case YdspIrOp::constF:
                node.isVectorizableOp = isF32 (inst.result);
                return;

            case YdspIrOp::constB:
                node.isVectorizableOp = isBool (inst.result);
                return;

            case YdspIrOp::loadInput:
                if (inst.a == induction && isF32 (inst.result))
                {
                    node.isLoadInput = true;
                    node.isVectorizableOp = true;
                }
                return;

            case YdspIrOp::storeOutput:
                if (inst.a == induction && isF32 (inst.b))
                {
                    node.isStoreOutput = true;
                    node.isVectorizableOp = true;
                }
                return;

            case YdspIrOp::movF:
                node.isVectorizableOp = isF32 (inst.result) && isF32 (inst.a);
                return;

            case YdspIrOp::movB:
                node.isVectorizableOp = isBool (inst.result) && isBool (inst.a);
                return;

            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::floorF:
            case YdspIrOp::ceilF:
            case YdspIrOp::rintF:
                if (isF32 (inst.result) && isF32 (inst.a))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::addF:
            case YdspIrOp::subF:
            case YdspIrOp::mulF:
            case YdspIrOp::divF:
            case YdspIrOp::minF:
            case YdspIrOp::maxF:
                if (isF32 (inst.result) && isF32 (inst.a) && isF32 (inst.b))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::clampF:
            case YdspIrOp::lerpF:
                if (isF32 (inst.result) && isF32 (inst.a) && isF32 (inst.b) && isF32 (inst.c))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::fmaF:
            case YdspIrOp::fmsubF:
                if (fn.packedFmaAvailable && isF32 (inst.result) && isF32 (inst.a) && isF32 (inst.b) && isF32 (inst.c))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::eqF:
            case YdspIrOp::neF:
            case YdspIrOp::ltF:
            case YdspIrOp::leF:
            case YdspIrOp::gtF:
            case YdspIrOp::geF:
                if (isBool (inst.result) && isF32 (inst.a) && isF32 (inst.b))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::andB:
            case YdspIrOp::orB:
                if (isBool (inst.result) && isBool (inst.a) && isBool (inst.b))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::notB:
                if (isBool (inst.result) && isBool (inst.a))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            case YdspIrOp::selectB:
                if (isBool (inst.a) && isF32 (inst.result) && isF32 (inst.b) && isF32 (inst.c))
                {
                    node.isVectorizableOp = true;
                    node.isVectorizableFloatOp = true;
                }
                return;

            default:
                if (isTranscendentalOp (inst.op))
                {
                    const bool canWiden = fn.vectorMathEnabled || (fn.fastMath && isInlineFastMathTranscendental (inst.op));

                    if (! canWiden || ! isF32 (inst.result) || ! isF32 (inst.a))
                        return;

                    if ((inst.op == YdspIrOp::powF || inst.op == YdspIrOp::atan2F || inst.op == YdspIrOp::fmodF)
                        && ! isF32 (inst.b))
                        return;

                    node.isVectorizableOp = true;
                    node.isVectorizableTranscendental = true;
                }
                return;
        }
    }

    bool analyseBody (int induction, int incResult, const std::vector<size_t>& payloadIndices)
    {
        const auto& bodyInsts = fn.blocks[2].insts;
        const size_t payloadCount = payloadIndices.size();

        std::vector<int> defsOutside (static_cast<size_t> (numValues), 0);
        std::vector<int> usesOutside (static_cast<size_t> (numValues), 0);
        std::vector<int> defsInBody (static_cast<size_t> (numValues), 0);

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];

            if (b == 2)
            {
                for (const size_t idx : payloadIndices)
                {
                    const auto& inst = block.insts[idx];

                    if (inst.result >= 0)
                    {
                        if (inst.result >= numValues || inst.result == induction || inst.result == incResult)
                            return false;

                        ++defsInBody[static_cast<size_t> (inst.result)];
                    }
                }

                continue;
            }

            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0)
                {
                    if (inst.result >= numValues)
                        return false;

                    ++defsOutside[static_cast<size_t> (inst.result)];
                }

                for (int operand = 0; operand < 3; ++operand)
                {
                    if (! isValueIdOperand (inst.op, operand))
                        continue;

                    const int val = operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c);

                    if (val >= 0)
                    {
                        if (val >= numValues)
                            return false;

                        if (b != 1 || val != induction)
                            ++usesOutside[static_cast<size_t> (val)];
                    }
                }
            }

            if (block.term == YdspIrTerm::branchIf && block.termCond >= 0)
            {
                if (block.termCond >= numValues)
                    return false;

                ++usesOutside[static_cast<size_t> (block.termCond)];
            }
        }

        if (usesOutside[static_cast<size_t> (incResult)] > 0 || usesOutside[static_cast<size_t> (induction)] > 0)
            return false;

        // Determine loop-carried / escaping values and disallow unsafe ops.
        isCarried.assign (static_cast<size_t> (numValues), false);
        std::vector<bool> seenDefInBody (static_cast<size_t> (numValues), false);

        bool sawStoreOutput = false;

        for (size_t i = 0; i < payloadCount; ++i)
        {
            const auto& inst = bodyInsts[payloadIndices[i]];

            if (inst.result >= 0 && fn.laneCountOf (inst.result) > 1)
                return false;

            switch (inst.op)
            {
                case YdspIrOp::phi:
                case YdspIrOp::loadOutput:
                case YdspIrOp::traceInt:
                case YdspIrOp::traceFloat:
                case YdspIrOp::traceCommit:
                case YdspIrOp::emitEvent:
                case YdspIrOp::storeParam:
                case YdspIrOp::storeParamOut:
                case YdspIrOp::storeEventFieldF:
                case YdspIrOp::storeEventFieldI:
                case YdspIrOp::vsplat:
                case YdspIrOp::vrampF:
                case YdspIrOp::vreduceAddF:
                case YdspIrOp::loadInputAddress:
                case YdspIrOp::loadOutputAddress:
                    return false;

                case YdspIrOp::loadInput:
                    if (sawStoreOutput)
                        return false;
                    break;

                case YdspIrOp::storeOutput:
                    sawStoreOutput = true;
                    break;

                default:
                    break;
            }

            for (int operand = 0; operand < 3; ++operand)
            {
                if (! isValueIdOperand (inst.op, operand))
                    continue;

                const int val = operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c);

                if (val < 0)
                    continue;

                if (val == incResult)
                    return false;

                if (val == induction)
                {
                    if ((inst.op != YdspIrOp::loadInput && inst.op != YdspIrOp::storeOutput) || operand != 0)
                        return false;

                    continue;
                }

                if (defsInBody[static_cast<size_t> (val)] > 0 && ! seenDefInBody[static_cast<size_t> (val)])
                    isCarried[static_cast<size_t> (val)] = true;
            }

            if (inst.result >= 0)
            {
                const auto res = static_cast<size_t> (inst.result);
                seenDefInBody[res] = true;

                if (usesOutside[res] > 0 || defsOutside[res] > 0)
                    isCarried[res] = true;
            }
        }

        if (fn.blocks[2].termCond == induction || fn.blocks[2].termCond == incResult)
            return false;

        // Build per-iteration versioned DAG nodes.
        nodes.resize (payloadCount);
        std::vector<int> currentDef (static_cast<size_t> (numValues), -1);
        bool hasAnyStateful = false;

        for (size_t i = 0; i < payloadCount; ++i)
        {
            auto& node = nodes[i];
            node.inst = bodyInsts[payloadIndices[i]];

            for (int operand = 0; operand < 3; ++operand)
            {
                if (! isValueIdOperand (node.inst.op, operand))
                    continue;

                const int val = operand == 0 ? node.inst.a : (operand == 1 ? node.inst.b : node.inst.c);

                if (val < 0 || val == induction)
                    continue;

                if (currentDef[static_cast<size_t> (val)] >= 0)
                    node.opNode[operand] = currentDef[static_cast<size_t> (val)];
                else if (isCarried[static_cast<size_t> (val)])
                    node.readsInitialCarried = true;
            }

            if (node.inst.result >= 0)
            {
                if (isCarried[static_cast<size_t> (node.inst.result)])
                    node.writesCarried = true;

                currentDef[static_cast<size_t> (node.inst.result)] = static_cast<int> (i);
            }

            classifyVectorizability (node, induction);

            const bool isMemoryStateful = node.inst.op == YdspIrOp::loadStateF
                                       || node.inst.op == YdspIrOp::storeStateF
                                       || node.inst.op == YdspIrOp::loadStateI
                                       || node.inst.op == YdspIrOp::storeStateI
                                       || node.inst.op == YdspIrOp::loadStateArrayF
                                       || node.inst.op == YdspIrOp::storeStateArrayF
                                       || node.inst.op == YdspIrOp::loadStateArrayI
                                       || node.inst.op == YdspIrOp::storeStateArrayI
                                       || node.inst.op == YdspIrOp::loadParamOut
                                       || node.inst.op == YdspIrOp::storeParamOut;

            node.isStatefulOp = isMemoryStateful
                             || node.readsInitialCarried
                             || node.writesCarried
                             || (! node.isVectorizableOp && ! node.isStoreOutput);

            if (node.isStatefulOp)
                hasAnyStateful = true;
        }

        if (! hasAnyStateful)
            return false;

        // Forward pass: dependsOnStateful & dependsOnPerSample.
        for (size_t i = 0; i < payloadCount; ++i)
        {
            auto& node = nodes[i];
            node.dependsOnStateful = node.isStatefulOp;
            node.dependsOnPerSample = node.isLoadInput || node.isStatefulOp;

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = node.opNode[operand];

                if (dep < 0)
                    continue;

                node.dependsOnStateful |= nodes[static_cast<size_t> (dep)].dependsOnStateful;
                node.dependsOnPerSample |= nodes[static_cast<size_t> (dep)].dependsOnPerSample;
            }
        }

        // Backward pass: feedsStateful.
        for (size_t i = payloadCount; i-- > 0;)
        {
            auto& node = nodes[i];

            if (node.isStatefulOp)
                node.feedsStateful = true;

            if (! node.feedsStateful)
                continue;

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = node.opNode[operand];

                if (dep >= 0)
                    nodes[static_cast<size_t> (dep)].feedsStateful = true;
            }
        }

        return true;
    }

    bool partitionStages()
    {
        const size_t n = nodes.size();
        inStage0.assign (n, false);
        inStage2.assign (n, false);

        // Candidate Stage 0: per-sample vectorizable nodes that do not depend on stateful work.
        int stage0Transcendentals = 0;
        int stage0FloatOps = 0;

        for (size_t i = 0; i < n; ++i)
        {
            const auto& node = nodes[i];

            if (! node.dependsOnStateful && ! node.isStoreOutput && node.isVectorizableOp && node.dependsOnPerSample)
            {
                inStage0[i] = true;

                if (node.isVectorizableTranscendental)
                    ++stage0Transcendentals;
                else if (node.isVectorizableFloatOp)
                    ++stage0FloatOps;
            }
        }

        // Find Stage 0 exports (per-sample, non-loadInput Stage 0 nodes consumed outside Stage 0).
        std::vector<bool> exportedFromStage0 (n, false);

        for (size_t i = 0; i < n; ++i)
        {
            if (inStage0[i])
                continue;

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = nodes[i].opNode[operand];

                if (dep >= 0 && inStage0[static_cast<size_t> (dep)] && ! nodes[static_cast<size_t> (dep)].isLoadInput)
                    exportedFromStage0[static_cast<size_t> (dep)] = true;
            }
        }

        stage0Exports.clear();

        for (size_t i = 0; i < n; ++i)
            if (exportedFromStage0[i])
                stage0Exports.push_back (static_cast<int> (i));

        useStage0 = ! stage0Exports.empty()
                 && boundaryFits (stage0Exports.size(), stage0Transcendentals, stage0FloatOps);

        if (! useStage0)
        {
            std::fill (inStage0.begin(), inStage0.end(), false);
            stage0Exports.clear();
        }

        // Candidate Stage 2: per-sample/storeOutput vectorizable nodes that do not feed stateful work.
        int stage2Transcendentals = 0;
        int stage2FloatOps = 0;
        int totalStoreOutputs = 0;
        int stage2StoreOutputs = 0;

        for (size_t i = 0; i < n; ++i)
        {
            const auto& node = nodes[i];

            if (node.isStoreOutput)
                ++totalStoreOutputs;

            if (! inStage0[i]
                && ! node.feedsStateful
                && ! node.isStatefulOp
                && node.isVectorizableOp
                && (node.dependsOnPerSample || node.isStoreOutput))
            {
                inStage2[i] = true;

                if (node.isStoreOutput)
                    ++stage2StoreOutputs;
                else if (node.isVectorizableTranscendental)
                    ++stage2Transcendentals;
                else if (node.isVectorizableFloatOp)
                    ++stage2FloatOps;
            }
        }

        // All storeOutput instructions must move to Stage 2 if Stage 2 is extracted,
        // so output streams are only written in the final stage.
        const bool allStoresInStage2 = totalStoreOutputs > 0 && stage2StoreOutputs == totalStoreOutputs;

        // Find Stage 2 imports from Stage 1 (excluding loadInput and Stage 0 exports).
        std::vector<bool> importedToStage2 (n, false);

        for (size_t i = 0; i < n; ++i)
        {
            if (! inStage2[i])
                continue;

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = nodes[i].opNode[operand];

                if (dep < 0)
                    continue;

                const auto u = static_cast<size_t> (dep);

                if (! inStage2[u] && ! inStage0[u] && ! nodes[u].isLoadInput && nodes[u].dependsOnPerSample)
                    importedToStage2[u] = true;
            }
        }

        stage2Imports.clear();

        for (size_t i = 0; i < n; ++i)
            if (importedToStage2[i])
                stage2Imports.push_back (static_cast<int> (i));

        useStage2 = allStoresInStage2
                 && boundaryFits (stage2Imports.size(), stage2Transcendentals, stage2FloatOps);

        if (! useStage2)
        {
            std::fill (inStage2.begin(), inStage2.end(), false);
            stage2Imports.clear();
        }

        if (! useStage0 && ! useStage2)
        {
            if (! partitionDecoupledThreeStage())
                return false;
        }

        if (stage0Exports.size() + stage2Imports.size() > maxTotalScratchBuffers)
            return false;

        return true;
    }

    bool partitionDecoupledThreeStage()
    {
        // Split [StagePre (decoupled scalar state, e.g. LFO phase)]
        //   ->  [StageMid (vectorizable stateless work, e.g. sin/pow/tan)]
        //   ->  [StagePost (remaining stateful filter + stream outputs)].
        const size_t n = nodes.size();

        std::vector<bool> taintedForPre (n, false);

        for (size_t i = 0; i < n; ++i)
        {
            const auto op = nodes[i].inst.op;
            const bool touchesNonScalarState = nodes[i].isLoadInput
                                            || nodes[i].isStoreOutput
                                            || op == YdspIrOp::loadStateF || op == YdspIrOp::storeStateF
                                            || op == YdspIrOp::loadStateI || op == YdspIrOp::storeStateI
                                            || op == YdspIrOp::loadStateArrayF || op == YdspIrOp::storeStateArrayF
                                            || op == YdspIrOp::loadStateArrayI || op == YdspIrOp::storeStateArrayI
                                            || op == YdspIrOp::loadParamOut || op == YdspIrOp::storeParamOut
                                            || (! nodes[i].isVectorizableOp && ! nodes[i].readsInitialCarried && ! nodes[i].writesCarried);

            if (touchesNonScalarState)
                taintedForPre[i] = true;
        }

        for (bool prop = true; prop;)
        {
            prop = false;

            // Forward: any node depending on a tainted node is tainted.
            for (size_t i = 0; i < n; ++i)
            {
                if (taintedForPre[i])
                    continue;

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];
                    if (dep >= 0 && taintedForPre[static_cast<size_t> (dep)])
                    {
                        taintedForPre[i] = true;
                        prop = true;
                        break;
                    }
                }
            }

            // Carried register equivalence: if any reader/writer of carried reg
            // `v` is tainted, taint every node reading/writing `v`.
            std::vector<bool> taintedCarried (static_cast<size_t> (numValues), false);

            for (size_t i = 0; i < n; ++i)
            {
                if (! taintedForPre[i])
                    continue;

                if (nodes[i].writesCarried && nodes[i].inst.result >= 0)
                    taintedCarried[static_cast<size_t> (nodes[i].inst.result)] = true;

                for (int operand = 0; operand < 3; ++operand)
                {
                    if (! isValueIdOperand (nodes[i].inst.op, operand))
                        continue;

                    const int val = operand == 0 ? nodes[i].inst.a : (operand == 1 ? nodes[i].inst.b : nodes[i].inst.c);
                    if (val >= 0 && isCarried[static_cast<size_t> (val)])
                        taintedCarried[static_cast<size_t> (val)] = true;
                }
            }

            for (size_t i = 0; i < n; ++i)
            {
                if (taintedForPre[i])
                    continue;

                if (nodes[i].writesCarried && nodes[i].inst.result >= 0
                    && taintedCarried[static_cast<size_t> (nodes[i].inst.result)])
                {
                    taintedForPre[i] = true;
                    prop = true;
                    continue;
                }

                for (int operand = 0; operand < 3; ++operand)
                {
                    if (! isValueIdOperand (nodes[i].inst.op, operand))
                        continue;

                    const int val = operand == 0 ? nodes[i].inst.a : (operand == 1 ? nodes[i].inst.b : nodes[i].inst.c);
                    if (val >= 0 && taintedCarried[static_cast<size_t> (val)])
                    {
                        taintedForPre[i] = true;
                        prop = true;
                        break;
                    }
                }
            }
        }

        // StagePre = untainted stateful nodes + their untainted predecessors.
        std::vector<bool> inPre (n, false);
        bool hasPreStateful = false;

        for (size_t i = n; i-- > 0;)
        {
            if (taintedForPre[i])
                continue;

            if (nodes[i].isStatefulOp)
            {
                inPre[i] = true;
                hasPreStateful = true;
            }

            if (! inPre[i])
                continue;

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = nodes[i].opNode[operand];
                if (dep >= 0 && ! taintedForPre[static_cast<size_t> (dep)])
                    inPre[static_cast<size_t> (dep)] = true;
            }
        }

        if (! hasPreStateful)
            return false;

        // StageMid = untainted vectorizable stateless nodes that depend on StagePre.
        std::vector<bool> inMid (n, false);
        std::vector<bool> dependsOnPre (n, false);

        for (size_t i = 0; i < n; ++i)
        {
            if (inPre[i])
            {
                dependsOnPre[i] = true;
                continue;
            }

            for (int operand = 0; operand < 3; ++operand)
            {
                const int dep = nodes[i].opNode[operand];
                if (dep >= 0 && dependsOnPre[static_cast<size_t> (dep)])
                    dependsOnPre[i] = true;
            }

            if (! taintedForPre[i] && dependsOnPre[i] && nodes[i].isVectorizableOp && ! nodes[i].isStatefulOp && ! nodes[i].isStoreOutput)
                inMid[i] = true;
        }

        // Any node in StageMid whose operand is NOT in StagePre/StageMid and not constant must move to StagePost.
        for (bool changed = true; changed;)
        {
            changed = false;

            for (size_t i = 0; i < n; ++i)
            {
                if (! inMid[i])
                    continue;

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];
                    if (dep < 0)
                        continue;

                    const auto d = static_cast<size_t> (dep);
                    if (! inPre[d] && ! inMid[d] && (nodes[d].dependsOnPerSample || nodes[d].isStatefulOp))
                    {
                        inMid[i] = false;
                        changed = true;
                        break;
                    }
                }
            }
        }

        const auto computeBoundary = [&] (const std::vector<bool>& srcMask, const std::vector<bool>& dstMask)
        {
            std::vector<bool> exported (n, false);

            for (size_t i = 0; i < n; ++i)
            {
                if (! dstMask[i])
                    continue;

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];
                    if (dep >= 0 && srcMask[static_cast<size_t> (dep)])
                        exported[static_cast<size_t> (dep)] = true;
                }
            }

            std::vector<int> list;
            for (size_t i = 0; i < n; ++i)
                if (exported[i])
                    list.push_back (static_cast<int> (i));

            return list;
        };

        // Trim non-transcendental exports of StageMid to StagePost (e.g. a1/a2/a3
        // computed from `g`) so the StageMid->StagePost boundary stays <= 2 scalars.
        std::vector<bool> inPost (n, false);
        for (size_t i = 0; i < n; ++i)
            inPost[i] = ! inPre[i] && ! inMid[i];

        auto midExports = computeBoundary (inMid, inPost);

        while (midExports.size() > maxBoundaryScalarsPerStage)
        {
            int peelIdx = -1;

            for (auto it = midExports.rbegin(); it != midExports.rend(); ++it)
            {
                if (! nodes[static_cast<size_t> (*it)].isVectorizableTranscendental)
                {
                    peelIdx = *it;
                    break;
                }
            }

            if (peelIdx < 0)
                break;

            inMid[static_cast<size_t> (peelIdx)] = false;
            inPost[static_cast<size_t> (peelIdx)] = true;

            for (size_t i = static_cast<size_t> (peelIdx) + 1; i < n; ++i)
            {
                if (! inMid[i])
                    continue;

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];
                    if (dep >= 0 && inPost[static_cast<size_t> (dep)])
                    {
                        inMid[i] = false;
                        inPost[i] = true;
                        break;
                    }
                }
            }

            midExports = computeBoundary (inMid, inPost);
        }

        std::vector<bool> notPre (n, false);
        for (size_t i = 0; i < n; ++i)
            notPre[i] = ! inPre[i];

        auto preExports = computeBoundary (inPre, notPre);

        int midTranscendentals = 0;
        int midFloatOps = 0;
        for (size_t i = 0; i < n; ++i)
        {
            if (! inMid[i])
                continue;

            if (nodes[i].isVectorizableTranscendental)
                ++midTranscendentals;
            else if (nodes[i].isVectorizableFloatOp)
                ++midFloatOps;
        }

        int totalStores = 0;
        int postStores = 0;
        for (size_t i = 0; i < n; ++i)
        {
            if (nodes[i].isStoreOutput)
            {
                ++totalStores;
                if (inPost[i])
                    ++postStores;
            }
        }

        const auto allF32 = [&] (const std::vector<int>& ids)
        {
            return std::all_of (ids.begin(), ids.end(), [&] (int idx)
            {
                const int r = nodes[static_cast<size_t> (idx)].inst.result;
                return r >= 0 && static_cast<size_t> (r) < fn.valueTypes.size()
                    && fn.valueTypes[static_cast<size_t> (r)] == YdspValueType::float32Type;
            });
        };

        if (preExports.empty() || preExports.size() > maxBoundaryScalarsPerStage || ! allF32 (preExports))
            return false;

        if (midExports.empty() || midExports.size() > maxBoundaryScalarsPerStage || ! allF32 (midExports))
            return false;

        if (totalStores == 0 || postStores != totalStores)
            return false;

        if (midTranscendentals < 1 && midFloatOps < minVectorizableFloatOps)
            return false;

        inStage0 = std::move (inPre);
        inStage2 = std::move (inPost);
        stage0Exports = std::move (preExports);
        stage2Imports = std::move (midExports);
        useStage0 = true;
        useStage2 = true;
        return true;
    }

    int newValue (YdspValueType type)
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
            fn.valueLanes.push_back (1);

        return id;
    }

    static void setOperand (YdspIrInst& inst, int operand, int value) noexcept
    {
        if (operand == 0)
            inst.a = value;
        else if (operand == 1)
            inst.b = value;
        else
            inst.c = value;
    }

    int emitConstI (std::vector<YdspIrInst>& out, int64_t value, YdspValueType type = YdspValueType::int32Type)
    {
        const int id = newValue (type);
        out.push_back ({ YdspIrOp::constI, id, -1, -1, -1, -1, 0.0, value });
        return id;
    }

    void rewrite (const YdspIrLoop& origLoop, int blockSizeVal)
    {
        const size_t n = nodes.size();

        // Allocate scratch buffers (64 float32 elements each, 16-element aligned base).
        if (fn.float32ArrayElements % 16 != 0)
            fn.float32ArrayElements += 16 - (fn.float32ArrayElements % 16);

        std::vector<int> scratchBase (n, -1);

        for (const int nodeIdx : stage0Exports)
        {
            scratchBase[static_cast<size_t> (nodeIdx)] = fn.float32ArrayElements;
            fn.float32ArrayElements += chunkSize;
        }

        for (const int nodeIdx : stage2Imports)
        {
            scratchBase[static_cast<size_t> (nodeIdx)] = fn.float32ArrayElements;
            fn.float32ArrayElements += chunkSize;
        }

        // Preheader (block 0): compute chunkEnd, optional stream overlap guard, and pos = 0.
        auto& preInsts = fn.blocks[0].insts;
        fn.blocks[0].term = YdspIrTerm::fallthrough;
        fn.blocks[0].termTarget = -1;
        fn.blocks[0].termTarget2 = -1;

        const int zeroI = emitConstI (preInsts, 0);
        const int oneI = emitConstI (preInsts, 1);
        const int chunkConst = emitConstI (preInsts, chunkSize);
        const int mask63 = emitConstI (preInsts, chunkSize - 1);

        const int rem = newValue (YdspValueType::int32Type);
        preInsts.push_back ({ YdspIrOp::andI, rem, blockSizeVal, mask63 });

        const int aligned = newValue (YdspValueType::int32Type);
        preInsts.push_back ({ YdspIrOp::subI, aligned, blockSizeVal, rem });

        const int tooSmall = newValue (YdspValueType::boolType);
        preInsts.push_back ({ YdspIrOp::ltI, tooSmall, blockSizeVal, chunkConst });

        const int chunkEnd = newValue (YdspValueType::int32Type);
        preInsts.push_back ({ YdspIrOp::selectB, chunkEnd, tooSmall, zeroI, aligned });

        // Guard the chunked outer loop if any stream store can overlap another
        // accessed stream within one chunk (excluding exact in-place alias dist == 0).
        std::vector<StreamRef> stores, accesses;

        const auto addUniqueStream = [] (std::vector<StreamRef>& list, StreamRef stream)
        {
            if (std::find (list.begin(), list.end(), stream) == list.end())
                list.push_back (stream);
        };

        for (const auto& node : nodes)
        {
            if (node.isLoadInput)
                addUniqueStream (accesses, { true, node.inst.memIndex });
            else if (node.isStoreOutput)
            {
                addUniqueStream (accesses, { false, node.inst.memIndex });
                addUniqueStream (stores, { false, node.inst.memIndex });
            }
        }

        std::vector<std::pair<StreamRef, StreamRef>> overlapPairs;

        for (const auto& store : stores)
            for (const auto& other : accesses)
                if (! (store == other)
                    && std::find (overlapPairs.begin(), overlapPairs.end(), std::make_pair (other, store)) == overlapPairs.end())
                    overlapPairs.emplace_back (store, other);

        int effectiveEnd = chunkEnd;

        if (! overlapPairs.empty())
        {
            std::vector<std::pair<StreamRef, int>> addresses;

            const auto addrOf = [&] (StreamRef stream)
            {
                for (const auto& [known, val] : addresses)
                    if (known == stream)
                        return val;

                const int val = newValue (YdspValueType::int64Type);
                preInsts.push_back ({ stream.isInput ? YdspIrOp::loadInputAddress : YdspIrOp::loadOutputAddress,
                                      val, -1, -1, -1, stream.slot });
                addresses.emplace_back (stream, val);
                return val;
            };

            constexpr int64_t chunkBytes = static_cast<int64_t> (chunkSize) * 4;
            const int zero64 = emitConstI (preInsts, 0, YdspValueType::int64Type);
            const int bias64 = emitConstI (preInsts, chunkBytes - 1, YdspValueType::int64Type);
            const int window64 = emitConstI (preInsts, 2 * chunkBytes - 1, YdspValueType::int64Type);

            int anyOverlap = -1;

            for (const auto& [first, second] : overlapPairs)
            {
                const int dist = newValue (YdspValueType::int64Type);
                preInsts.push_back ({ YdspIrOp::subI, dist, addrOf (first), addrOf (second) });

                const int shifted = newValue (YdspValueType::int64Type);
                preInsts.push_back ({ YdspIrOp::addI, shifted, dist, bias64 });

                const int inWindow = newValue (YdspValueType::boolType);
                preInsts.push_back ({ YdspIrOp::ltUI, inWindow, shifted, window64 });

                const int nonZero = newValue (YdspValueType::boolType);
                preInsts.push_back ({ YdspIrOp::neI, nonZero, dist, zero64 });

                const int pair = newValue (YdspValueType::boolType);
                preInsts.push_back ({ YdspIrOp::andB, pair, inWindow, nonZero });

                if (anyOverlap < 0)
                {
                    anyOverlap = pair;
                }
                else
                {
                    const int combined = newValue (YdspValueType::boolType);
                    preInsts.push_back ({ YdspIrOp::orB, combined, anyOverlap, pair });
                    anyOverlap = combined;
                }
            }

            effectiveEnd = newValue (YdspValueType::int32Type);
            preInsts.push_back ({ YdspIrOp::selectB, effectiveEnd, anyOverlap, zeroI, chunkEnd });
        }

        const int pos = newValue (YdspValueType::int32Type);
        preInsts.push_back ({ YdspIrOp::movI, pos, zeroI });

        // Build replacement blocks starting at block index 1.
        std::vector<YdspIrBlock> newBlocks;
        std::vector<YdspIrLoop> newLoops;

        const int outerHeaderBlock = 1;
        newBlocks.emplace_back(); // placeholder for outerHeader (block 1)

        newLoops.push_back ({ 0, outerHeaderBlock, -1, pos, { YdspLoopBoundKind::blockSize, 0 }, false });

        const auto cloneConstantNode = [&] (int nodeIdx, std::vector<YdspIrInst>& dest, std::vector<int>& cache) -> int
        {
            auto self = [&] (auto& rec, int idx) -> int
            {
                if (cache[static_cast<size_t> (idx)] >= 0)
                    return cache[static_cast<size_t> (idx)];

                auto copy = nodes[static_cast<size_t> (idx)].inst;

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[static_cast<size_t> (idx)].opNode[operand];

                    if (dep >= 0)
                        setOperand (copy, operand, rec (rec, dep));
                }

                if (copy.result >= 0)
                    copy.result = newValue (fn.valueTypes[static_cast<size_t> (copy.result)]);

                dest.push_back (copy);
                cache[static_cast<size_t> (idx)] = copy.result;
                return copy.result;
            };

            return self (self, nodeIdx);
        };

        const auto addStageLoop = [&] (int k, const std::vector<YdspIrInst>& stageInsts)
        {
            const int preIdx = 1 + static_cast<int> (newBlocks.size());
            const int hdrIdx = preIdx + 1;
            const int bdyIdx = preIdx + 2;
            const int nxtIdx = preIdx + 3;

            YdspIrBlock stagePre;
            stagePre.insts.push_back ({ YdspIrOp::movI, k, zeroI });
            stagePre.term = YdspIrTerm::fallthrough;
            newBlocks.push_back (std::move (stagePre));

            YdspIrBlock stageHdr;
            const int cond = newValue (YdspValueType::boolType);
            stageHdr.insts.push_back ({ YdspIrOp::ltI, cond, k, chunkConst });
            stageHdr.term = YdspIrTerm::branchIf;
            stageHdr.termCond = cond;
            stageHdr.termTarget = bdyIdx;
            stageHdr.termTarget2 = nxtIdx;
            newBlocks.push_back (std::move (stageHdr));

            YdspIrBlock stageBdy;
            stageBdy.insts = stageInsts;
            const int kNext = newValue (YdspValueType::int32Type);
            stageBdy.insts.push_back ({ YdspIrOp::addI, kNext, k, oneI });
            stageBdy.insts.back().overflowFreeIntegerArithmetic = true;
            stageBdy.insts.push_back ({ YdspIrOp::movI, k, kNext });
            stageBdy.term = YdspIrTerm::branch;
            stageBdy.termTarget = hdrIdx;
            newBlocks.push_back (std::move (stageBdy));

            YdspIrLoop stageLoop;
            stageLoop.id = static_cast<int> (newLoops.size());
            stageLoop.headerBlock = hdrIdx;
            stageLoop.exitBlock = nxtIdx;
            stageLoop.induction = k;
            stageLoop.bound = { YdspLoopBoundKind::constant, chunkSize };
            newLoops.push_back (stageLoop);
        };

        // ---- Stage 0 (optional pre-stateful stateless loop) ----
        if (useStage0)
        {
            const int k0 = newValue (YdspValueType::int32Type);
            std::vector<YdspIrInst> insts;
            std::vector<int> valMap (n, -1);

            bool needsStreamIdx = false;

            for (size_t i = 0; i < n; ++i)
                if (inStage0[i] && nodes[i].isLoadInput)
                    needsStreamIdx = true;

            int streamIdx = -1;

            if (needsStreamIdx)
            {
                streamIdx = newValue (YdspValueType::int32Type);
                insts.push_back ({ YdspIrOp::addI, streamIdx, pos, k0 });
            }

            for (size_t i = 0; i < n; ++i)
            {
                if (! inStage0[i])
                    continue;

                auto copy = nodes[i].inst;

                if (nodes[i].isLoadInput)
                {
                    copy.a = streamIdx;
                }
                else
                {
                    for (int operand = 0; operand < 3; ++operand)
                    {
                        const int dep = nodes[i].opNode[operand];

                        if (dep < 0)
                            continue;

                        const int mapped = inStage0[static_cast<size_t> (dep)]
                                             ? valMap[static_cast<size_t> (dep)]
                                             : cloneConstantNode (dep, insts, valMap);
                        setOperand (copy, operand, mapped);
                    }
                }

                if (copy.result >= 0 && ! nodes[i].writesCarried)
                    copy.result = newValue (fn.valueTypes[static_cast<size_t> (copy.result)]);

                valMap[i] = copy.result;
                insts.push_back (copy);

                if (scratchBase[i] >= 0)
                    insts.push_back ({ YdspIrOp::storeStateArrayF, -1, k0, copy.result, -1, scratchBase[i] });
            }

            addStageLoop (k0, insts);
        }

        // ---- Stage 1 (stateful core loop) ----
        {
            const int k1 = newValue (YdspValueType::int32Type);
            std::vector<YdspIrInst> insts;
            std::vector<int> valMap (n, -1);

            bool needsStreamIdx = false;

            for (size_t i = 0; i < n; ++i)
            {
                if (inStage0[i] || inStage2[i])
                    continue;

                if (nodes[i].isLoadInput || nodes[i].isStoreOutput)
                {
                    needsStreamIdx = true;
                    break;
                }

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];

                    if (dep >= 0 && inStage0[static_cast<size_t> (dep)] && nodes[static_cast<size_t> (dep)].isLoadInput)
                        needsStreamIdx = true;
                }
            }

            int streamIdx = -1;

            if (needsStreamIdx)
            {
                streamIdx = newValue (YdspValueType::int32Type);
                insts.push_back ({ YdspIrOp::addI, streamIdx, pos, k1 });
            }

            // Load Stage 0 exports needed by Stage 1.
            for (const int expIdx : stage0Exports)
            {
                bool usedInStage1 = false;

                for (size_t i = 0; i < n && ! usedInStage1; ++i)
                {
                    if (inStage0[i] || inStage2[i])
                        continue;

                    for (int operand = 0; operand < 3; ++operand)
                        if (nodes[i].opNode[operand] == expIdx)
                            usedInStage1 = true;
                }

                if (usedInStage1)
                {
                    const int loaded = newValue (YdspValueType::float32Type);
                    insts.push_back ({ YdspIrOp::loadStateArrayF, loaded, k1, -1, -1, scratchBase[static_cast<size_t> (expIdx)] });
                    valMap[static_cast<size_t> (expIdx)] = loaded;
                }
            }

            for (size_t i = 0; i < n; ++i)
            {
                if (inStage0[i] || inStage2[i])
                    continue;

                auto copy = nodes[i].inst;

                if (nodes[i].isLoadInput)
                {
                    copy.a = streamIdx;
                }
                else
                {
                    if (nodes[i].isStoreOutput)
                        copy.a = streamIdx;

                    for (int operand = 0; operand < 3; ++operand)
                    {
                        const int dep = nodes[i].opNode[operand];

                        if (dep < 0)
                            continue;

                        const auto depIdx = static_cast<size_t> (dep);

                        if (valMap[depIdx] < 0)
                        {
                            if (inStage0[depIdx] && nodes[depIdx].isLoadInput)
                            {
                                auto loadCopy = nodes[depIdx].inst;
                                loadCopy.a = streamIdx;
                                loadCopy.result = newValue (YdspValueType::float32Type);
                                insts.push_back (loadCopy);
                                valMap[depIdx] = loadCopy.result;
                            }
                            else
                            {
                                cloneConstantNode (dep, insts, valMap);
                            }
                        }

                        setOperand (copy, operand, valMap[depIdx]);
                    }
                }

                if (copy.result >= 0 && ! nodes[i].writesCarried)
                    copy.result = newValue (fn.valueTypes[static_cast<size_t> (copy.result)]);

                valMap[i] = copy.result;
                insts.push_back (copy);

                if (scratchBase[i] >= 0)
                    insts.push_back ({ YdspIrOp::storeStateArrayF, -1, k1, copy.result, -1, scratchBase[i] });
            }

            addStageLoop (k1, insts);
        }

        // ---- Stage 2 (optional post-stateful stateless loop) ----
        if (useStage2)
        {
            const int k2 = newValue (YdspValueType::int32Type);
            std::vector<YdspIrInst> insts;
            std::vector<int> valMap (n, -1);

            bool needsStreamIdx = false;

            for (size_t i = 0; i < n; ++i)
            {
                if (! inStage2[i])
                    continue;

                if (nodes[i].isLoadInput || nodes[i].isStoreOutput)
                {
                    needsStreamIdx = true;
                    break;
                }

                for (int operand = 0; operand < 3; ++operand)
                {
                    const int dep = nodes[i].opNode[operand];

                    if (dep >= 0 && ! inStage2[static_cast<size_t> (dep)] && nodes[static_cast<size_t> (dep)].isLoadInput)
                        needsStreamIdx = true;
                }
            }

            int streamIdx = -1;

            if (needsStreamIdx)
            {
                streamIdx = newValue (YdspValueType::int32Type);
                insts.push_back ({ YdspIrOp::addI, streamIdx, pos, k2 });
            }

            // Load Stage 0 exports needed by Stage 2, and Stage 2 imports from Stage 1.
            for (const int expIdx : stage0Exports)
            {
                bool usedInStage2 = false;

                for (size_t i = 0; i < n && ! usedInStage2; ++i)
                {
                    if (! inStage2[i])
                        continue;

                    for (int operand = 0; operand < 3; ++operand)
                        if (nodes[i].opNode[operand] == expIdx)
                            usedInStage2 = true;
                }

                if (usedInStage2)
                {
                    const int loaded = newValue (YdspValueType::float32Type);
                    insts.push_back ({ YdspIrOp::loadStateArrayF, loaded, k2, -1, -1, scratchBase[static_cast<size_t> (expIdx)] });
                    valMap[static_cast<size_t> (expIdx)] = loaded;
                }
            }

            for (const int impIdx : stage2Imports)
            {
                const int loaded = newValue (YdspValueType::float32Type);
                insts.push_back ({ YdspIrOp::loadStateArrayF, loaded, k2, -1, -1, scratchBase[static_cast<size_t> (impIdx)] });
                valMap[static_cast<size_t> (impIdx)] = loaded;
            }

            for (size_t i = 0; i < n; ++i)
            {
                if (! inStage2[i])
                    continue;

                auto copy = nodes[i].inst;

                if (nodes[i].isLoadInput)
                {
                    copy.a = streamIdx;
                }
                else
                {
                    if (nodes[i].isStoreOutput)
                        copy.a = streamIdx;

                    for (int operand = 0; operand < 3; ++operand)
                    {
                        const int dep = nodes[i].opNode[operand];

                        if (dep < 0)
                            continue;

                        const auto depIdx = static_cast<size_t> (dep);

                        if (valMap[depIdx] < 0)
                        {
                            if (! inStage2[depIdx] && nodes[depIdx].isLoadInput)
                            {
                                auto loadCopy = nodes[depIdx].inst;
                                loadCopy.a = streamIdx;
                                loadCopy.result = newValue (YdspValueType::float32Type);
                                insts.push_back (loadCopy);
                                valMap[depIdx] = loadCopy.result;
                            }
                            else
                            {
                                cloneConstantNode (dep, insts, valMap);
                            }
                        }

                        setOperand (copy, operand, valMap[depIdx]);
                    }
                }

                if (copy.result >= 0 && ! nodes[i].writesCarried)
                    copy.result = newValue (fn.valueTypes[static_cast<size_t> (copy.result)]);

                valMap[i] = copy.result;
                insts.push_back (copy);
            }

            addStageLoop (k2, insts);
        }

        // ---- Outer latch ----
        {
            YdspIrBlock outerLatch;
            const int posNext = newValue (YdspValueType::int32Type);
            outerLatch.insts.push_back ({ YdspIrOp::addI, posNext, pos, chunkConst });
            outerLatch.insts.back().overflowFreeIntegerArithmetic = true;
            outerLatch.insts.push_back ({ YdspIrOp::movI, pos, posNext });
            outerLatch.term = YdspIrTerm::branch;
            outerLatch.termTarget = outerHeaderBlock;
            newBlocks.push_back (std::move (outerLatch));
        }

        const int tailPreheaderBlock = 1 + static_cast<int> (newBlocks.size());
        const int tailHeaderBlock = tailPreheaderBlock + 1;
        const int tailBodyBlock = tailPreheaderBlock + 2;
        const int newExitBlock = tailPreheaderBlock + 3;

        // Fill outerHeader now that tailPreheaderBlock is known.
        newLoops[0].exitBlock = tailPreheaderBlock;
        {
            auto& outerHdr = newBlocks[0];
            const int outerCond = newValue (YdspValueType::boolType);
            outerHdr.insts.push_back ({ YdspIrOp::ltI, outerCond, pos, effectiveEnd });
            outerHdr.term = YdspIrTerm::branchIf;
            outerHdr.termCond = outerCond;
            outerHdr.termTarget = outerHeaderBlock + 1;
            outerHdr.termTarget2 = tailPreheaderBlock;
        }

        // ---- Scalar remainder tail loop ----
        {
            YdspIrBlock tailPre;
            tailPre.insts.push_back ({ YdspIrOp::movI, origLoop.induction, pos });
            tailPre.term = YdspIrTerm::fallthrough;
            newBlocks.push_back (std::move (tailPre));

            auto tailHdr = std::move (fn.blocks[1]);
            tailHdr.termTarget = tailBodyBlock;
            tailHdr.termTarget2 = newExitBlock;
            newBlocks.push_back (std::move (tailHdr));

            auto tailBdy = std::move (fn.blocks[2]);
            tailBdy.term = YdspIrTerm::branch;
            tailBdy.termTarget = tailHeaderBlock;
            newBlocks.push_back (std::move (tailBdy));

            YdspIrLoop tailLoop;
            tailLoop.id = static_cast<int> (newLoops.size());
            tailLoop.headerBlock = tailHeaderBlock;
            tailLoop.exitBlock = newExitBlock;
            tailLoop.induction = origLoop.induction;
            tailLoop.bound = { YdspLoopBoundKind::blockSize, 0 };
            newLoops.push_back (tailLoop);
        }

        // Splice newBlocks in place of old blocks [1, 3) and rebase old blocks >= 3.
        const int delta = newExitBlock - 3;

        for (size_t b = 3; b < fn.blocks.size(); ++b)
        {
            auto& block = fn.blocks[b];

            if (block.termTarget >= 3)
                block.termTarget += delta;

            if (block.termTarget2 >= 3)
                block.termTarget2 += delta;
        }

        fn.blocks.erase (fn.blocks.begin() + 1, fn.blocks.begin() + 3);
        fn.blocks.insert (fn.blocks.begin() + 1,
                          std::make_move_iterator (newBlocks.begin()),
                          std::make_move_iterator (newBlocks.end()));

        fn.loops = std::move (newLoops);
    }

    YdspIrFunction& fn;
    int numValues = 0;

    std::vector<bool> isCarried;
    std::vector<Node> nodes;
    std::vector<bool> inStage0;
    std::vector<bool> inStage2;
    std::vector<int> stage0Exports;
    std::vector<int> stage2Imports;
    bool useStage0 = false;
    bool useStage2 = false;
};

} // namespace

//==============================================================================

bool YdspOptimizer::distributeSampleLoop (YdspIrFunction& fn)
{
    return SampleLoopDistributor (fn).run();
}

} // namespace yup
