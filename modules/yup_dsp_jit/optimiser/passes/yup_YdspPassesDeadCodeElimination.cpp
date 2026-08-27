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

void YdspOptimizer::deadCodeElimination (YdspIrFunction& fn)
{
    for (;;)
    {
        std::unordered_map<int, int> useCounts;

        for (auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.a >= 0)
                    ++useCounts[inst.a];
                if (inst.b >= 0)
                    ++useCounts[inst.b];
                if (inst.c >= 0)
                    ++useCounts[inst.c];
            }

            if (block.termCond >= 0)
                ++useCounts[block.termCond];
        }

        bool removed = false;

        for (auto& block : fn.blocks)
        {
            auto& insts = block.insts;

            insts.erase (std::remove_if (insts.begin(), insts.end(), [&] (const YdspIrInst& inst)
            {
                if (inst.result < 0 || ! hasValueResult (inst.op))
                    return false;

                const auto it = useCounts.find (inst.result);

                if (it != useCounts.end() && it->second > 0)
                    return false;

                removed = true;
                return true;
            }),
                         insts.end());
        }

        if (! removed)
            break;
    }
}
} // namespace yup
