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

std::optional<int> constantIntValue (const YdspIrFunction& fn, int value)
{
    if (value < 0)
        return std::nullopt;

    const YdspIrInst* definition = nullptr;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result != value)
                continue;

            if (definition != nullptr)
                return std::nullopt;

            definition = &inst;
        }
    }

    if (definition == nullptr || definition->op != YdspIrOp::constI)
        return std::nullopt;

    return static_cast<int> (definition->ivalue);
}

} // namespace

//==============================================================================

void YdspOptimizer::fullyUnrollBoundedLoops (YdspIrFunction& fn)
{
    // Copying a body is only a win while the copies stay small enough to keep
    // the whole sample loop in the instruction cache.
    constexpr int maxUnrolledInstructions = 256;
    // Scalar loops keep more independent values live across the whole body;
    // keeping their budget at the measured limit avoids turning a compact
    // source loop into a high-pressure native basic block. Widened loops have
    // already reduced that pressure by running fewer iterations.
    constexpr int maxScalarUnrolledInstructions = 128;
    constexpr int maxTripCount = 32;

    const auto hasNativeCall = [] (const YdspIrInst& inst)
    {
        switch (inst.op)
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
            case YdspIrOp::roundF:
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::fmodF:
            case YdspIrOp::atan2F:
            case YdspIrOp::copysignF:
                return true;

            default:
                return false;
        }
    };

    if (fn.isInit)
        return;

    for (auto& loop : fn.loops)
    {
        if (loop.unrolled || loop.bound.kind != YdspLoopBoundKind::constant || loop.bound.constant <= 0)
            continue;

        const auto header = loop.headerBlock;
        const auto body = header + 1;
        const auto exit = loop.exitBlock;
        const auto preheader = header - 1;

        if (preheader < 0 || exit != body + 1 || exit >= static_cast<int> (fn.blocks.size()))
            continue;

        auto& preheaderBlock = fn.blocks[static_cast<size_t> (preheader)];
        auto& headerBlock = fn.blocks[static_cast<size_t> (header)];
        auto& bodyBlock = fn.blocks[static_cast<size_t> (body)];

        if (headerBlock.term != YdspIrTerm::branchIf
            || headerBlock.termTarget != body
            || headerBlock.termTarget2 != exit)
            continue;

        if (bodyBlock.term != YdspIrTerm::branch || bodyBlock.termTarget != header)
            continue;

        if (preheaderBlock.term != YdspIrTerm::fallthrough || bodyBlock.insts.size() < 2)
            continue;

        auto allInstructionsHaveResult = std::all_of (headerBlock.insts.begin(), headerBlock.insts.end(), [] (const YdspIrInst& inst)
        {
            return inst.result >= 0 && hasValueResult (inst.op);
        });
        if (! allInstructionsHaveResult)
            continue;

        const auto& move = bodyBlock.insts.back();
        if (move.op != YdspIrOp::movI || move.result != loop.induction || move.a < 0)
            continue;

        const auto& increment = bodyBlock.insts[bodyBlock.insts.size() - 2];

        if (increment.op != YdspIrOp::addI || increment.result != move.a || increment.a != loop.induction)
            continue;

        const auto step = constantIntValue (fn, increment.b);

        if (! step.has_value() || *step <= 0)
            continue;

        int start = 0;
        bool foundStart = false;

        for (const auto& inst : preheaderBlock.insts)
        {
            if (inst.op != YdspIrOp::movI || inst.result != loop.induction)
                continue;

            if (const auto value = constantIntValue (fn, inst.a); value.has_value())
            {
                start = *value;
                foundStart = true;
            }
            else
            {
                foundStart = false;
            }
        }

        if (! foundStart || start >= loop.bound.constant)
            continue;

        const auto span = loop.bound.constant - start;

        if ((span % *step) != 0)
            continue;

        const auto tripCount = span / *step;
        const auto bodySize = static_cast<int> (bodyBlock.insts.size());

        if (std::any_of (bodyBlock.insts.begin(), bodyBlock.insts.end(), hasNativeCall))
            continue;

        const bool hasWidenedValue = std::any_of (bodyBlock.insts.begin(), bodyBlock.insts.end(), [&fn] (const YdspIrInst& inst)
        {
            return fn.laneCountOf (inst.result) > 1;
        });
        const auto instructionBudget = hasWidenedValue ? maxUnrolledInstructions : maxScalarUnrolledInstructions;

        if (tripCount < 1 || tripCount > maxTripCount || tripCount * bodySize > instructionBudget)
            continue;

        preheaderBlock.insts.insert (preheaderBlock.insts.end(),
                                     headerBlock.insts.begin(),
                                     headerBlock.insts.end());

        preheaderBlock.insts.reserve (preheaderBlock.insts.size()
                                      + static_cast<size_t> (tripCount * bodySize));

        for (int k = 0; k < tripCount; ++k)
            preheaderBlock.insts.insert (preheaderBlock.insts.end(),
                                         bodyBlock.insts.begin(),
                                         bodyBlock.insts.end());

        headerBlock.insts.clear();
        bodyBlock.insts.clear();

        for (auto* block : { &headerBlock, &bodyBlock })
        {
            block->term = YdspIrTerm::fallthrough;
            block->termCond = -1;
            block->termTarget = -1;
            block->termTarget2 = -1;
        }

        loop.unrolled = true;
    }
}
} // namespace yup
