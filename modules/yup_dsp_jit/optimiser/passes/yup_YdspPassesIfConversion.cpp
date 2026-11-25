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

bool isSpeculatable (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::constF:
        case YdspIrOp::constI:
        case YdspIrOp::constB:
        case YdspIrOp::movF:
        case YdspIrOp::movI:
        case YdspIrOp::movB:
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF: // a zero divisor yields inf/nan, it does not trap
        case YdspIrOp::negF:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::addI:
        case YdspIrOp::subI:
        case YdspIrOp::mulI:
        case YdspIrOp::negI:
        case YdspIrOp::wrapI:
        case YdspIrOp::minI:
        case YdspIrOp::maxI:
        case YdspIrOp::absI:
        case YdspIrOp::clampI:
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
        case YdspIrOp::truncI:
        case YdspIrOp::extF:
        case YdspIrOp::truncF:
        case YdspIrOp::selectB:
            return true;

        default:
            return false;
    }
}

bool isMove (YdspIrOp op) noexcept
{
    return isMoveOpcode (op);
}

// Calls without side effects, and reads of what is fixed for the block:
// evaluating one whose result is then discarded costs time and nothing else.
bool isPureBlockConstant (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadSampleRate:
        case YdspIrOp::loadBlockSize:
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
        case YdspIrOp::roundF:
        case YdspIrOp::copysignF:
            return true;

        default:
            return false;
    }
}

bool isSpeculatableStreamInputLoad (const YdspIrFunction& fn, YdspIrOp op, int indexValue, int containingBlock) noexcept
{
    if (op != YdspIrOp::loadInput || indexValue < 0 || containingBlock < 0)
        return false;

    const YdspIrLoop* innermost = nullptr;

    for (const auto& loop : fn.loops)
    {
        if (loop.headerBlock < containingBlock && containingBlock < loop.exitBlock)
            if (innermost == nullptr || loop.headerBlock > innermost->headerBlock)
                innermost = &loop;
    }

    if (innermost == nullptr || innermost->induction != indexValue)
        return false;

    switch (innermost->bound.kind)
    {
        case YdspLoopBoundKind::blockSize:
        case YdspLoopBoundKind::blockSizeMinusConst:
        case YdspLoopBoundKind::blockSizePlusConst:
            return true;

        default:
            return false;
    }
}

} // namespace

//==============================================================================

