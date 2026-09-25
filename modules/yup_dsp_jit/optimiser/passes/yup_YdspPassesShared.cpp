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

bool hasValueResult (YdspIrOp op)
{
    switch (op)
    {
        case YdspIrOp::constF:
        case YdspIrOp::constI:
        case YdspIrOp::constB:
        case YdspIrOp::loadBlockSize:
        case YdspIrOp::loadSampleRate:
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadEventFieldI:
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::modF:
        case YdspIrOp::negF:
        case YdspIrOp::addI:
        case YdspIrOp::subI:
        case YdspIrOp::mulI:
        case YdspIrOp::divI:
        case YdspIrOp::modI:
        case YdspIrOp::negI:
        case YdspIrOp::minI:
        case YdspIrOp::maxI:
        case YdspIrOp::absI:
        case YdspIrOp::clampI:
        case YdspIrOp::signI:
        case YdspIrOp::andI:
        case YdspIrOp::orI:
        case YdspIrOp::xorI:
        case YdspIrOp::shlI:
        case YdspIrOp::shrI:
        case YdspIrOp::eqF:
        case YdspIrOp::neF:
        case YdspIrOp::ltF:
        case YdspIrOp::leF:
        case YdspIrOp::gtF:
        case YdspIrOp::geF:
        case YdspIrOp::eqI:
        case YdspIrOp::neI:
        case YdspIrOp::ltI:
        case YdspIrOp::ltUI:
        case YdspIrOp::leI:
        case YdspIrOp::gtI:
        case YdspIrOp::geI:
        case YdspIrOp::andB:
        case YdspIrOp::orB:
        case YdspIrOp::notB:
        case YdspIrOp::itof:
        case YdspIrOp::ftoi:
        case YdspIrOp::extI:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::asinF:
        case YdspIrOp::acosF:
        case YdspIrOp::atanF:
        case YdspIrOp::sinhF:
        case YdspIrOp::coshF:
        case YdspIrOp::tanhF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::signF:
        case YdspIrOp::powF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::fmodF:
        case YdspIrOp::atan2F:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::selectB:
        case YdspIrOp::movF:
        case YdspIrOp::movI:
        case YdspIrOp::movB:
            return true;

        // `vsplat` and `vreduceAddF` are deliberately absent. They are pure, so
        // listing them would be sound for dead-code elimination - but it would
        // also expose them to constant folding and copy propagation, which know
        // nothing about lanes and would happily fold a splat of a literal into a
        // scalar constant. Only the vectoriser creates them, it runs after every
        // pass here, and it never creates one that is unused.
        default:
            return false;
    }
}

bool isValueIdOperand (YdspIrOp op, int operand)
{
    return ! ydspFirstOperandIsSlot (op) || operand != 0;
}

//==============================================================================
template <typename F>
void forEachValueUse (const YdspIrFunction& fn, F&& visitor)
{
    const auto fieldOf = [] (const YdspIrInst& inst, int operand) noexcept -> int
    {
        return operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c);
    };

    for (int blockIndex = 0; blockIndex < static_cast<int> (fn.blocks.size()); ++blockIndex)
    {
        const auto& block = fn.blocks[static_cast<size_t> (blockIndex)];

        for (int instIndex = 0; instIndex < static_cast<int> (block.insts.size()); ++instIndex)
        {
            const auto& inst = block.insts[static_cast<size_t> (instIndex)];

            for (int operand = 0; operand < 3; ++operand)
            {
                if (! isValueIdOperand (inst.op, operand))
                    continue;

                const int value = fieldOf (inst, operand);

                if (value >= 0)
                    visitor (blockIndex, instIndex, operand, value);
            }
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond >= 0)
            visitor (blockIndex, -1, -1, block.termCond);
    }
}

struct YdspValueUseCounts
{
    std::vector<int> definitions;
    std::vector<int> uses;

    explicit YdspValueUseCounts (const YdspIrFunction& fn)
        : definitions (fn.valueTypes.size(), 0)
        , uses (fn.valueTypes.size(), 0)
    {
        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0 && static_cast<size_t> (inst.result) < definitions.size())
                    ++definitions[static_cast<size_t> (inst.result)];
            }
        }

        forEachValueUse (fn, [this] (int, int, int, int value)
        {
            ++uses[static_cast<size_t> (value)];
        });
    }

    bool definedOnce (int value) const
    {
        return value >= 0
            && static_cast<size_t> (value) < definitions.size()
            && definitions[static_cast<size_t> (value)] == 1;
    }

    int useCount (int value) const
    {
        return value >= 0 && static_cast<size_t> (value) < uses.size()
                 ? uses[static_cast<size_t> (value)]
                 : 0;
    }
};

