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

struct ConstEntry
{
    bool isConst = false;
    bool isFloat = false;
    bool isBool = false;
    double f = 0.0;
    int64_t i = 0;
    bool b = false;
};

double computeUnaryFloat (YdspIrOp op, double x)
{
    switch (op)
    {
        case YdspIrOp::absF:
            return std::fabs (x);
        case YdspIrOp::sqrtF:
            return std::sqrt (x);
        case YdspIrOp::floorF:
            return std::floor (x);
        case YdspIrOp::ceilF:
            return std::ceil (x);
        case YdspIrOp::rintF:
            return std::rint (x);
        case YdspIrOp::sinF:
            return std::sin (x);
        case YdspIrOp::cosF:
            return std::cos (x);
        case YdspIrOp::tanF:
            return std::tan (x);
        case YdspIrOp::asinF:
            return std::asin (x);
        case YdspIrOp::acosF:
            return std::acos (x);
        case YdspIrOp::atanF:
            return std::atan (x);
        case YdspIrOp::sinhF:
            return std::sinh (x);
        case YdspIrOp::coshF:
            return std::cosh (x);
        case YdspIrOp::tanhF:
            return std::tanh (x);
        case YdspIrOp::expF:
            return std::exp (x);
        case YdspIrOp::logF:
            return std::log (x);
        case YdspIrOp::log10F:
            return std::log10 (x);
        case YdspIrOp::negF:
            return -x;
        case YdspIrOp::signF:
            return (x > 0.0) ? 1.0 : (x < 0.0 ? -1.0 : 0.0);
        default:
            return x;
    }
}

} // namespace

//==============================================================================