bool YdspOptimizer::ifConversion (YdspIrFunction& fn, bool speculateLoopInvariantArms)
{
    bool changed = false;

    constexpr size_t maxSpeculatedInstructions = 8;

    if (fn.blocks.size() < 3)
        return false;

    // Where each value is defined, for the loop-invariant arm test: -1 when it
    // has no definition and -2 when it has several.
    std::vector<int> definitionBlock (fn.valueTypes.size(), -1);

    if (speculateLoopInvariantArms)
        for (size_t b = 0; b < fn.blocks.size(); ++b)
            for (const auto& inst : fn.blocks[b].insts)
                if (inst.result >= 0)
                    definitionBlock[static_cast<size_t> (inst.result)] = definitionBlock[static_cast<size_t> (inst.result)] == -1 ? static_cast<int> (b) : -2;

    // An arm is fixed for the whole run of the loop around it when everything
    // it computes is pure and reads only values from before that loop (or its
    // own earlier results). Such an arm can run once ahead of the loop.
    const auto outermostHeaderAround = [&fn] (int block)
    {
        int outermostHeader = -1;

        for (const auto& loop : fn.loops)
            if (! loop.unrolled && loop.headerBlock <= block && block < loop.exitBlock
                && (outermostHeader < 0 || loop.headerBlock < outermostHeader))
                outermostHeader = loop.headerBlock;

        return outermostHeader;
    };

    const auto isLoopInvariantArm = [&] (int armIndex, size_t numTemporaries)
    {
        const int outermostHeader = outermostHeaderAround (armIndex);

        if (outermostHeader < 0)
            return false;

        const auto& insts = fn.blocks[static_cast<size_t> (armIndex)].insts;
        std::unordered_set<int> ownResults;

        for (size_t i = 0; i <= numTemporaries && i < insts.size(); ++i)
        {
            const auto& inst = insts[i];

            for (int operand = 0; operand < 3; ++operand)
            {
                const int value = operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c);

                if (value < 0 || ! isValueIdOperand (inst.op, operand) || ownResults.count (value) != 0)
                    continue;

                const auto definedIn = definitionBlock[static_cast<size_t> (value)];

                if (definedIn < 0 || definedIn >= outermostHeader)
                    return false;
            }

            if (i < numTemporaries)
            {
                if (definitionBlock[static_cast<size_t> (inst.result)] < 0)
                    return false;

                ownResults.insert (inst.result);
            }
        }

        return true;
    };

    // Two-sided: `if (cond) { y = a; } else { y = b; }` lowers to
    //   c:      branchIf cond -> then (c+1), else (c+2)
    //   then:   <pure temps...> mov y = a; branch -> join (c+3)
    //   else:   <pure temps...> mov y = b; branch -> join (c+3)
    //   join:   ...
    // When both arms write the same value and only move pure temporaries in,
    // replace them with one select in c (the wavefolder shape, every clipper).
    for (size_t c = 0; c + 3 < fn.blocks.size(); ++c)
    {
        const auto thenIndex = c + 1;
        const auto elseIndex = c + 2;
        const auto joinIndex = c + 3;

        const auto& condBlock = fn.blocks[c];

        if (condBlock.term != YdspIrTerm::branchIf
            || condBlock.termCond < 0
            || condBlock.termTarget != static_cast<int> (thenIndex)
            || condBlock.termTarget2 != static_cast<int> (elseIndex))
            continue;

        const auto armMove = [speculateLoopInvariantArms] (const YdspIrFunction& function, const YdspIrBlock& block, int armIndex, int join)
        {
            // Returns (resultId, sourceId, pureHoistEnd) or (-1, ...): the
            // final instruction must be a move into the shared value and every
            // earlier instruction a pure computation (no local writes - movs
            // are how the builder writes locals, and hoisting one
            // unconditionally would change which arm's side effect runs).
            if (block.term != YdspIrTerm::branch || block.termTarget != join || block.insts.empty())
                return std::tuple<int, int, size_t> (-1, -1, 0);

            const auto& mov = block.insts.back();

            if (! isMove (mov.op) || mov.result < 0 || mov.a < 0)
                return std::tuple<int, int, size_t> (-1, -1, 0);

            for (size_t i = 0; i + 1 < block.insts.size(); ++i)
                if ((! isSpeculatable (block.insts[i].op)
                     && ! (speculateLoopInvariantArms && isPureBlockConstant (block.insts[i].op) && block.insts[i].result >= 0)
                     && ! isSpeculatableStreamInputLoad (function, block.insts[i].op, block.insts[i].a, armIndex))
                    || isMove (block.insts[i].op))
                    return std::tuple<int, int, size_t> (-1, -1, 0);

            return std::tuple<int, int, size_t> (mov.result, mov.a, block.insts.size() - 1);
        };

        const auto thenArm = armMove (fn, fn.blocks[thenIndex], static_cast<int> (thenIndex), static_cast<int> (joinIndex));
        const auto elseArm = armMove (fn, fn.blocks[elseIndex], static_cast<int> (elseIndex), static_cast<int> (joinIndex));

        if (std::get<0> (thenArm) < 0 || std::get<0> (elseArm) < 0 || std::get<0> (thenArm) != std::get<0> (elseArm))
            continue;

        // A `?:` promises that only the chosen arm is evaluated. Selecting
        // between two values that already exist evaluates nothing, so that is
        // the one shape of it a select may replace.
        const auto onlyNamesAValue = [] (const YdspIrBlock& arm, size_t numTemporaries)
        {
            return std::all_of (arm.insts.begin(), arm.insts.begin() + static_cast<std::ptrdiff_t> (numTemporaries), [] (const YdspIrInst& inst)
            {
                return isConstantOp (inst.op);
            });
        };

        const bool namesValues = onlyNamesAValue (fn.blocks[thenIndex], std::get<2> (thenArm))
                              && onlyNamesAValue (fn.blocks[elseIndex], std::get<2> (elseArm));

        // The whole choice has to be fixed, condition included: one that the
        // signal decides keeps evaluating only the arm it picks.
        const auto conditionDefinedIn = definitionBlock[static_cast<size_t> (condBlock.termCond)];

        const bool invariantChoice = speculateLoopInvariantArms
                                  && conditionDefinedIn >= 0 && conditionDefinedIn < outermostHeaderAround (static_cast<int> (c))
                                  && isLoopInvariantArm (static_cast<int> (thenIndex), std::get<2> (thenArm))
                                  && isLoopInvariantArm (static_cast<int> (elseIndex), std::get<2> (elseArm));

        // This round only looks for the choices the hoist can now lift out.
        if (speculateLoopInvariantArms ? ! invariantChoice
                                       : condBlock.preservesConditionalEvaluation && ! namesValues)
            continue;

        const auto reachedElsewhere = [&fn, c, thenIndex, elseIndex]
        {
            for (size_t b = 0; b < fn.blocks.size(); ++b)
            {
                if (b == c)
                    continue;

                const auto& other = fn.blocks[b];

                if (other.termTarget == static_cast<int> (thenIndex)
                    || other.termTarget2 == static_cast<int> (thenIndex)
                    || other.termTarget == static_cast<int> (elseIndex)
                    || other.termTarget2 == static_cast<int> (elseIndex))
                    return true;
            }

            return false;
        };

        if (reachedElsewhere())
            continue;

        changed = true;
        const auto& thenBlock = fn.blocks[thenIndex];
        const auto& elseBlock = fn.blocks[elseIndex];

        // Hoist each arm's pure temporaries into the condition block (they are
        // straight-line and exclusive: both arms read only the pre-diamond
        // value), then select the shared value.
        auto& mergedInsts = const_cast<std::vector<YdspIrInst>&> (condBlock.insts);

        for (size_t i = 0; i < std::get<2> (thenArm); ++i)
            mergedInsts.push_back (thenBlock.insts[i]);

        for (size_t i = 0; i < std::get<2> (elseArm); ++i)
            mergedInsts.push_back (elseBlock.insts[i]);

        YdspIrInst select;
        select.op = YdspIrOp::selectB;
        select.result = std::get<0> (thenArm);
        select.a = condBlock.termCond;
        select.b = std::get<1> (thenArm);
        select.c = std::get<1> (elseArm);
        mergedInsts.push_back (select);

        const_cast<std::vector<YdspIrInst>&> (thenBlock.insts).clear();
        const_cast<std::vector<YdspIrInst>&> (elseBlock.insts).clear();

        // With the value selected in c and both arms empty, the diamond becomes
        // straight-line: neutralize the condition and the two empty arms so
        // they fall through to the join.
        for (const auto index : { c, thenIndex, elseIndex })
        {
            auto& block = fn.blocks[index];
            block.term = YdspIrTerm::fallthrough;
            block.termCond = -1;
            block.termTarget = -1;
            block.termTarget2 = -1;
            block.preservesConditionalEvaluation = false;
        }
    }

    if (speculateLoopInvariantArms)
        return changed;

    for (size_t c = 0; c + 2 < fn.blocks.size(); ++c)
    {
        const auto thenIndex = static_cast<int> (c) + 1;
        const auto joinIndex = static_cast<int> (c) + 2;

        if (fn.blocks[c].preservesConditionalEvaluation
            || fn.blocks[c].term != YdspIrTerm::branchIf
            || fn.blocks[c].termCond < 0
            || fn.blocks[c].termTarget != thenIndex
            || fn.blocks[c].termTarget2 != joinIndex)
            continue;

        const auto& body = fn.blocks[static_cast<size_t> (thenIndex)];

        if (body.term != YdspIrTerm::fallthrough
            && ! (body.term == YdspIrTerm::branch && body.termTarget == joinIndex))
            continue;

        if (body.insts.empty() || body.insts.size() > maxSpeculatedInstructions)
            continue;

        if (! std::all_of (body.insts.begin(), body.insts.end(), [&fn, thenIndex] (const YdspIrInst& inst)
        {
            return isSpeculatable (inst.op)
                || isSpeculatableStreamInputLoad (fn, inst.op, inst.a, thenIndex);
        }))
            continue;

        const auto reachedElsewhere = [&fn, thenIndex, c]
        {
            for (size_t b = 0; b < fn.blocks.size(); ++b)
            {
                if (b == c)
                    continue;

                const auto& other = fn.blocks[b];

                if (other.termTarget == thenIndex || other.termTarget2 == thenIndex)
                    return true;

                if (other.term == YdspIrTerm::fallthrough && static_cast<int> (b) + 1 == thenIndex)
                    return true;
            }

            return false;
        };

        if (reachedElsewhere())
            continue;

        changed = true;
        const auto condition = fn.blocks[c].termCond;
        auto merged = std::move (fn.blocks[static_cast<size_t> (thenIndex)].insts);

        for (auto& inst : merged)
        {
            if (isMove (inst.op) && inst.result >= 0 && inst.a >= 0)
            {
                YdspIrInst select;
                select.op = YdspIrOp::selectB;
                select.result = inst.result;
                select.a = condition;
                select.b = inst.a;
                select.c = inst.result;

                fn.blocks[c].insts.push_back (select);
            }
            else
            {
                fn.blocks[c].insts.push_back (inst);
            }
        }

        fn.blocks[static_cast<size_t> (thenIndex)].insts.clear();

        for (const auto block : { c, static_cast<size_t> (thenIndex) })
        {
            fn.blocks[block].term = YdspIrTerm::fallthrough;
            fn.blocks[block].termCond = -1;
            fn.blocks[block].termTarget = -1;
            fn.blocks[block].termTarget2 = -1;
        }
    }
    return changed;
}
} // namespace yup
