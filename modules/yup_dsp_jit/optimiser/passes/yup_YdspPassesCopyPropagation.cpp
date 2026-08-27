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

void YdspOptimizer::copyPropagation (YdspIrFunction& fn)
{
    for (auto& block : fn.blocks)
    {
        for (size_t i = 0; i < block.insts.size(); ++i)
        {
            const auto& mov = block.insts[i];

            const bool isMov = mov.op == YdspIrOp::movF || mov.op == YdspIrOp::movI || mov.op == YdspIrOp::movB;

            if (! isMov || mov.result < 0 || mov.a < 0)
                continue;

            const int dst = mov.result;
            const int src = mov.a;

            // Scan forward in this block; propagate only while neither src nor
            // dst is redefined. Stopping at dst redefinition matters for
            // pass-by-value parameter copies (`mov t, phase` ... `mov t, t2`):
            // uses of t after the reassignment refer to t2, not to phase.
            bool srcRedefined = false;

            for (size_t j = i + 1; j < block.insts.size(); ++j)
            {
                auto& use = block.insts[j];

                if (use.result == src)
                {
                    srcRedefined = true;
                    break;
                }

                if (use.result == dst)
                    break;

                // Only rewrite fields that hold value ids: for the param/
                // state loads the `a` field is a slot index, which must not
                // be conflated with a mov's value id (see isValueIdOperand).
                if (isValueIdOperand (use.op, 0) && use.a == dst)
                    use.a = src;
                if (isValueIdOperand (use.op, 1) && use.b == dst)
                    use.b = src;
                if (isValueIdOperand (use.op, 2) && use.c == dst)
                    use.c = src;
            }

            if (! srcRedefined && block.termCond == dst)
                block.termCond = src;

            // The mov itself is left in place; DCE removes it if dst is unused elsewhere.
        }
    }
}
} // namespace yup
