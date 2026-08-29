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

void YdspOptimizer::algebraicSimplification (YdspIrFunction& fn)
{
    std::unordered_map<int, int> definitionCount;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0)
                ++definitionCount[inst.result];

    for (auto& block : fn.blocks)
    {
        for (auto& inst : block.insts)
        {
            const int result = inst.result;

            if (result < 0)
                continue;

            auto literalOf = [&] (int id) -> const YdspIrInst*
            {
                if (const auto count = definitionCount.find (id); count == definitionCount.end() || count->second != 1)
                    return nullptr;

                for (const auto& other : block.insts)
                    if (other.result == id && isConstantOp (other.op))
                        return &other;

                return nullptr;
            };

            switch (inst.op)
            {
                case YdspIrOp::addF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::addI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::subF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::subI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::mulF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::constF;
                        inst.a = inst.b = -1;
                        inst.fvalue = 0.0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->fvalue == 0.0)
                    {
                        inst.op = YdspIrOp::constF;
                        inst.a = inst.b = -1;
                        inst.fvalue = 0.0;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::mulI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::divF:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->fvalue == 1.0)
                    {
                        inst.op = YdspIrOp::movF;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::divI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::andI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = 0;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::orI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == -1)
                    {
                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = -1;
                        inst.ivalue = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::xorI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::shlI:
                case YdspIrOp::shrI:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->ivalue == 0)
                    {
                        inst.op = YdspIrOp::movI;
                        inst.b = -1;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::andB:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::constB;
                        inst.a = inst.b = -1;
                        inst.bvalue = false;
                        continue;
                    }

                    break;
                }

                case YdspIrOp::orB:
                {
                    if (const auto* lit = literalOf (inst.b); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.a); lit != nullptr && ! lit->bvalue)
                    {
                        inst.op = YdspIrOp::movB;
                        inst.a = inst.b;
                        inst.b = -1;
                        continue;
                    }

                    if (const auto* lit = literalOf (inst.b); lit != nullptr && lit->bvalue)
                    {
                        inst.op = YdspIrOp::constB;
                        inst.a = inst.b = -1;
                        inst.bvalue = true;
                        continue;
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }
}
} // namespace yup
