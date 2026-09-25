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

        const auto span = static_cast<int64_t> (loop.bound.constant) - start;

        if ((span % *step) != 0)
            continue;

        const auto tripCount = span / *step;
        const auto bodySize = static_cast<int> (bodyBlock.insts.size());

        const auto callCount = std::count_if (bodyBlock.insts.begin(), bodyBlock.insts.end(), hasNativeCall);
        if (callCount != 0)
        {
            // Four-lane native math needs one call per copy. Wider calls may
            // split, and scalar/WASM calls retain the original conservative path.
            if (! fn.vectorMathEnabled || callCount != 1 || tripCount > 2 || tripCount * bodySize > 64)
                continue;
            const auto call = std::find_if (bodyBlock.insts.begin(), bodyBlock.insts.end(), hasNativeCall);
            if (fn.laneCountOf (call->result) != 4)
                continue;

            std::unordered_set<int> defined, live;
            for (const auto& inst : bodyBlock.insts)
            {
                const int operands[] { inst.a, inst.b, inst.c };
                for (int operand = 0; operand < 3; ++operand)
                    if (isValueIdOperand (inst.op, operand) && operands[operand] >= 0
                        && defined.count (operands[operand]) == 0)
                        live.insert (operands[operand]);
                if (inst.result >= 0)
                    defined.insert (inst.result);
            }
            for (size_t b = 0; b < fn.blocks.size(); ++b)
            {
                if (b == static_cast<size_t> (body))
                    continue;
                for (const auto& inst : fn.blocks[b].insts)
                {
                    const int operands[] { inst.a, inst.b, inst.c };
                    for (int operand = 0; operand < 3; ++operand)
                        if (isValueIdOperand (inst.op, operand) && defined.count (operands[operand]) != 0)
                            live.insert (operands[operand]);
                }
                if (defined.count (fn.blocks[b].termCond) != 0)
                    live.insert (fn.blocks[b].termCond);
            }

            bool pressureFits = true;
            for (auto it = bodyBlock.insts.rbegin(); it != bodyBlock.insts.rend(); ++it)
            {
                live.erase (it->result);
                if (hasNativeCall (*it))
                {
                    const auto vectors = std::count_if (live.begin(), live.end(), [&fn] (int value)
                    {
                        return fn.laneCountOf (value) > 1;
                    });
                    // Use the smaller common budget of the four-lane native
                    // targets, including x64 where vectors survive via spills.
                    pressureFits &= vectors <= 2 && live.size() - static_cast<size_t> (vectors) <= 8;
                }
                const int operands[] { it->a, it->b, it->c };
                for (int operand = 0; operand < 3; ++operand)
                    if (isValueIdOperand (it->op, operand) && operands[operand] >= 0)
                        live.insert (operands[operand]);
            }
            if (! pressureFits)
                continue;
        }

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

        // Only rename values confined to one iteration. Reads before a local
        // definition and uses outside the body identify carried or escaping values.
        const auto valueCount = fn.valueTypes.size();
        std::vector<bool> local (valueCount, true), defined (valueCount, false);
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto inspectOperand = [&] (int value)
            {
                if (value >= 0 && (b != static_cast<size_t> (body) || ! defined[static_cast<size_t> (value)]))
                    local[static_cast<size_t> (value)] = false;
            };
            for (const auto& inst : fn.blocks[b].insts)
            {
                const int operands[] { inst.a, inst.b, inst.c };
                for (int operand = 0; operand < 3; ++operand)
                    if (isValueIdOperand (inst.op, operand))
                        inspectOperand (operands[operand]);
                if (inst.result >= 0)
                {
                    if (b == static_cast<size_t> (body))
                        defined[static_cast<size_t> (inst.result)] = true;
                    else
                        local[static_cast<size_t> (inst.result)] = false;
                }
            }
            if (fn.blocks[b].term == YdspIrTerm::branchIf)
                inspectOperand (fn.blocks[b].termCond);
        }

        const auto newValue = [&] (int original)
        {
            const auto type = fn.valueTypes[static_cast<size_t> (original)];
            const auto lanes = fn.laneCountOf (original);
            const int result = static_cast<int> (fn.valueTypes.size());
            fn.valueTypes.push_back (type);
            if (! fn.valueLanes.empty())
                fn.valueLanes.push_back (lanes);
            return result;
        };

        // An induction assignment inside the body makes substitution unsafe.
        const bool constantIndices = std::none_of (bodyBlock.insts.begin(), bodyBlock.insts.end() - 1, [&] (const auto& inst)
        {
            return inst.result == loop.induction;
        });
        for (int k = 0; k < tripCount; ++k)
        {
            std::vector<int> replacements (valueCount, -1);
            if (constantIndices)
            {
                const int index = newValue (loop.induction);
                preheaderBlock.insts.push_back ({ YdspIrOp::constI, index, -1, -1, -1, -1, 0.0, start + static_cast<int64_t> (k) * *step });
                replacements[static_cast<size_t> (loop.induction)] = index;
            }
            for (size_t i = 0; i < bodyBlock.insts.size(); ++i)
            {
                auto inst = bodyBlock.insts[i];
                int* operands[] { &inst.a, &inst.b, &inst.c };
                for (int operand = 0; operand < 3; ++operand)
                    if (isValueIdOperand (inst.op, operand) && *operands[operand] >= 0)
                    {
                        const auto replacement = replacements[static_cast<size_t> (*operands[operand])];
                        if (replacement >= 0)
                            *operands[operand] = replacement;
                    }
                if (inst.result >= 0 && local[static_cast<size_t> (inst.result)])
                {
                    const int original = inst.result;
                    inst.result = newValue (original);
                    replacements[static_cast<size_t> (original)] = inst.result;
                }
                if (constantIndices && i == bodyBlock.insts.size() - 2)
                    inst = { YdspIrOp::constI, inst.result, -1, -1, -1, -1, 0.0, start + static_cast<int64_t> (k + 1) * *step };
                preheaderBlock.insts.push_back (inst);
            }
        }

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