//==============================================================================

bool isConstantOp (YdspIrOp op)
{
    return op == YdspIrOp::constF || op == YdspIrOp::constI || op == YdspIrOp::constB;
}

bool isCseEligible (YdspIrOp op) noexcept
{
    // Input streams are read-only. Array reads are reusable until an array
    // store invalidates the cache below; index redefinitions also invalidate it.
    // Other mutable loads remain excluded.
    if (! hasValueResult (op) || isConstantOp (op) || op == YdspIrOp::loadBlockSize || op == YdspIrOp::loadSampleRate
        || op == YdspIrOp::loadParam || op == YdspIrOp::loadParamOut
        || op == YdspIrOp::loadStateF || op == YdspIrOp::loadStateI
        || op == YdspIrOp::loadOutput
        || op == YdspIrOp::loadEventFieldF || op == YdspIrOp::loadEventFieldI)
        return false;

    return true;
}

bool sameCseExpression (const YdspIrInst& a, YdspValueType aType, const YdspIrInst& b, YdspValueType bType) noexcept
{
    return a.op == b.op && aType == bType && a.a == b.a && a.b == b.b && a.c == b.c
        && a.memIndex == b.memIndex && a.ivalue == b.ivalue && a.bvalue == b.bvalue
        && a.saturatingIntegerArithmetic == b.saturatingIntegerArithmetic
        && a.boundedFloatToInt == b.boundedFloatToInt
        && std::bit_cast<uint64_t> (a.fvalue) == std::bit_cast<uint64_t> (b.fvalue);
}

} // namespace

//==============================================================================

