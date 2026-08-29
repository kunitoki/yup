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
    return op == YdspIrOp::movF || op == YdspIrOp::movI || op == YdspIrOp::movB;
}

} // namespace

//==============================================================================

void YdspOptimizer::ifConversion (YdspIrFunction& fn)
{
    constexpr size_t maxSpeculatedInstructions = 8;

    if (fn.blocks.size() < 3)
        return;

    for (size_t c = 0; c + 2 < fn.blocks.size(); ++c)
    {
        const auto thenIndex = static_cast<int> (c) + 1;
        const auto joinIndex = static_cast<int> (c) + 2;

        if (fn.blocks[c].term != YdspIrTerm::branchIf
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

        if (! std::all_of (body.insts.begin(), body.insts.end(), [] (const YdspIrInst& inst)
        {
            return isSpeculatable (inst.op);
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
}
} // namespace yup
