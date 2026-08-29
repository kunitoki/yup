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

//==============================================================================

template <typename Callback>
void forEachValueOperand (const YdspIrInst& inst, Callback&& callback)
{
    switch (inst.op)
    {
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
            return;

        default:
            break;
    }

    if (inst.a >= 0)
        callback (inst.a);

    if (inst.b >= 0)
        callback (inst.b);

    if (inst.c >= 0)
        callback (inst.c);
}

bool isPackedFloatOp (YdspIrOp op) noexcept
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
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::movF:
            return true;

        default:
            return false;
    }
}

bool isStateArrayAccess (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::storeStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::storeStateArrayI:
            return true;

        default:
            return false;
    }
}

bool isStreamAccess (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::storeOutput:
            return true;

        default:
            return false;
    }
}

//==============================================================================
/** Widens one loop, or leaves it exactly as it was.

    Analysis never mutates the function: every legality test runs first and only
    a fully-proven loop reaches rewrite(), so a rejected loop costs nothing but
    the analysis.
*/
class LoopWidener
{
public:
    LoopWidener (YdspIrFunction& functionToWiden, const YdspIrLoop& loopToWiden, int width)
        : fn (functionToWiden)
        , loop (loopToWiden)
        , lanes (width)
    {
    }

    bool tryWiden (YdspVectorizationReason& reason)
    {
        if (! analyse())
        {
            reason = rejectionReason;
            return false;
        }

        rewrite();
        return true;
    }

private:
    /** Records why the loop was rejected; the only way a legality test may
        return false. */
    bool fail (YdspVectorizationReason reason) noexcept
    {
        rejectionReason = reason;
        return false;
    }

    /** One recognised `acc = acc + x` accumulation in the loop body. */
    struct Reduction
    {
        int accumulator = -1; // the scalar the loop accumulates into
        int addIndex = -1;    // body index of the accumulating addF
        int moveIndex = -1;   // body index of the `movF acc, sum`
        int addend = -1;      // the add's non-accumulator operand
        int vectorAcc = -1;   // the vector accumulator (filled in by the rewrite)
    };

    //==============================================================================
    // Analysis

    bool analyse()
    {
        if (loop.bound.kind != YdspLoopBoundKind::constant
            && loop.bound.kind != YdspLoopBoundKind::blockSize)
        {
            return fail (YdspVectorizationReason::unsupportedLoopBound);
        }

        header = loop.headerBlock;
        body = header + 1;
        exit = loop.exitBlock;
        preheader = header - 1;

        if (preheader < 0 || body < 0 || exit != body + 1)
            return fail (YdspVectorizationReason::multiBlockBody); // a multi-block (or nested) body

        if (static_cast<size_t> (exit) >= fn.blocks.size())
            return fail (YdspVectorizationReason::multiBlockBody);

        const auto& bodyBlock = fn.blocks[static_cast<size_t> (body)];

        if (bodyBlock.term != YdspIrTerm::branch || bodyBlock.termTarget != header)
            return fail (YdspVectorizationReason::multiBlockBody);

        numValues = static_cast<int> (fn.valueTypes.size());

        if (numValues <= 0)
            return fail (YdspVectorizationReason::notVectorizable);

        defsInBody.assign (static_cast<size_t> (numValues), 0);
        defsElsewhere.assign (static_cast<size_t> (numValues), 0);
        usesInBody.assign (static_cast<size_t> (numValues), 0);
        usesElsewhere.assign (static_cast<size_t> (numValues), 0);
        definedInHeader.assign (static_cast<size_t> (numValues), false);

        if (! countDefsAndUses())
            return fail (YdspVectorizationReason::notVectorizable);

        if (loop.bound.kind == YdspLoopBoundKind::constant)
        {
            if (loop.bound.constant <= 0)
                return fail (YdspVectorizationReason::unsupportedLoopBound);

            if (! findLoopStart())
                return fail (YdspVectorizationReason::nonConstantStart);

            const auto span = loop.bound.constant - start;

            if (span < lanes)
                return fail (YdspVectorizationReason::shortTripCount); // no full vector trip: the loop is all tail

            tail = span % lanes;
        }
        else
        {
            if (! findHeaderCompare())
                return fail (YdspVectorizationReason::missingHeaderCompare);

            if (! findLoopStart())
                return fail (YdspVectorizationReason::nonConstantStart);

            if (start != 0)
                return fail (YdspVectorizationReason::nonzeroStart);
        }

        if (! findInductionUpdate())
            return fail (YdspVectorizationReason::unrecognizedInductionUpdate);

        if (! checkInductionUses())
            return fail (YdspVectorizationReason::inductionUsedAsValue);

        if (! findReductions())
            return fail (YdspVectorizationReason::unrecognizedAccumulation);

        if (! classifyBody())
            return false; // rejectionReason was set by the failing case

        if (! checkSingleDefinitionAndLiveOut())
            return fail (YdspVectorizationReason::loopCarriedValue);

        if (! checkTailFeasibility())
            return fail (YdspVectorizationReason::runtimeBoundWithoutStreams);

        if (widenedAccessCount == 0)
            return fail (YdspVectorizationReason::nothingToWiden);

        return true;
    }

