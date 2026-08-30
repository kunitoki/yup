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

bool hasValueResult (YdspIrOp op)
{
    switch (op)
    {
        case YdspIrOp::constF:
        case YdspIrOp::constI:
        case YdspIrOp::constB:
        case YdspIrOp::loadBlockSize:
        case YdspIrOp::loadSampleRate:
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::modF:
        case YdspIrOp::negF:
        case YdspIrOp::addI:
        case YdspIrOp::subI:
        case YdspIrOp::mulI:
        case YdspIrOp::divI:
        case YdspIrOp::modI:
        case YdspIrOp::negI:
        case YdspIrOp::minI:
        case YdspIrOp::maxI:
        case YdspIrOp::absI:
        case YdspIrOp::clampI:
        case YdspIrOp::signI:
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
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::sinF:
        case YdspIrOp::cosF:
        case YdspIrOp::tanF:
        case YdspIrOp::asinF:
        case YdspIrOp::acosF:
        case YdspIrOp::atanF:
        case YdspIrOp::sinhF:
        case YdspIrOp::coshF:
        case YdspIrOp::tanhF:
        case YdspIrOp::expF:
        case YdspIrOp::logF:
        case YdspIrOp::log10F:
        case YdspIrOp::signF:
        case YdspIrOp::powF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::fmodF:
        case YdspIrOp::atan2F:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::fmaF:
        case YdspIrOp::fmsubF:
        case YdspIrOp::selectB:
        case YdspIrOp::movF:
        case YdspIrOp::movI:
        case YdspIrOp::movB:
            return true;

        // `vsplat` and `vreduceAddF` are deliberately absent. They are pure, so
        // listing them would be sound for dead-code elimination - but it would
        // also expose them to constant folding and copy propagation, which know
        // nothing about lanes and would happily fold a splat of a literal into a
        // scalar constant. Only the vectoriser creates them, it runs after every
        // pass here, and it never creates one that is unused.
        default:
            return false;
    }
}

bool isValueIdOperand (YdspIrOp op, int operand)
{
    switch (op)
    {
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
            return operand != 0;

        default:
            return true;
    }
}

//==============================================================================

bool isConstantOp (YdspIrOp op)
{
    return op == YdspIrOp::constF || op == YdspIrOp::constI || op == YdspIrOp::constB;
}

bool isCseEligible (YdspIrOp op) noexcept
{
    // Loads observe mutable memory. Vector-only operations are left for the
    // vectoriser because the scalar passes do not carry lane information.
    if (! hasValueResult (op) || isConstantOp (op) || op == YdspIrOp::loadBlockSize || op == YdspIrOp::loadSampleRate
        || op == YdspIrOp::loadParam || op == YdspIrOp::loadParamOut
        || op == YdspIrOp::loadStateF || op == YdspIrOp::loadStateI
        || op == YdspIrOp::loadStateArrayF || op == YdspIrOp::loadStateArrayI
        || op == YdspIrOp::loadInput || op == YdspIrOp::loadOutput)
        return false;

    return true;
}

bool sameCseExpression (const YdspIrInst& a, YdspValueType aType, const YdspIrInst& b, YdspValueType bType) noexcept
{
    return a.op == b.op && aType == bType && a.a == b.a && a.b == b.b && a.c == b.c
        && a.memIndex == b.memIndex && a.ivalue == b.ivalue && a.bvalue == b.bvalue
        && std::bit_cast<uint64_t> (a.fvalue) == std::bit_cast<uint64_t> (b.fvalue);
}

YdspIrOp moveOpForCse (YdspValueType type) noexcept
{
    if (isFloatValueType (type))
        return YdspIrOp::movF;

    return type == YdspValueType::boolType ? YdspIrOp::movB : YdspIrOp::movI;
}

} // namespace

//==============================================================================

void YdspOptimizer::commonSubexpressionElimination (YdspIrFunction& fn)
{
    struct Expression
    {
        YdspIrInst inst;
        int result = -1;
        YdspValueType type = YdspValueType::boolType;
    };

    for (auto& block : fn.blocks)
    {
        std::vector<Expression> expressions;

        for (auto& inst : block.insts)
        {
            if (inst.result >= 0)
            {
                expressions.erase (std::remove_if (expressions.begin(), expressions.end(), [&] (const Expression& expression)
                {
                    return expression.result == inst.result || expression.inst.a == inst.result
                        || expression.inst.b == inst.result || expression.inst.c == inst.result;
                }),
                                  expressions.end());
            }

            if (inst.result < 0 || ! isCseEligible (inst.op))
                continue;

            const auto type = static_cast<size_t> (inst.result) < fn.valueTypes.size()
                                ? fn.valueTypes[static_cast<size_t> (inst.result)]
                                : inferTypeFromOp (inst.op);

            const auto match = std::find_if (expressions.begin(), expressions.end(), [&] (const Expression& expression)
            {
                return sameCseExpression (expression.inst, expression.type, inst, type);
            });

            if (match != expressions.end())
            {
                inst.op = moveOpForCse (type);
                inst.a = match->result;
                inst.b = -1;
                inst.c = -1;
                inst.memIndex = -1;
                continue;
            }

            expressions.push_back ({ inst, inst.result, type });
        }
    }
}

} // namespace yup