void YdspOptimizer::constantFolding (YdspIrFunction& fn)
{
    std::vector<ConstEntry> constants;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0 && static_cast<size_t> (inst.result) >= constants.size())
                constants.resize (static_cast<size_t> (inst.result) + 1);

    // The IR is non-SSA: a value id can be redefined at several program
    // points (the sample-loop induction register is written both by the
    // prologue `constI 0` and by the per-iteration `movI` update). A constant
    // recorded for one definition is not valid at the others, so only value
    // ids defined exactly once may ever be treated as constant.
    std::vector<int> definitionCount (constants.size(), 0);

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0)
                ++definitionCount[static_cast<size_t> (inst.result)];

    bool changed = true;

    while (changed)
    {
        changed = false;

        for (auto& block : fn.blocks)
        {
            for (auto& inst : block.insts)
            {
                if (isConstantOp (inst.op) && inst.result >= 0)
                {
                    // Multi-defined ids change value across program points;
                    // their recorded constant is never trustworthy.
                    if (definitionCount[static_cast<size_t> (inst.result)] != 1)
                        continue;

                    auto& entry = constants[static_cast<size_t> (inst.result)];

                    if (inst.op == YdspIrOp::constF)
                    {
                        entry = { true, true, false, inst.fvalue, 0, false };
                    }
                    else if (inst.op == YdspIrOp::constI)
                    {
                        entry = { true, false, false, 0.0, inst.ivalue, false };
                    }
                    else
                    {
                        entry = { true, false, true, 0.0, 0, inst.bvalue };
                    }

                    continue;
                }

                if (inst.result < 0 || ! hasValueResult (inst.op))
                    continue;

                auto operand = [&] (int id) -> const ConstEntry&
                {
                    static ConstEntry unknown;
                    return id >= 0 && static_cast<size_t> (id) < constants.size() ? constants[static_cast<size_t> (id)] : unknown;
                };

                const auto& a = operand (inst.a);
                const auto& b = operand (inst.b);
                const auto& c = operand (inst.c);

                const bool allConst = (inst.a < 0 || a.isConst) && (inst.b < 0 || b.isConst) && (inst.c < 0 || c.isConst);

                if (! allConst)
                    continue;

                // Folding a multi-defined result would also record a constant
                // that other uses of the id could read incorrectly.
                if (definitionCount[static_cast<size_t> (inst.result)] != 1)
                    continue;

                auto& result = constants[static_cast<size_t> (inst.result)];

                switch (inst.op)
                {
                    case YdspIrOp::addF:
                        result = { true, true, false, a.f + b.f };
                        break;
                    case YdspIrOp::subF:
                        result = { true, true, false, a.f - b.f };
                        break;
                    case YdspIrOp::mulF:
                        result = { true, true, false, a.f * b.f };
                        break;
                    case YdspIrOp::divF:
                        result = { true, true, false, a.f / b.f };
                        break;
                    case YdspIrOp::modF:
                        result = { true, true, false, std::fmod (a.f, b.f) };
                        break;
                    case YdspIrOp::negF:
                        result = { true, true, false, -a.f };
                        break;

                    case YdspIrOp::addI:
                        result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) + static_cast<uint64_t> (b.i)) };
                        break;
                    case YdspIrOp::subI:
                        result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) - static_cast<uint64_t> (b.i)) };
                        break;
                    case YdspIrOp::mulI:
                        result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) * static_cast<uint64_t> (b.i)) };
                        break;
                    case YdspIrOp::divI:
                        if (b.i != 0 && ! (a.i == std::numeric_limits<int64_t>::min() && b.i == -1))
                            result = { true, false, false, 0.0, a.i / b.i };
                        break;
                    case YdspIrOp::modI:
                        if (b.i != 0 && ! (a.i == std::numeric_limits<int64_t>::min() && b.i == -1))
                            result = { true, false, false, 0.0, a.i % b.i };
                        break;
                    case YdspIrOp::negI:
                        result = { true, false, false, 0.0, static_cast<int64_t> (0 - static_cast<uint64_t> (a.i)) };
                        break;

                    case YdspIrOp::minI:
                        result = { true, false, false, 0.0, std::min (a.i, b.i) };
                        break;
                    case YdspIrOp::maxI:
                        result = { true, false, false, 0.0, std::max (a.i, b.i) };
                        break;
                    case YdspIrOp::absI:
                        result = { true, false, false, 0.0, a.i < 0 ? static_cast<int64_t> (0 - static_cast<uint64_t> (a.i)) : a.i };
                        break;
                    case YdspIrOp::clampI:
                        result = { true, false, false, 0.0, std::min (std::max (a.i, b.i), c.i) };
                        break;
                    case YdspIrOp::signI:
                        result = { true, false, false, 0.0, a.i > 0 ? 1 : (a.i < 0 ? -1 : 0) };
                        break;

                    case YdspIrOp::andI:
                        result = { true, false, false, 0.0, a.i & b.i };
                        break;
                    case YdspIrOp::orI:
                        result = { true, false, false, 0.0, a.i | b.i };
                        break;
                    case YdspIrOp::xorI:
                        result = { true, false, false, 0.0, a.i ^ b.i };
                        break;
                    case YdspIrOp::shlI:
                        if (b.i >= 0 && b.i < 64)
                            result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) << static_cast<unsigned> (b.i)) };
                        break;
                    case YdspIrOp::shrI:
                        if (b.i >= 0 && b.i < 64)
                            result = { true, false, false, 0.0, a.i >> b.i };
                        break;

                    case YdspIrOp::eqF:
                        result = { true, false, true, 0.0, 0, a.f == b.f };
                        break;
                    case YdspIrOp::neF:
                        result = { true, false, true, 0.0, 0, a.f != b.f };
                        break;
                    case YdspIrOp::ltF:
                        result = { true, false, true, 0.0, 0, a.f < b.f };
                        break;
                    case YdspIrOp::leF:
                        result = { true, false, true, 0.0, 0, a.f <= b.f };
                        break;
                    case YdspIrOp::gtF:
                        result = { true, false, true, 0.0, 0, a.f > b.f };
                        break;
                    case YdspIrOp::geF:
                        result = { true, false, true, 0.0, 0, a.f >= b.f };
                        break;

                    case YdspIrOp::eqI:
                        result = { true, false, true, 0.0, 0, a.i == b.i };
                        break;
                    case YdspIrOp::neI:
                        result = { true, false, true, 0.0, 0, a.i != b.i };
                        break;
                    case YdspIrOp::ltI:
                        result = { true, false, true, 0.0, 0, a.i < b.i };
                        break;
                    case YdspIrOp::leI:
                        result = { true, false, true, 0.0, 0, a.i <= b.i };
                        break;
                    case YdspIrOp::gtI:
                        result = { true, false, true, 0.0, 0, a.i > b.i };
                        break;
                    case YdspIrOp::geI:
                        result = { true, false, true, 0.0, 0, a.i >= b.i };
                        break;

                    case YdspIrOp::andB:
                        result = { true, false, true, 0.0, 0, a.b && b.b };
                        break;
                    case YdspIrOp::orB:
                        result = { true, false, true, 0.0, 0, a.b || b.b };
                        break;
                    case YdspIrOp::notB:
                        result = { true, false, true, 0.0, 0, ! a.b };
                        break;

                    case YdspIrOp::itof:
                        result = { true, true, false, static_cast<double> (a.i) };
                        break;
                    case YdspIrOp::ftoi:
                        result = { true, false, false, 0.0, static_cast<int64_t> (a.f) };
                        break;

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
                        result = { true, true, false, computeUnaryFloat (inst.op, a.f) };
                        break;

                    case YdspIrOp::powF:
                        result = { true, true, false, std::pow (a.f, b.f) };
                        break;
                    case YdspIrOp::minF:
                        result = { true, true, false, std::fmin (a.f, b.f) };
                        break;
                    case YdspIrOp::maxF:
                        result = { true, true, false, std::fmax (a.f, b.f) };
                        break;
                    case YdspIrOp::fmodF:
                        result = { true, true, false, std::fmod (a.f, b.f) };
                        break;
                    case YdspIrOp::atan2F:
                        result = { true, true, false, std::atan2 (a.f, b.f) };
                        break;

                    case YdspIrOp::clampF:
                        result = { true, true, false, std::fmin (std::fmax (a.f, b.f), c.f) };
                        break;
                    case YdspIrOp::lerpF:
                        result = { true, true, false, a.f + (b.f - a.f) * c.f };
                        break;
                    case YdspIrOp::fmaF:
                        // Folded in double like every other float op here, and
                        // for this one that is not an approximation of the
                        // runtime behaviour but exactly it: the operands are
                        // float32, their product is exact in float64, and the
                        // single rounding back down to float32 is the rounding
                        // the hardware instruction performs.
                        result = { true, true, false, a.f * b.f + c.f };
                        break;
                    case YdspIrOp::selectB:
                        result = a.b ? b : c;
                        break;

                    case YdspIrOp::movF:
                        result = a;
                        break;
                    case YdspIrOp::movI:
                        result = a;
                        break;
                    case YdspIrOp::movB:
                        result = a;
                        break;

                    default:
                        continue;
                }

                // Replace the folded instruction with a constant.
                inst.op = result.isBool ? YdspIrOp::constB : (result.isFloat ? YdspIrOp::constF : YdspIrOp::constI);
                inst.a = inst.b = inst.c = -1;
                inst.memIndex = -1;
                inst.fvalue = result.f;
                inst.ivalue = result.i;
                inst.bvalue = result.b;

                changed = true;
            }
        }
    }
}
} // namespace yup