bool YdspOptimizer::commonSubexpressionElimination (YdspIrFunction& fn)
{
    bool changed = false;
    const YdspValueUseCounts counts (fn);
    std::vector<int> sharedValues (fn.valueTypes.size(), -1);
    const bool entryRunsOnce = std::none_of (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.termTarget == 0 || block.termTarget2 == 0;
    });
    const auto shareUse = [&] (int& value)
    {
        if (value >= 0 && sharedValues[static_cast<size_t> (value)] >= 0)
        {
            value = sharedValues[static_cast<size_t> (value)];
            changed = true;
        }
    };

    struct Expression
    {
        YdspIrInst inst;
        int result = -1;
        YdspValueType type = YdspValueType::boolType;
    };

    // The IR is non-SSA: a value id may be redefined at several points, and a
    // use refers to the most recent definition on the path that reached it.
    // Inside a single block that is unambiguous (the existing pass's rule).
    // Across blocks it stays sound only while execution is strictly linear -
    // one block, then the next, with no other way in and no back edge. Find
    // those maximal straight-line runs: block k flows to k+1, and no other
    // terminator targets k+1.
    const auto flowsToNext = [&fn] (int blockIndex) -> bool
    {
        const auto& block = fn.blocks[static_cast<size_t> (blockIndex)];

        if (block.term == YdspIrTerm::fallthrough)
            return true;

        return block.term == YdspIrTerm::branch && block.termTarget == blockIndex + 1;
    };

    const auto hasOtherPredecessor = [&fn] (int blockIndex, int from) -> bool
    {
        for (int b = 0; b < static_cast<int> (fn.blocks.size()); ++b)
        {
            if (b == from)
                continue;

            const auto& block = fn.blocks[static_cast<size_t> (b)];

            if (block.termTarget == blockIndex || block.termTarget2 == blockIndex)
                return true;
        }

        return false;
    };

    std::vector<std::pair<int, int>> segments;

    for (int start = 0; start < static_cast<int> (fn.blocks.size());)
    {
        int end = start;

        while (end + 1 < static_cast<int> (fn.blocks.size())
               && flowsToNext (end) && ! hasOtherPredecessor (end + 1, end))
            ++end;

        segments.emplace_back (start, end);
        start = end + 1;
    }

    for (const auto& [start, end] : segments)
    {
        std::vector<Expression> expressions;

        for (int blockIndex = start; blockIndex <= end; ++blockIndex)
        {
            for (auto& inst : fn.blocks[static_cast<size_t> (blockIndex)].insts)
            {
                if (isValueIdOperand (inst.op, 0))
                    shareUse (inst.a);
                if (isValueIdOperand (inst.op, 1))
                    shareUse (inst.b);
                if (isValueIdOperand (inst.op, 2))
                    shareUse (inst.c);

                if (inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeStateArrayI)
                {
                    const auto load = inst.op == YdspIrOp::storeStateArrayF ? YdspIrOp::loadStateArrayF : YdspIrOp::loadStateArrayI;
                    std::erase_if (expressions, [&fn, &inst, load] (const Expression& expression)
                    {
                        if (expression.inst.op != load)
                            return false;

                        if (inst.a < 0 || expression.inst.a != inst.a || inst.b < 0
                            || static_cast<size_t> (inst.b) >= fn.valueTypes.size()
                            || expression.type != fn.valueTypes[static_cast<size_t> (inst.b)])
                            return true;

                        const int64_t readStart = expression.inst.memIndex;
                        const int64_t writeStart = inst.memIndex;
                        return readStart < writeStart + fn.laneCountOf (inst.b)
                            && writeStart < readStart + fn.laneCountOf (expression.result);
                    });
                }

                if (inst.result >= 0)
                {
                    expressions.erase (std::remove_if (expressions.begin(), expressions.end(), [&] (const Expression& expression)
                    {
                        return expression.result == inst.result || expression.inst.a == inst.result
                            || expression.inst.b == inst.result || expression.inst.c == inst.result;
                    }),
                                      expressions.end());
                }

                if (inst.result < 0)
                    continue;

                const bool shareEntryValue = blockIndex == 0 && entryRunsOnce && counts.definedOnce (inst.result);
                if (! isCseEligible (inst.op) && ! (shareEntryValue && isConstantOp (inst.op)))
                    continue;

                const auto type = static_cast<size_t> (inst.result) < fn.valueTypes.size()
                                    ? fn.valueTypes[static_cast<size_t> (inst.result)]
                                    : inferTypeFromOp (inst.op);

                const auto match = std::find_if (expressions.begin(), expressions.end(), [&] (const Expression& expression)
                {
                    return sameCseExpression (expression.inst, expression.type, inst, type)
                        && fn.laneCountOf (expression.result) == fn.laneCountOf (inst.result);
                });

                if (match != expressions.end())
                {
                    if (shareEntryValue && counts.definedOnce (match->result))
                        sharedValues[static_cast<size_t> (inst.result)] = match->result;

                    changed |= inst.op != moveOpcodeFor (type) || inst.a != match->result
                            || inst.b != -1 || inst.c != -1 || inst.memIndex != -1;
                    inst.op = moveOpcodeFor (type);
                    inst.a = match->result;
                    inst.b = -1;
                    inst.c = -1;
                    inst.memIndex = -1;
                    continue;
                }

                expressions.push_back ({ inst, inst.result, type });
            }
            shareUse (fn.blocks[static_cast<size_t> (blockIndex)].termCond);
        }
    }
    return changed;
}

//==============================================================================

