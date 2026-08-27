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

/** How many times a value id is defined and read across the whole function.

    Whole-function rather than per-block because the IR is not SSA: a multiply
    whose result is read again three blocks later cannot be folded away, and
    only a global count can say so.
*/
struct YdspValueUseCounts
{
    std::vector<int> definitions;
    std::vector<int> uses;

    explicit YdspValueUseCounts (const YdspIrFunction& fn)
        : definitions (fn.valueTypes.size(), 0)
        , uses (fn.valueTypes.size(), 0)
    {
        const auto count = [this] (std::vector<int>& into, int value)
        {
            if (value >= 0 && static_cast<size_t> (value) < into.size())
                ++into[static_cast<size_t> (value)];
        };

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                count (definitions, inst.result);

                for (int operand = 0; operand < 3; ++operand)
                {
                    if (! isValueIdOperand (inst.op, operand))
                        continue;

                    count (uses, operand == 0 ? inst.a : (operand == 1 ? inst.b : inst.c));
                }
            }

            if (block.term == YdspIrTerm::branchIf)
                count (uses, block.termCond);
        }
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

} // namespace

//==============================================================================

void YdspOptimizer::contractMultiplyAdd (YdspIrFunction& fn)
{
    const YdspValueUseCounts counts (fn);

    const auto isScalarFloat32 = [&fn] (int value)
    {
        return value >= 0
            && static_cast<size_t> (value) < fn.valueTypes.size()
            && fn.valueTypes[static_cast<size_t> (value)] == YdspValueType::float32Type
            && fn.laneCountOf (value) == 1;
    };

    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;

        for (size_t i = 0; i < insts.size(); ++i)
        {
            if (insts[i].op != YdspIrOp::addF || ! isScalarFloat32 (insts[i].result))
                continue;

            // A candidate is a multiply this add consumes and nothing else does,
            // and which can be moved down to the add's position unchanged.
            const auto findMultiply = [&] (int operand) -> std::optional<size_t>
            {
                if (! isScalarFloat32 (operand) || ! counts.definedOnce (operand) || counts.useCount (operand) != 1)
                    return std::nullopt;

                for (size_t j = i; j-- > 0;)
                {
                    if (insts[j].result != operand)
                        continue;

                    if (insts[j].op != YdspIrOp::mulF || ! isScalarFloat32 (insts[j].a) || ! isScalarFloat32 (insts[j].b))
                        return std::nullopt;

                    // The multiply is about to happen later than it did, so
                    // anything it reads has to still hold the same value there.
                    for (size_t k = j + 1; k < i; ++k)
                        if (insts[k].result == insts[j].a || insts[k].result == insts[j].b)
                            return std::nullopt;

                    return j;
                }

                return std::nullopt; // defined in another block
            };

            const auto fromA = findMultiply (insts[i].a);
            const auto fromB = findMultiply (insts[i].b);

            if (! fromA.has_value() && ! fromB.has_value())
                continue;

            // With both operands eligible the choice is a latency decision, not
            // a free one. Fusing removes a link from the chain running through
            // the *fused* operand and leaves the other chain as it was, so on
            // `last * 0.5 + y * 0.5` fusing the `last` multiply leaves
            // `last -> fma -> last` where fusing the other leaves
            // `last -> mul -> fma -> last`, one link longer round a recurrence
            // that is the whole critical path.
            //
            // What distinguishes them locally is that the multiply on the
            // recurrence reads a register the block writes again further down -
            // a promoted `state` scalar carried across the sample loop's back
            // edge. That is the one to fuse.
            //
            // Scanned from the add rather than from the multiply: a write
            // between the two would mean the multiply cannot move down at all,
            // which is findMultiply()'s business and not this one's. Keeping
            // the ranges disjoint is what stops the two predicates having to
            // agree about anything.
            const auto isOnARecurrence = [&] (size_t multiplyIndex)
            {
                for (size_t k = i + 1; k < insts.size(); ++k)
                    if (insts[k].result == insts[multiplyIndex].a || insts[k].result == insts[multiplyIndex].b)
                        return true;

                return false;
            };

            auto chosen = fromA.value_or (fromB.value_or (0));

            if (fromA.has_value() && fromB.has_value() && ! isOnARecurrence (*fromA) && isOnARecurrence (*fromB))
                chosen = *fromB;

            // Copied, not referenced: the assignment below writes into the same
            // vector the multiply lives in.
            const auto factorA = insts[chosen].a;
            const auto factorB = insts[chosen].b;
            const auto addend = (insts[chosen].result == insts[i].a) ? insts[i].b : insts[i].a;

            // Checked rather than inferred from the add's result being scalar:
            // `vreduceAddF` produces a scalar from a widened operand, so a
            // scalar result does not by itself prove every operand is one.
            if (! isScalarFloat32 (addend))
                continue;

            insts[i].op = YdspIrOp::fmaF;
            insts[i].a = factorA;
            insts[i].b = factorB;
            insts[i].c = addend;

            // The multiply is now unread - it had exactly one use and this was
            // it - so the dead-code pass that follows collects it. Erasing it
            // here instead would invalidate `i` and every index above.
        }
    }
}
} // namespace yup
