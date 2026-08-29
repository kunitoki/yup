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

void YdspOptimizer::lowerFusedMultiplyAdd (YdspIrFunction& fn)
{
    const auto newValue = [&fn] (YdspValueType type)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
            fn.valueLanes.push_back (1);

        return id;
    };

    for (auto& block : fn.blocks)
    {
        if (std::none_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst)
        {
            return inst.op == YdspIrOp::fmaF || inst.op == YdspIrOp::fmsubF;
        }))
            continue;

        std::vector<YdspIrInst> lowered;
        lowered.reserve (block.insts.size());

        for (const auto& inst : block.insts)
        {
            if (inst.op != YdspIrOp::fmaF && inst.op != YdspIrOp::fmsubF)
            {
                lowered.push_back (inst);
                continue;
            }

            const auto wideA = newValue (YdspValueType::float64Type);
            const auto wideB = newValue (YdspValueType::float64Type);
            const auto wideC = newValue (YdspValueType::float64Type);
            const auto product = newValue (YdspValueType::float64Type);
            const auto sum = newValue (YdspValueType::float64Type);

            lowered.push_back ({ YdspIrOp::extF, wideA, inst.a });
            lowered.push_back ({ YdspIrOp::extF, wideB, inst.b });
            lowered.push_back ({ YdspIrOp::extF, wideC, inst.c });
            lowered.push_back ({ YdspIrOp::mulF, product, wideA, wideB });
            lowered.push_back ({ inst.op == YdspIrOp::fmaF ? YdspIrOp::addF : YdspIrOp::subF,
                                 sum, inst.op == YdspIrOp::fmaF ? product : wideC,
                                 inst.op == YdspIrOp::fmaF ? wideC : product });
            lowered.push_back ({ YdspIrOp::truncF, inst.result, sum });
        }

        block.insts = std::move (lowered);
    }
}
} // namespace yup
