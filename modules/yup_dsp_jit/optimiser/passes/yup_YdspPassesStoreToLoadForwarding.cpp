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

bool isStateArrayStore (YdspIrOp op) noexcept
{
    return op == YdspIrOp::storeStateArrayF || op == YdspIrOp::storeStateArrayI;
}

YdspIrOp matchingStoreFor (YdspIrOp loadOp) noexcept
{
    return loadOp == YdspIrOp::loadStateArrayF ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI;
}

} // namespace

//==============================================================================

void YdspOptimizer::storeToLoadForwarding (YdspIrFunction& fn)
{
    for (auto& block : fn.blocks)
    {
        auto& insts = block.insts;

        for (size_t j = 0; j < insts.size(); ++j)
        {
            auto& load = insts[j];

            if (load.op != YdspIrOp::loadStateArrayF && load.op != YdspIrOp::loadStateArrayI)
                continue;

            if (load.result < 0 || load.a < 0)
                continue;

            const auto storeOp = matchingStoreFor (load.op);

            size_t source = 0;
            bool found = false;

            for (size_t k = j; k-- > 0;)
            {
                const auto& previous = insts[k];

                if (previous.result >= 0 && previous.result == load.a)
                    break;

                if (! isStateArrayStore (previous.op))
                    continue;

                if (previous.op == storeOp && previous.memIndex == load.memIndex && previous.a == load.a)
                {
                    source = k;
                    found = true;
                    break;
                }

                if (previous.op != storeOp || previous.a != load.a || previous.memIndex == load.memIndex)
                    break;
            }

            if (! found)
                continue;

            const auto stored = insts[source].b;

            if (stored < 0 || stored == load.result)
                continue;

            bool valueStable = true;

            for (size_t k = source + 1; k < j && valueStable; ++k)
                valueStable = insts[k].result < 0 || insts[k].result != stored;

            if (! valueStable)
                continue;

            load.op = (load.op == YdspIrOp::loadStateArrayF) ? YdspIrOp::movF : YdspIrOp::movI;
            load.a = stored;
            load.b = -1;
            load.c = -1;
            load.memIndex = -1;
        }
    }
}
} // namespace yup