void YdspOptimizer::foldStateWriteBacks (YdspIrFunction& fn)
{
    const auto isFoldableProducer = [] (YdspIrOp op) noexcept
    {
        switch (op)
        {
            case YdspIrOp::movF:
            case YdspIrOp::movI:
            case YdspIrOp::fmaF:
            case YdspIrOp::fmsubF:
            case YdspIrOp::addF:
            case YdspIrOp::subF:
            case YdspIrOp::mulF:
            case YdspIrOp::negF:
            case YdspIrOp::absF:
            case YdspIrOp::sqrtF:
            case YdspIrOp::minF:
            case YdspIrOp::maxF:
            case YdspIrOp::addI:
            case YdspIrOp::subI:
            case YdspIrOp::mulI:
            case YdspIrOp::negI:
            case YdspIrOp::absI:
            case YdspIrOp::minI:
            case YdspIrOp::maxI:
            case YdspIrOp::clampI:
            case YdspIrOp::andI:
            case YdspIrOp::orI:
            case YdspIrOp::xorI:
            case YdspIrOp::shlI:
            case YdspIrOp::shrI:
            case YdspIrOp::selectB:
                return true;

            default:
                return false;
        }
    };

    const auto hasSameShape = [&fn] (int a, int b) noexcept
    {
        if (a < 0 || b < 0 || static_cast<size_t> (a) >= fn.valueTypes.size()
            || static_cast<size_t> (b) >= fn.valueTypes.size())
            return false;

        return fn.valueTypes[static_cast<size_t> (a)] == fn.valueTypes[static_cast<size_t> (b)]
            && fn.laneCountOf (a) == fn.laneCountOf (b);
    };

    const auto isConstantBoundLoopInduction = [&fn] (int value) noexcept
    {
        for (const auto& loop : fn.loops)
            if (loop.induction == value
                && loop.bound.kind == YdspLoopBoundKind::constant
                && ! loop.unrolled)
                return true;

        return false;
    };

    bool applied = true;

    while (applied)
    {
        applied = false;

        for (size_t blockIndex = 0; blockIndex < fn.blocks.size() && ! applied; ++blockIndex)
        {
            auto& block = fn.blocks[blockIndex];
            auto& insts = block.insts;

            for (size_t move = 0; move < insts.size() && ! applied; ++move)
            {
                const auto& mov = insts[move];

                if ((mov.op != YdspIrOp::movF && mov.op != YdspIrOp::movI)
                    || mov.result < 0 || mov.result == mov.a)
                    continue;

                const int carried = mov.result;
                const int produced = mov.a;

                if (isConstantBoundLoopInduction (carried))
                    continue;

                if (! hasSameShape (produced, carried))
                    continue;

                // The producer must define `produced` exactly once, and in this
                // same block: folding a producer hoisted into another block
                // would drag loop-invariant work back into the loop, and a
                // second definition would make some use read a different value.
                int producer = -1;
                bool producerIsUnique = true;

                for (const auto& otherBlock : fn.blocks)
                {
                    for (size_t j = 0; j < otherBlock.insts.size(); ++j)
                    {
                        if (otherBlock.insts[j].result != produced)
                            continue;

                        if (producer >= 0 || &otherBlock != &block)
                        {
                            producerIsUnique = false;
                            break;
                        }

                        producer = static_cast<int> (j);
                    }

                    if (! producerIsUnique)
                        break;
                }

                if (! producerIsUnique || producer < 0 || static_cast<size_t> (producer) >= move)
                    continue;

                if (! isFoldableProducer (insts[static_cast<size_t> (producer)].op))
                    continue;

                bool valid = true;

                forEachValueUse (fn, [&] (int useBlock, int useInst, int operand, int value)
                {
                    (void) operand;

                    if (! valid || value != produced)
                        return;

                    if (useInst < 0 || useBlock != static_cast<int> (blockIndex) || useInst <= producer)
                    {
                        valid = false;
                        return;
                    }

                    for (int j = producer + 1; j < useInst && valid; ++j)
                    {
                        if (j == static_cast<int> (move))
                            continue;

                        const auto& between = insts[static_cast<size_t> (j)];

                        if (between.result == carried)
                        {
                            valid = false;
                            return;
                        }

                        if (j < static_cast<int> (move))
                        {
                            for (int readOperand = 0; readOperand < 3; ++readOperand)
                            {
                                if (! isValueIdOperand (between.op, readOperand))
                                    continue;

                                const int readValue = readOperand == 0 ? between.a
                                                     : (readOperand == 1 ? between.b : between.c);

                                if (readValue == carried)
                                {
                                    valid = false;
                                    return;
                                }
                            }
                        }
                    }
                });

                if (! valid)
                    continue;

                insts[static_cast<size_t> (producer)].result = carried;

                for (auto& inst : insts)
                {
                    for (int operand = 0; operand < 3; ++operand)
                    {
                        if (! isValueIdOperand (inst.op, operand))
                            continue;

                        if (operand == 0)
                        {
                            if (inst.a == produced)
                                inst.a = carried;
                        }
                        else if (operand == 1)
                        {
                            if (inst.b == produced)
                                inst.b = carried;
                        }
                        else if (inst.c == produced)
                        {
                            inst.c = carried;
                        }
                    }
                }

                insts.erase (insts.begin() + static_cast<std::ptrdiff_t> (move));
                applied = true;
            }
        }
    }
}


//==============================================================================