    bool findLoopStart()
    {
        for (const auto& inst : fn.blocks[static_cast<size_t> (preheader)].insts)
            if (inst.op == YdspIrOp::movI && inst.result == loop.induction && inst.a >= 0)
                return readConstantInt (inst.a, start);

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            if (static_cast<int> (b) == body)
                continue;

            for (const auto& inst : fn.blocks[b].insts)
                if (inst.result == loop.induction && inst.op == YdspIrOp::constI)
                {
                    start = static_cast<int> (inst.ivalue);
                    return true;
                }
        }

        return false;
    }

    bool readConstantInt (int value, int& out) const
    {
        if (value < 0 || value >= numValues)
            return false;

        if (defsInBody[static_cast<size_t> (value)] + defsElsewhere[static_cast<size_t> (value)] != 1)
            return false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result == value)
                {
                    if (inst.op != YdspIrOp::constI)
                        return false;

                    out = static_cast<int> (inst.ivalue);
                    return true;
                }
            }
        }

        return false;
    }

    bool findHeaderCompare()
    {
        int matches = 0;

        for (const auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == loop.induction && inst.b >= 0)
                ++matches;

        return matches == 1;
    }

    bool checkTailFeasibility() const
    {
        if (loop.bound.kind == YdspLoopBoundKind::blockSize && ! hasStreamAccess)
            return false;

        return true;
    }

    bool countDefsAndUses()
    {
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const bool inBody = (static_cast<int> (b) == body);
            const auto& block = fn.blocks[b];

            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0)
                {
                    if (inst.result >= numValues)
                        return false;

                    ++(inBody ? defsInBody : defsElsewhere)[static_cast<size_t> (inst.result)];

                    if (static_cast<int> (b) == header)
                        definedInHeader[static_cast<size_t> (inst.result)] = true;
                }

                bool operandsValid = true;

                forEachValueOperand (inst, [&] (int value)
                {
                    if (value >= numValues)
                    {
                        operandsValid = false;
                        return;
                    }

                    ++(inBody ? usesInBody : usesElsewhere)[static_cast<size_t> (value)];
                });

                if (! operandsValid)
                    return false;
            }

            if (block.termCond >= 0)
            {
                if (block.termCond >= numValues)
                    return false;

                ++(inBody ? usesInBody : usesElsewhere)[static_cast<size_t> (block.termCond)];
            }
        }

        return true;
    }

    bool findInductionUpdate()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        if (insts.size() < 2)
            return false;

        const auto last = static_cast<int> (insts.size()) - 1;
        const auto& move = insts[static_cast<size_t> (last)];

        if (move.op != YdspIrOp::movI || move.result != loop.induction || move.a < 0)
            return false;

        inductionMoveIndex = last;

        for (int j = last; j-- > 0;)
        {
            if (insts[static_cast<size_t> (j)].result == move.a)
            {
                incrementIndex = j;
                break;
            }
        }

        if (incrementIndex < 0)
            return false;

        const auto& increment = insts[static_cast<size_t> (incrementIndex)];

        if (increment.op != YdspIrOp::addI || increment.a != loop.induction)
            return false;

        if (! isConstantInt (increment.b, 1))
            return false;

        const auto step = static_cast<size_t> (increment.result);

        return defsInBody[step] == 1 && defsElsewhere[step] == 0
            && usesInBody[step] == 1 && usesElsewhere[step] == 0;
    }

    bool isConstantInt (int value, int64_t expected) const
    {
        if (value < 0 || value >= numValues)
            return false;

        if (defsInBody[static_cast<size_t> (value)] + defsElsewhere[static_cast<size_t> (value)] != 1)
            return false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result == value)
                    return inst.op == YdspIrOp::constI && inst.ivalue == expected;
            }
        }

        return false;
    }

    bool checkInductionUses() const
    {
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];

            for (size_t j = 0; j < block.insts.size(); ++j)
            {
                const auto& inst = block.insts[j];

                bool readsInduction = false;

                forEachValueOperand (inst, [&] (int value)
                {
                    if (value == loop.induction)
                        readsInduction = true;
                });

                if (! readsInduction)
                    continue;

                if (static_cast<int> (b) == header)
                    continue;

                if (static_cast<int> (b) != body)
                    return false;

                if (static_cast<int> (j) == incrementIndex)
                    continue;

                const bool indexOnly = (isStateArrayAccess (inst.op) || isStreamAccess (inst.op))
                                    && inst.a == loop.induction
                                    && inst.b != loop.induction
                                    && inst.c != loop.induction;

                if (! indexOnly)
                    return false;
            }

            if (block.termCond == loop.induction && static_cast<int> (b) != header)
                return false;
        }

        return true;
    }

    bool findReductions()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& move = insts[j];

            if (move.op != YdspIrOp::movF || move.result < 0)
                continue;

            const auto accumulator = static_cast<size_t> (move.result);

            if (defsElsewhere[accumulator] == 0 && usesElsewhere[accumulator] == 0)
                continue;

            if (defsElsewhere[accumulator] == 0 || defsInBody[accumulator] != 1)
                return false;

            if (fn.valueTypes[accumulator] != YdspValueType::float32Type)
                return false;

            int addIndex = -1;

            for (size_t k = j; k-- > 0;)
            {
                if (insts[k].result == move.a)
                {
                    addIndex = static_cast<int> (k);
                    break;
                }
            }

            if (addIndex < 0)
                return false;

            const auto& add = insts[static_cast<size_t> (addIndex)];

            if (add.op != YdspIrOp::addF)
                return false;

            const auto sum = static_cast<size_t> (add.result);

            if (defsInBody[sum] != 1 || defsElsewhere[sum] != 0
                || usesInBody[sum] != 1 || usesElsewhere[sum] != 0)
                return false;

            if (usesInBody[accumulator] != 1)
                return false;

            const auto addend = add.a == move.result ? add.b
                              : add.b == move.result ? add.a
                                                     : -1;

            if (addend < 0 || addend == move.result)
                return false;

            if (fn.valueTypes[static_cast<size_t> (addend)] != YdspValueType::float32Type
                || fn.valueTypes[sum] != YdspValueType::float32Type)
                return false;

            reductions.push_back ({ move.result, addIndex, static_cast<int> (j), addend, -1 });
        }

        return true;
    }

    bool classifyBody()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        isVector.assign (static_cast<size_t> (numValues), false);
        widened.assign (insts.size(), false);

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& inst = insts[j];

            if (static_cast<int> (j) == incrementIndex || static_cast<int> (j) == inductionMoveIndex)
                continue;

            if (const auto* reduction = reductionAt (static_cast<int> (j)))
            {
                if (reduction->addIndex == static_cast<int> (j))
                {
                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                }

                continue;
            }

            switch (inst.op)
            {
                case YdspIrOp::loadStateArrayF:
                {
                    if (inst.a == loop.induction)
                    {
                        if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                            return fail (YdspVectorizationReason::unsupportedWidenedType);

                        widened[j] = true;
                        isVector[static_cast<size_t> (inst.result)] = true;
                        ++widenedAccessCount;
                    }
                    else if (! isLoopInvariant (inst.a))
                    {
                        return fail (YdspVectorizationReason::indirectAccess);
                    }

                    break;
                }

                case YdspIrOp::storeStateArrayF:
                {
                    if (inst.a != loop.induction)
                        return fail (YdspVectorizationReason::indirectAccess);

                    if (inst.b < 0 || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    ++widenedAccessCount;
                    break;
                }

                case YdspIrOp::loadStateArrayI:
                {
                    if (! isLoopInvariant (inst.a))
                        return fail (YdspVectorizationReason::indirectAccess);

                    break;
                }

                case YdspIrOp::loadInput:
                case YdspIrOp::loadOutput:
                {
                    if (inst.a == loop.induction)
                    {
                        if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                            return fail (YdspVectorizationReason::unsupportedWidenedType);

                        widened[j] = true;
                        isVector[static_cast<size_t> (inst.result)] = true;
                        hasStreamAccess = true;
                        ++widenedAccessCount;
                    }
                    else if (! isLoopInvariant (inst.a))
                    {
                        return fail (YdspVectorizationReason::indirectAccess);
                    }

                    break;
                }

                case YdspIrOp::storeOutput:
                {
                    if (inst.a != loop.induction)
                        return fail (YdspVectorizationReason::invariantStreamStore);

                    if (inst.b < 0 || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    hasStreamAccess = true;
                    ++widenedAccessCount;
                    break;
                }

                case YdspIrOp::storeStateArrayI:
                case YdspIrOp::storeStateF:
                case YdspIrOp::storeStateI:
                case YdspIrOp::storeParam:
                case YdspIrOp::storeParamOut:
                case YdspIrOp::storeEventFieldF:
                case YdspIrOp::storeEventFieldI:
                    return fail (YdspVectorizationReason::stateWriteInBody);

                case YdspIrOp::emitEvent:
                    return fail (YdspVectorizationReason::emitInBody);

                default:
                {
                    bool readsVector = false;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (isVector[static_cast<size_t> (value)])
                            readsVector = true;
                    });

                    if (! readsVector)
                        break;

                    if (! isPackedFloatOp (inst.op) || inst.result < 0)
                        return fail (YdspVectorizationReason::unsupportedWidenedOp);

                    if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    bool operandsAreFloat32 = true;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (fn.valueTypes[static_cast<size_t> (value)] != YdspValueType::float32Type)
                            operandsAreFloat32 = false;
                    });

                    if (! operandsAreFloat32)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                    break;
                }
            }
        }

        return true;
    }

    bool checkSingleDefinitionAndLiveOut() const
    {
        for (const auto& inst : fn.blocks[static_cast<size_t> (body)].insts)
        {
            if (inst.result < 0)
                continue;

            const auto result = static_cast<size_t> (inst.result);

            if (defsInBody[result] != 1)
                return false;

            if (inst.result == loop.induction || isReductionAccumulator (inst.result))
                continue;

            if (defsElsewhere[result] != 0 || usesElsewhere[result] != 0)
                return false;
        }

        for (const auto& inst : fn.blocks[static_cast<size_t> (body)].insts)
        {
            bool available = true;

            forEachValueOperand (inst, [&] (int value)
            {
                if (defsInBody[static_cast<size_t> (value)] == 0
                    && definedInHeader[static_cast<size_t> (value)])
                    available = false;
            });

            if (! available)
                return false;
        }

        return true;
    }

    bool isLoopInvariant (int value) const
    {
        return value >= 0
            && value < numValues
            && value != loop.induction
            && defsInBody[static_cast<size_t> (value)] == 0;
    }

    //==============================================================================
    // Rewrite

    const Reduction* reductionAt (int bodyIndex) const
    {
        for (const auto& reduction : reductions)
        {
            if (reduction.addIndex == bodyIndex || reduction.moveIndex == bodyIndex)
                return &reduction;
        }

        return nullptr;
    }

    bool isReductionAccumulator (int value) const
    {
        for (const auto& reduction : reductions)
        {
            if (reduction.accumulator == value)
                return true;
        }

        return false;
    }

    int newValue (YdspValueType type, int laneCount)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (type);
        fn.valueLanes.push_back (laneCount);

        return id;
    }

    void planHoisting (const std::vector<YdspIrInst>& insts)
    {
        hoistToPreheader.assign (insts.size(), false);
        valueHoisted.assign (static_cast<size_t> (numValues), false);

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& inst = insts[j];

            if (widened[j]
                || static_cast<int> (j) == incrementIndex
                || static_cast<int> (j) == inductionMoveIndex
                || reductionAt (static_cast<int> (j)) != nullptr)
                continue;

            if (inst.result < 0
                || inst.op == YdspIrOp::loadStateArrayF
                || inst.op == YdspIrOp::loadStateArrayI)
                continue;

            bool operandsReady = true;

            forEachValueOperand (inst, [&] (int value)
            {
                if (defsInBody[static_cast<size_t> (value)] > 0 && ! valueHoisted[static_cast<size_t> (value)])
                    operandsReady = false;
            });

            if (! operandsReady)
                continue;

            hoistToPreheader[j] = true;
            valueHoisted[static_cast<size_t> (inst.result)] = true;
        }
    }

    void rewrite()
    {
        if (fn.valueLanes.size() < fn.valueTypes.size())
            fn.valueLanes.resize (fn.valueTypes.size(), 1);

        auto& preheaderInsts = fn.blocks[static_cast<size_t> (preheader)].insts;
        auto& bodyInsts = fn.blocks[static_cast<size_t> (body)].insts;

        const auto oldBody = bodyInsts;

        planHoisting (oldBody);

        const auto step = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::constI, step, -1, -1, -1, -1, 0.0, lanes });

        for (auto& reduction : reductions)
        {
            const auto zero = newValue (YdspValueType::float32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::constF, zero, -1, -1, -1, -1, 0.0 });

            reduction.vectorAcc = newValue (YdspValueType::float32Type, lanes);
            preheaderInsts.push_back ({ YdspIrOp::vsplat, reduction.vectorAcc, zero });
        }

        std::vector<YdspIrInst> newBody;
        newBody.reserve (oldBody.size() + 4);

        std::map<int, int> preheaderSplats;
        std::map<int, int> bodySplats;

        const auto splatOf = [&] (int scalar)
        {
            if (isVector[static_cast<size_t> (scalar)])
                return scalar;

            const bool definedInBody = defsInBody[static_cast<size_t> (scalar)] > 0
                                    && ! valueHoisted[static_cast<size_t> (scalar)];

            auto& cache = definedInBody ? bodySplats : preheaderSplats;

            if (const auto existing = cache.find (scalar); existing != cache.end())
                return existing->second;

            const auto splat = newValue (YdspValueType::float32Type, lanes);

            if (definedInBody)
                newBody.push_back ({ YdspIrOp::vsplat, splat, scalar });
            else
                preheaderInsts.push_back ({ YdspIrOp::vsplat, splat, scalar });

            cache[scalar] = splat;
            return splat;
        };

        const auto isFloatOperand = [this] (int value)
        {
            return value >= 0 && isFloatValueType (fn.valueTypes[static_cast<size_t> (value)]);
        };

        for (size_t j = 0; j < oldBody.size(); ++j)
        {
            auto inst = oldBody[j];

            if (hoistToPreheader[j])
            {
                preheaderInsts.push_back (inst);
                continue;
            }

            if (static_cast<int> (j) == incrementIndex)
            {
                inst.b = step;
                newBody.push_back (inst);
                continue;
            }

            if (const auto* reduction = reductionAt (static_cast<int> (j)))
            {
                if (reduction->addIndex == static_cast<int> (j))
                {
                    inst.a = reduction->vectorAcc;
                    inst.b = splatOf (reduction->addend);
                    fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;
                }
                else
                {
                    inst.result = reduction->vectorAcc;
                }

                newBody.push_back (inst);
                continue;
            }

            if (! widened[j])
            {
                newBody.push_back (inst);
                continue;
            }

            if (inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeOutput)
            {
                inst.b = splatOf (inst.b);
            }
            else if (inst.op != YdspIrOp::loadStateArrayF
                  && inst.op != YdspIrOp::loadInput
                  && inst.op != YdspIrOp::loadOutput)
            {
                if (isFloatOperand (inst.a))
                    inst.a = splatOf (inst.a);

                if (isFloatOperand (inst.b))
                    inst.b = splatOf (inst.b);

                if (isFloatOperand (inst.c))
                    inst.c = splatOf (inst.c);
            }

            if (inst.result >= 0)
                fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;

            newBody.push_back (inst);
        }

        bodyInsts = std::move (newBody);

        tailRemap.assign (static_cast<size_t> (numValues), -1);
        int foldBlock = exit;

        if (loop.bound.kind == YdspLoopBoundKind::constant)
        {
            if (tail > 0)
                emitPeel (oldBody);
        }
        else
        {
            foldBlock = emitRuntimeEpilogue (oldBody);
        }

        std::vector<YdspIrInst> reduceOnExit;

        for (const auto& reduction : reductions)
        {
            const auto reduced = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::vreduceAddF, reduced, reduction.vectorAcc });

            const auto total = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::addF, total, reduction.accumulator, reduced });
            reduceOnExit.push_back ({ YdspIrOp::movF, reduction.accumulator, total });
        }

        auto& exitInsts = fn.blocks[static_cast<size_t> (foldBlock)].insts;
        exitInsts.insert (exitInsts.begin(), reduceOnExit.begin(), reduceOnExit.end());
    }

    int tailId (int value)
    {
        if (value < 0)
            return -1;

        if (value == loop.induction || isReductionAccumulator (value))
            return value;

        const auto v = static_cast<size_t> (value);

        if (defsInBody[v] == 0 || valueHoisted[v])
            return value;

        auto& slot = tailRemap[v];

        if (slot < 0)
            slot = newValue (fn.valueTypes[v], 1);

        return slot;
    }

    void emitPeel (const std::vector<YdspIrInst>& oldBody)
    {
        auto& preheaderInsts = fn.blocks[static_cast<size_t> (preheader)].insts;

        const auto newStart = newValue (YdspValueType::int32Type, 1);
        bool rebased = false;

        for (size_t j = 0; j < preheaderInsts.size(); ++j)
        {
            if (preheaderInsts[j].op != YdspIrOp::movI || preheaderInsts[j].result != loop.induction)
                continue;

            preheaderInsts.insert (preheaderInsts.begin() + static_cast<ptrdiff_t> (j),
                                   YdspIrInst { YdspIrOp::constI, newStart, -1, -1, -1, -1, 0.0, start + tail });
            preheaderInsts[j + 1].a = newStart;
            rebased = true;
            break;
        }

        if (! rebased)
        {
            for (size_t b = 0; b < fn.blocks.size() && ! rebased; ++b)
            {
                if (static_cast<int> (b) == body)
                    continue;

                for (auto& inst : fn.blocks[b].insts)
                {
                    if (inst.result == loop.induction && inst.op == YdspIrOp::constI)
                    {
                        inst.ivalue = start + tail;
                        rebased = true;
                        break;
                    }
                }
            }
        }

        for (int k = 0; k < tail; ++k)
        {
            const auto index = newValue (YdspValueType::int32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::constI, index, -1, -1, -1, -1, 0.0, start + k });

            for (size_t j = 0; j < oldBody.size(); ++j)
            {
                if (static_cast<int> (j) == incrementIndex || static_cast<int> (j) == inductionMoveIndex)
                    continue;

                auto inst = oldBody[j];

                if (inst.result == loop.induction)
                    continue;

                if (inst.result >= 0)
                    inst.result = tailId (inst.result);

                const auto replaceOperand = [&] (int operand) -> int
                {
                    return operand == loop.induction ? index : tailId (operand);
                };

                inst.a = replaceOperand (inst.a);
                inst.b = replaceOperand (inst.b);
                inst.c = replaceOperand (inst.c);

                preheaderInsts.push_back (inst);
            }
        }
    }

    int emitRuntimeEpilogue (const std::vector<YdspIrInst>& oldBody)
    {
        const auto loopId = loop.id;
        const auto induction = loop.induction;

        int boundValue = -1;

        for (const auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == induction)
                boundValue = inst.b;

        auto& preheaderInsts = fn.blocks[static_cast<size_t> (preheader)].insts;

        const auto mask = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::constI, mask, -1, -1, -1, -1, 0.0, lanes - 1 });

        const auto tailValue = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::andI, tailValue, boundValue, mask });

        const auto vectorN = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::subI, vectorN, boundValue, tailValue });

        for (auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == induction)
                inst.b = vectorN;

        fn.blocks.insert (fn.blocks.begin() + exit, 2, YdspIrBlock {});

        for (auto& block : fn.blocks)
        {
            if (block.termTarget >= exit)
                block.termTarget += 2;

            if (block.termTarget2 >= exit)
                block.termTarget2 += 2;
        }

        for (auto& loopEntry : fn.loops)
        {
            if (loopEntry.headerBlock >= exit)
                loopEntry.headerBlock += 2;

            if (loopEntry.exitBlock >= exit)
                loopEntry.exitBlock += 2;
        }

        fn.loops[static_cast<size_t> (loopId)].exitBlock = exit;
        fn.blocks[static_cast<size_t> (header)].termTarget2 = exit;

        auto& tailHeaderInsts = fn.blocks[static_cast<size_t> (exit)].insts;

        const auto tailCond = newValue (YdspValueType::boolType, 1);
        tailHeaderInsts.push_back ({ YdspIrOp::ltI, tailCond, induction, boundValue });

        auto& tailHeader = fn.blocks[static_cast<size_t> (exit)];
        tailHeader.term = YdspIrTerm::branchIf;
        tailHeader.termCond = tailCond;
        tailHeader.termTarget = exit + 1;
        tailHeader.termTarget2 = exit + 2;

        auto& tailBodyInsts = fn.blocks[static_cast<size_t> (exit + 1)].insts;

        for (const auto& inst : oldBody)
        {
            auto copy = inst;

            if (copy.result >= 0)
                copy.result = tailId (copy.result);

            copy.a = tailId (copy.a);
            copy.b = tailId (copy.b);
            copy.c = tailId (copy.c);

            tailBodyInsts.push_back (copy);
        }

        auto& tailBody = fn.blocks[static_cast<size_t> (exit + 1)];
        tailBody.term = YdspIrTerm::branch;
        tailBody.termTarget = exit;

        YdspIrLoop tailLoop;
        tailLoop.id = static_cast<int> (fn.loops.size());
        tailLoop.headerBlock = exit;
        tailLoop.exitBlock = exit + 2;
        tailLoop.induction = induction;
        tailLoop.bound = { YdspLoopBoundKind::blockSize, 0 };
        fn.loops.push_back (tailLoop);

        return exit + 2;
    }

    //==============================================================================

    YdspIrFunction& fn;
    const YdspIrLoop& loop;
    const int lanes;

    YdspVectorizationReason rejectionReason = YdspVectorizationReason::notVectorizable;

    int header = -1;
    int body = -1;
    int exit = -1;
    int preheader = -1;
    int numValues = 0;

    int incrementIndex = -1;
    int inductionMoveIndex = -1;
    int widenedAccessCount = 0;

    int start = 0;
    int tail = 0;

    bool hasStreamAccess = false;

    std::vector<int> defsInBody, defsElsewhere, usesInBody, usesElsewhere;
    std::vector<bool> definedInHeader;
    std::vector<bool> isVector;
    std::vector<bool> widened;
    std::vector<bool> hoistToPreheader;
    std::vector<bool> valueHoisted;
    std::vector<int> tailRemap;
    std::vector<Reduction> reductions;
};

} // namespace

