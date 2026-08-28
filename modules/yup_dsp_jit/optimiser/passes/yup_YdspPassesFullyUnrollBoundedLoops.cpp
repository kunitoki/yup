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

/** The single constant an integer value id holds, or nullopt.

    The IR is not SSA, so a `constI` writing a register proves nothing unless it
    is that register's only definition - the same guard constantFolding() needs.
*/
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
                return std::nullopt; // written more than once

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
    //
    // 256 AArch64 instructions is a kilobyte against a 192 KB L1i, so the cache
    // is nowhere near the binding constraint at this size - what actually bounds
    // this is register pressure and compile time.
    //
    // Where the win is measured, it is large: a 16-mode bank widened to four
    // lanes unrolls to ~56 instructions and ran 37% faster (14.3 to 8.3
    // ns/sample), and its run-to-run spread fell from ~30% to under 8%. The
    // step from 128 to 256 is *not* measured, though: it was raised to admit a
    // 32-partial bank at ~190 instructions, and that shape then moved by ~2%,
    // which is inside its own spread. Treat 256 as unproven above 128.
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

    // A one-shot init kernel runs before audio does, so trading its code size
    // for branches buys nothing that can be heard.
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

        // The same CFG-linear shape the vectoriser matches: a single-block body
        // between its header and the block after it. A nested loop or an `if`
        // inside the body pushes the exit further along and is left alone.
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

        // The header only ever holds the bound and the compare, both pure. If
        // anything else has settled there, leave the loop alone rather than
        // reason about what dropping it would mean.
        if (! std::all_of (headerBlock.insts.begin(), headerBlock.insts.end(), [] (const YdspIrInst& inst)
        {
            return inst.result >= 0 && hasValueResult (inst.op);
        }))
            continue;

        // `next = i + step; i = next` closes every body. The step is what the
        // vectoriser rewrites when it widens, so read it rather than assume 1.
        const auto& move = bodyBlock.insts.back();

        if (move.op != YdspIrOp::movI || move.result != loop.induction || move.a < 0)
            continue;

        const auto& increment = bodyBlock.insts[bodyBlock.insts.size() - 2];

        if (increment.op != YdspIrOp::addI || increment.result != move.a || increment.a != loop.induction)
            continue;

        const auto step = constantIntValue (fn, increment.b);

        if (! step.has_value() || *step <= 0)
            continue;

        // The preheader's own `movI i, start` is the loop's entry value.
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
            continue; // a trip the compare would cut short; not this pass's job

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

        // ---- Rewrite ----
        // The body is copied verbatim, increment and all, so copy k sees
        // exactly the induction value iteration k saw. Nothing is substituted
        // and no value id is invented, which is what makes this sound in a
        // non-SSA IR: it is the same instruction sequence the loop executed,
        // written out straight.
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

        // Both now fall through into the exit, so no index moves and the two
        // emptied blocks cost nothing on any backend.
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