void YdspOptimizer::rematerializePostCallLeaves (YdspIrFunction& fn)
{
    const auto isCallBoundary = [] (YdspIrOp op) noexcept
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
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
            case YdspIrOp::roundF:
            case YdspIrOp::copysignF:
                return true;

            default:
                return false;
        }
    };

    const auto isLeafDef = [] (YdspIrOp op) noexcept
    {
        return op == YdspIrOp::constF || op == YdspIrOp::constI || op == YdspIrOp::loadParam;
    };

    std::vector<std::pair<int, int>> defs (fn.valueTypes.size(), { -1, -1 });
    std::vector<char> multiDef (fn.valueTypes.size(), 0);

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        for (size_t i = 0; i < fn.blocks[b].insts.size(); ++i)
        {
            const auto result = fn.blocks[b].insts[i].result;

            if (result < 0 || static_cast<size_t> (result) >= defs.size())
                continue;

            if (defs[static_cast<size_t> (result)].first >= 0)
                multiDef[static_cast<size_t> (result)] = 1;
            else
                defs[static_cast<size_t> (result)] = { static_cast<int> (b), static_cast<int> (i) };
        }
    }

    for (const auto& loop : fn.loops)
    {
        if (loop.unrolled)
            continue;

        if (loop.headerBlock < 0 || loop.exitBlock <= loop.headerBlock + 1
            || loop.exitBlock >= static_cast<int> (fn.blocks.size()))
            continue;

        struct FlatInst
        {
            int block;
            int index;
        };

        std::vector<FlatInst> flatInsts;
        bool linear = true;

        for (int b = loop.headerBlock + 1; b < loop.exitBlock && linear; ++b)
        {
            const auto& block = fn.blocks[static_cast<size_t> (b)];

            for (size_t i = 0; i < block.insts.size(); ++i)
                flatInsts.push_back ({ b, static_cast<int> (i) });

            if (b == loop.exitBlock - 1)
                linear = block.term == YdspIrTerm::branch && block.termTarget == loop.headerBlock;
            else
                linear = block.term == YdspIrTerm::fallthrough
                      || (block.term == YdspIrTerm::branch && block.termTarget == b + 1);
        }

        if (! linear || flatInsts.empty())
            continue;

        std::vector<int> calls;
        for (size_t i = 0; i < flatInsts.size(); ++i)
            if (isCallBoundary (fn.blocks[static_cast<size_t> (flatInsts[i].block)].insts[static_cast<size_t> (flatInsts[i].index)].op))
                calls.push_back (static_cast<int> (i));

        // Work backwards so inserted clones do not shift earlier regions.
        for (size_t region = calls.size(); region-- > 0;)
        {
            const int lastCall = calls[region];
            const int regionEnd = region + 1 < calls.size() ? calls[region + 1] + 1
                                                           : static_cast<int> (flatInsts.size());

            struct RematCandidate
            {
                int value;
                YdspIrInst clone;
                YdspIrInst constant;
                bool hasConstant = false;
            };

            std::vector<RematCandidate> candidates;

            for (size_t i = static_cast<size_t> (lastCall) + 1; i < static_cast<size_t> (regionEnd); ++i)
            {
                const auto& use = fn.blocks[static_cast<size_t> (flatInsts[i].block)].insts[static_cast<size_t> (flatInsts[i].index)];

                const auto consider = [&] (int operand, int value)
                {
                    if (! isValueIdOperand (use.op, operand) || value < 0 || static_cast<size_t> (value) >= defs.size())
                        return;

                    if (multiDef[static_cast<size_t> (value)])
                        return;

                    const auto defBlock = defs[static_cast<size_t> (value)].first;
                    const auto defIndex = defs[static_cast<size_t> (value)].second;

                    if (defBlock < 0 || (defBlock > loop.headerBlock && defBlock < loop.exitBlock))
                        return;

                    const auto& def = fn.blocks[static_cast<size_t> (defBlock)].insts[static_cast<size_t> (defIndex)];

                    YdspIrInst constant;
                    bool rematerializeDifference = false;
                    if (defBlock == 0 && def.op == YdspIrOp::subF && region + 1 == calls.size()
                        && def.a >= 0 && def.b >= 0 && ! multiDef[static_cast<size_t> (def.a)] && ! multiDef[static_cast<size_t> (def.b)]
                        && defs[static_cast<size_t> (def.a)].first == 0 && defs[static_cast<size_t> (def.b)].first == 0)
                    {
                        constant = fn.blocks[0].insts[static_cast<size_t> (defs[static_cast<size_t> (def.a)].second)];
                        const auto& parameter = fn.blocks[0].insts[static_cast<size_t> (defs[static_cast<size_t> (def.b)].second)];
                        if (constant.op == YdspIrOp::constF && parameter.op == YdspIrOp::loadParam)
                        {
                            // Reuse a parameter already needed after the call;
                            // only the derived coefficient leaves the live set.
                            for (int f = lastCall + 1; f < regionEnd; ++f)
                            {
                                const auto& position = flatInsts[static_cast<size_t> (f)];
                                const auto& consumer = fn.blocks[static_cast<size_t> (position.block)].insts[static_cast<size_t> (position.index)];
                                rematerializeDifference |= (isValueIdOperand (consumer.op, 0) && consumer.a == def.b)
                                                        || (isValueIdOperand (consumer.op, 1) && consumer.b == def.b)
                                                        || (isValueIdOperand (consumer.op, 2) && consumer.c == def.b);
                            }
                        }
                    }

                    if ((! isLeafDef (def.op) && ! rematerializeDifference)
                        || (def.op == YdspIrOp::loadParam && region + 1 < calls.size()))
                        return;

                    for (const auto& candidate : candidates)
                        if (candidate.value == value)
                            return;

                    bool valid = true;

                    forEachValueUse (fn, [&] (int useBlock, int useInst, int operand, int useValue)
                    {
                        (void) operand;

                        if (! valid || useValue != value)
                            return;

                        int position = -1;

                        for (size_t f = 0; f < flatInsts.size(); ++f)
                            if (flatInsts[f].block == useBlock && flatInsts[f].index == useInst)
                            {
                                position = static_cast<int> (f);
                                break;
                            }

                        if (position <= lastCall || position >= regionEnd)
                            valid = false;
                    });

                    if (! valid)
                        return;

                    auto clone = def;
                    clone.result = -1; // fresh id assigned below
                    candidates.push_back ({ value, clone, constant, rematerializeDifference });
                };

                consider (0, use.a);
                consider (1, use.b);
                consider (2, use.c);
            }

            if (candidates.empty())
                continue;

            auto& callBlockInsts = fn.blocks[static_cast<size_t> (flatInsts[static_cast<size_t> (lastCall)].block)].insts;
            const auto callInstIndex = flatInsts[static_cast<size_t> (lastCall)].index;

            std::vector<YdspIrInst> clones;
            clones.reserve (candidates.size());

            for (auto& candidate : candidates)
            {
                const auto type = fn.valueTypes[static_cast<size_t> (candidate.value)];
                const auto lanes = fn.laneCountOf (candidate.value);

                if (candidate.hasConstant)
                {
                    const int constantId = static_cast<int> (fn.valueTypes.size());
                    fn.valueLanes.resize (fn.valueTypes.size(), 1);
                    fn.valueTypes.push_back (type);
                    fn.valueLanes.push_back (lanes);
                    candidate.constant.result = constantId;
                    clones.push_back (candidate.constant);
                    candidate.clone.a = constantId;
                }

                const auto newId = static_cast<int> (fn.valueTypes.size());
                fn.valueTypes.push_back (type);

                if (fn.valueLanes.size() < fn.valueTypes.size())
                    fn.valueLanes.resize (fn.valueTypes.size(), 1);

                fn.valueLanes[static_cast<size_t> (newId)] = lanes;
                candidate.clone.result = newId;
                clones.push_back (candidate.clone);
            }

            const auto remap = [&candidates] (int value)
            {
                for (const auto& candidate : candidates)
                    if (candidate.value == value)
                        return candidate.clone.result;

                return value;
            };

            for (int b = loop.headerBlock + 1; b < loop.exitBlock; ++b)
            {
                auto& blockInsts = fn.blocks[static_cast<size_t> (b)].insts;

                for (auto& inst : blockInsts)
                {
                    if (isValueIdOperand (inst.op, 0))
                        inst.a = remap (inst.a);

                    if (isValueIdOperand (inst.op, 1))
                        inst.b = remap (inst.b);

                    if (isValueIdOperand (inst.op, 2))
                        inst.c = remap (inst.c);
                }
            }

            callBlockInsts.insert (callBlockInsts.begin() + (callInstIndex + 1), clones.begin(), clones.end());
        }
    }
}

} // namespace yup