//==============================================================================

bool YdspVectorizer::run (YdspIrFunction& fn)
{
    return run (fn, vectorWidth);
}

bool YdspVectorizer::run (YdspIrFunction& fn, int targetVectorWidth)
{
    YdspVectorizationReport report;
    return run (fn, targetVectorWidth, report);
}

bool YdspVectorizer::run (YdspIrFunction& fn, int targetVectorWidth, YdspVectorizationReport& report)
{
    if (targetVectorWidth != 4 && targetVectorWidth != 8 && targetVectorWidth != 16)
        return false;

    bool changed = false;
    const auto loopCount = fn.loops.size();

    fn.vectorizationResults.clear();
    fn.vectorizationResults.reserve (loopCount);

    for (size_t i = 0; i < loopCount; ++i)
    {
        YdspVectorizationReason reason = YdspVectorizationReason::notVectorizable;

        if (LoopWidener (fn, fn.loops[i], targetVectorWidth).tryWiden (reason))
        {
            changed = true;
            fn.vectorizationResults.push_back ({ static_cast<int> (i), YdspVectorizationReason::widened, targetVectorWidth });
        }
        else
        {
            fn.vectorizationResults.push_back ({ static_cast<int> (i), reason, 1 });
        }
    }

    report.loops = fn.vectorizationResults;

    if (changed)
    {
        fn.vectorized = true;
        fn.vectorWidth = targetVectorWidth;
    }

    return changed;
}

} // namespace yup
