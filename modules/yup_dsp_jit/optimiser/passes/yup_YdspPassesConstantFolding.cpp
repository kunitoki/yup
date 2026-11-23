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
        case YdspIrOp::asinhF:
            return std::asinh (x);
        case YdspIrOp::acoshF:
            return std::acosh (x);
        case YdspIrOp::atanhF:
            return std::atanh (x);
        case YdspIrOp::roundF:
            return std::round (x);
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

bool YdspOptimizer::constantFolding (YdspIrFunction& fn)
{
    bool anyChanged = false;

    std::vector<ConstEntry> constants;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result >= 0 && static_cast<size_t> (inst.result) >= constants.size())
                constants.resize (static_cast<size_t> (inst.result) + 1);

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
                        const auto value = static_cast<size_t> (inst.result) < fn.valueTypes.size()
                                                && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int32Type
                                             ? static_cast<int64_t> (static_cast<int32_t> (inst.ivalue))
                                             : inst.ivalue;
                        entry = { true, false, false, 0.0, value, false };
                    }
                    else
                    {
                        entry = { true, false, true, 0.0, static_cast<int64_t> (inst.bvalue), inst.bvalue };
                    }

                    continue;
                }

                if (inst.result < 0 || ! hasValueResult (inst.op) || fn.laneCountOf (inst.result) != 1)
                    continue;

                if (inst.op == YdspIrOp::phi)
                {
                    if (definitionCount[static_cast<size_t> (inst.result)] != 1 || inst.phiIncoming.empty())
                        continue;

                    bool allSameConst = true;
                    ConstEntry firstEntry;

                    for (const auto& [pred, val] : inst.phiIncoming)
                    {
                        (void) pred;

                        if (val == inst.result)
                            continue;

                        if (val < 0 || static_cast<size_t> (val) >= constants.size()
                            || ! constants[static_cast<size_t> (val)].isConst)
                        {
                            allSameConst = false;
                            break;
                        }

                        const auto& candidate = constants[static_cast<size_t> (val)];

                        if (! firstEntry.isConst)
                        {
                            firstEntry = candidate;
                        }
                        else if (firstEntry.isFloat != candidate.isFloat
                                 || firstEntry.isBool != candidate.isBool
                                 || std::bit_cast<uint64_t> (firstEntry.f) != std::bit_cast<uint64_t> (candidate.f)
                                 || firstEntry.i != candidate.i
                                 || firstEntry.b != candidate.b)
                        {
                            allSameConst = false;
                            break;
                        }
                    }

                    if (! allSameConst || ! firstEntry.isConst)
                        continue;

                    constants[static_cast<size_t> (inst.result)] = firstEntry;
                    inst.op = firstEntry.isBool ? YdspIrOp::constB : (firstEntry.isFloat ? YdspIrOp::constF : YdspIrOp::constI);
                    inst.a = inst.b = inst.c = -1;
                    inst.memIndex = -1;
                    inst.fvalue = firstEntry.f;
                    inst.ivalue = firstEntry.i;
                    inst.bvalue = firstEntry.b;
                    inst.phiIncoming.clear();

                    changed = true;
                    anyChanged = true;
                    continue;
                }

                auto operand = [&] (int id) -> const ConstEntry&
                {
                    static ConstEntry unknown;
                    return id >= 0 && static_cast<size_t> (id) < constants.size() ? constants[static_cast<size_t> (id)] : unknown;
                };

                const auto& a = operand (inst.a);
                const auto& b = operand (inst.b);
                const auto& c = operand (inst.c);

                if (inst.op == YdspIrOp::selectB && a.isConst && a.isBool
                    && definitionCount[static_cast<size_t> (inst.result)] == 1
                    && fn.laneCountOf (inst.result) == 1
                    && (! b.isConst || ! c.isConst))
                {
                    const int chosen = a.b ? inst.b : inst.c;
                    const auto type = static_cast<size_t> (inst.result) < fn.valueTypes.size()
                                          ? fn.valueTypes[static_cast<size_t> (inst.result)]
                                          : YdspValueType::float32Type;

                    if (chosen >= 0)
                    {
                        inst.op = moveOpcodeFor (type);
                        inst.a = chosen;
                        inst.b = -1;
                        inst.c = -1;
                        changed = true;
                        anyChanged = true;
                        continue;
                    }
                }

                // An element of a table: the array is written by its
                // initialiser list alone, so a known index names a value.
                if (inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI)
                {
                    if (static_cast<size_t> (inst.result) >= fn.valueTypes.size())
                        continue;

                    const auto type = fn.valueTypes[static_cast<size_t> (inst.result)];
                    const bool isFloat = isFloatValueType (type);

                    if ((inst.op == YdspIrOp::loadStateArrayF) != isFloat)
                        continue;

                    const auto table = std::find_if (fn.stateArrays.begin(), fn.stateArrays.end(), [&] (const YdspIrFunction::StateArray& array)
                    {
                        return array.compileTimeConstant && array.base == inst.memIndex && array.type == type;
                    });

                    if (table == fn.stateArrays.end() || ! a.isConst || a.isFloat || a.isBool || a.i < 0 || a.i >= table->length
                        || definitionCount[static_cast<size_t> (inst.result)] != 1 || fn.laneCountOf (inst.result) != 1)
                        continue;

                    if (isFloat)
                    {
                        if (static_cast<size_t> (a.i) >= table->constFloatValues.size())
                            continue;

                        const auto value = table->constFloatValues[static_cast<size_t> (a.i)];
                        constants[static_cast<size_t> (inst.result)] = { true, true, false, value, 0, false };

                        inst.op = YdspIrOp::constF;
                        inst.a = inst.b = inst.c = -1;
                        inst.memIndex = -1;
                        inst.fvalue = value;
                    }
                    else
                    {
                        if (static_cast<size_t> (a.i) >= table->constIntValues.size())
                            continue;

                        const auto rawValue = table->constIntValues[static_cast<size_t> (a.i)];
                        const auto value = type == YdspValueType::int32Type
                                             ? static_cast<int64_t> (static_cast<int32_t> (rawValue))
                                             : rawValue;
                        constants[static_cast<size_t> (inst.result)] = { true, false, false, 0.0, value, false };

                        inst.op = YdspIrOp::constI;
                        inst.a = inst.b = inst.c = -1;
                        inst.memIndex = -1;
                        inst.ivalue = value;
                    }

                    changed = true;
                    anyChanged = true;
                    continue;
                }

                const bool allConst = (inst.a < 0 || a.isConst) && (inst.b < 0 || b.isConst) && (inst.c < 0 || c.isConst);

                if (! allConst)
                    continue;

                if (definitionCount[static_cast<size_t> (inst.result)] != 1)
                    continue;

                auto& result = constants[static_cast<size_t> (inst.result)];
                const bool isInt32 = static_cast<size_t> (inst.result) < fn.valueTypes.size()
                                  && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int32Type;
                const auto intMin = isInt32 ? std::numeric_limits<int32_t>::min() : std::numeric_limits<int64_t>::min();
                const auto intMax = isInt32 ? std::numeric_limits<int32_t>::max() : std::numeric_limits<int64_t>::max();
                const auto ai = isInt32 ? static_cast<int32_t> (a.i) : a.i;
                const auto bi = isInt32 ? static_cast<int32_t> (b.i) : b.i;

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
                        if (inst.saturatingIntegerArithmetic)
                            result = { true, false, false, 0.0, bi > 0 && ai > intMax - bi ? intMax : (bi < 0 && ai < intMin - bi ? intMin : ai + bi) };
                        else
                            result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) + static_cast<uint64_t> (b.i)) };
                        break;
                    case YdspIrOp::subI:
                        if (inst.saturatingIntegerArithmetic)
                            result = { true, false, false, 0.0, bi < 0 && ai > intMax + bi ? intMax : (bi > 0 && ai < intMin + bi ? intMin : ai - bi) };
                        else
                            result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) - static_cast<uint64_t> (b.i)) };
                        break;
                    case YdspIrOp::mulI:
                    {
                        if (! inst.saturatingIntegerArithmetic)
                        {
                            result = { true, false, false, 0.0, static_cast<int64_t> (static_cast<uint64_t> (a.i) * static_cast<uint64_t> (b.i)) };
                            break;
                        }
                        const bool negative = (ai < 0) != (bi < 0);
                        const auto magnitudeA = ai < 0 ? uint64_t { 0 } - static_cast<uint64_t> (ai) : static_cast<uint64_t> (ai);
                        const auto magnitudeB = bi < 0 ? uint64_t { 0 } - static_cast<uint64_t> (bi) : static_cast<uint64_t> (bi);
                        const auto limit = negative ? uint64_t { 0 } - static_cast<uint64_t> (intMin) : static_cast<uint64_t> (intMax);
                        const bool overflow = magnitudeB != 0 && magnitudeA > limit / magnitudeB;
                        result = { true, false, false, 0.0, overflow ? (negative ? intMin : intMax) : ai * bi };
                        break;
                    }
                    case YdspIrOp::divI:
                        result = { true, false, false, 0.0, bi == 0 ? 0 : (ai == intMin && bi == -1 ? intMax : ai / bi) };
                        break;
                    case YdspIrOp::modI:
                        result = { true, false, false, 0.0, bi == 0 || (ai == intMin && bi == -1) ? 0 : ai % bi };
                        break;
                    case YdspIrOp::negI:
                        result = { true, false, false, 0.0, ai == intMin ? intMax : -ai };
                        break;

                    case YdspIrOp::minI:
                        result = { true, false, false, 0.0, std::min (a.i, b.i) };
                        break;
                    case YdspIrOp::maxI:
                        result = { true, false, false, 0.0, std::max (a.i, b.i) };
                        break;
                    case YdspIrOp::absI:
                        result = { true, false, false, 0.0, ai == intMin ? intMax : (ai < 0 ? -ai : ai) };
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
                    {
                        const auto count = static_cast<unsigned> (b.i) & (isInt32 ? 31u : 63u);
                        const auto bits = static_cast<uint64_t> (ai) << count;
                        result = { true, false, false, 0.0, isInt32 ? static_cast<int64_t> (static_cast<int32_t> (bits)) : static_cast<int64_t> (bits) };
                        break;
                    }
                    case YdspIrOp::shrI:
                        result = { true, false, false, 0.0, ai >> (static_cast<unsigned> (b.i) & (isInt32 ? 31u : 63u)) };
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
                    case YdspIrOp::ltUI:
                        result = { true, false, true, 0.0, 0,
                                   fn.valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::int64Type
                                       ? static_cast<uint64_t> (a.i) < static_cast<uint64_t> (b.i)
                                       : static_cast<uint32_t> (a.i) < static_cast<uint32_t> (b.i) };
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

                    case YdspIrOp::extI:
                        result = { true, false, false, 0.0, a.isBool ? static_cast<int64_t> (a.b) : static_cast<int32_t> (a.i) };
                        break;
                    case YdspIrOp::itof:
                        result = { true, true, false, a.isBool ? static_cast<double> (a.b) : static_cast<double> (fn.valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::int32Type ? static_cast<int32_t> (a.i) : a.i) };
                        break;
                    case YdspIrOp::ftoi:
                    {
                        const double value = fn.valueTypes[static_cast<size_t> (inst.a)] == YdspValueType::float32Type ? static_cast<float> (a.f) : a.f;
                        const auto truncated = std::trunc (value);
                        const auto converted = std::isnan (value) ? int64_t { 0 }
                                             : truncated >= -static_cast<double> (intMin) ? intMax
                                             : truncated <= static_cast<double> (intMin) ? intMin
                                             : static_cast<int64_t> (truncated);
                        result = { true, false, false, 0.0, converted };
                        break;
                    }

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
                    case YdspIrOp::asinhF:
                    case YdspIrOp::acoshF:
                    case YdspIrOp::atanhF:
                    case YdspIrOp::roundF:
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
                        result = { true, true, false, a.f * b.f + c.f };
                        break;
                    case YdspIrOp::selectB:
                        result = a.b ? b : c;
                        break;

                    case YdspIrOp::movF:
                        result = a;
                        break;
                    case YdspIrOp::movI:
                        result = { true, false, false, 0.0, a.isBool ? static_cast<int64_t> (a.b) : a.i };
                        break;
                    case YdspIrOp::movB:
                        result = a;
                        break;

                    default:
                        continue;
                }

                // Unsupported folds must stay unfolded rather than becoming zero.
                if (! result.isConst)
                    continue;

                if (result.isBool)
                    result.i = static_cast<int64_t> (result.b);

                if (result.isFloat)
                {
                    // A folded float32 value rounds once to float32 so folded
                    // and unfolded code agree; keeping the double intermediate
                    // would hand the folded build extra precision.
                    if (static_cast<size_t> (inst.result) < fn.valueTypes.size()
                        && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type)
                        result.f = static_cast<float> (result.f);
                }
                else if (! result.isBool && isInt32)
                {
                    // Consumers must see the same signed value as a 32-bit register.
                    result.i = static_cast<int32_t> (result.i);
                }

                inst.saturatingIntegerArithmetic = false;
                inst.op = result.isBool ? YdspIrOp::constB : (result.isFloat ? YdspIrOp::constF : YdspIrOp::constI);
                inst.a = inst.b = inst.c = -1;
                inst.memIndex = -1;
                inst.fvalue = result.f;
                inst.ivalue = result.i;
                inst.bvalue = result.b;

                changed = true;
                anyChanged = true;
            }
        }
    }
    bool foldedBranch = false;

    for (size_t blockIdx = 0; blockIdx < fn.blocks.size(); ++blockIdx)
    {
        auto& block = fn.blocks[blockIdx];

        if (block.term != YdspIrTerm::branchIf || block.termCond < 0
            || static_cast<size_t> (block.termCond) >= constants.size())
            continue;

        const auto& condition = constants[static_cast<size_t> (block.termCond)];
        if (! condition.isConst || ! condition.isBool
            || definitionCount[static_cast<size_t> (block.termCond)] != 1)
            continue;

        const int keptTarget = condition.b ? block.termTarget : block.termTarget2;
        const int deadTarget = condition.b ? block.termTarget2 : block.termTarget;

        block.term = YdspIrTerm::branch;
        block.termTarget = keptTarget;
        block.termTarget2 = -1;
        block.termCond = -1;
        foldedBranch = true;

        if (deadTarget >= 0 && static_cast<size_t> (deadTarget) < fn.blocks.size() && deadTarget != keptTarget)
        {
            for (auto& inst : fn.blocks[static_cast<size_t> (deadTarget)].insts)
            {
                if (inst.op != YdspIrOp::phi)
                    continue;

                inst.phiIncoming.erase (std::remove_if (inst.phiIncoming.begin(), inst.phiIncoming.end(), [blockIdx] (const auto& entry)
                {
                    return entry.first == static_cast<int> (blockIdx);
                }), inst.phiIncoming.end());
            }
        }
    }

    if (foldedBranch)
    {
        // Keep phiIncoming predecessor indices synchronized with block
        // compaction performed by removeUnreachableBlocks().
        if (fn.blocks.size() >= 2)
        {
            std::vector<char> reachable (fn.blocks.size(), 0);
            std::vector<int> stack { 0 };
            reachable[0] = 1;

            while (! stack.empty())
            {
                const auto index = stack.back();
                stack.pop_back();

                const auto& block = fn.blocks[static_cast<size_t> (index)];
                const auto visit = [&] (int target)
                {
                    if (target >= 0 && static_cast<size_t> (target) < fn.blocks.size() && ! reachable[static_cast<size_t> (target)])
                    {
                        reachable[static_cast<size_t> (target)] = 1;
                        stack.push_back (target);
                    }
                };

                if (block.term == YdspIrTerm::fallthrough)
                {
                    if (index + 1 < static_cast<int> (fn.blocks.size()))
                        visit (index + 1);
                    continue;
                }

                visit (block.termTarget);
                visit (block.termTarget2);
            }

            if (std::find (reachable.begin(), reachable.end(), 0) != reachable.end())
            {
                std::vector<int> remap (fn.blocks.size(), -1);
                int next = 0;

                for (size_t i = 0; i < fn.blocks.size(); ++i)
                    if (reachable[i])
                        remap[i] = next++;

                for (size_t i = 0; i < fn.blocks.size(); ++i)
                {
                    if (! reachable[i])
                        continue;

                    for (auto& inst : fn.blocks[i].insts)
                    {
                        if (inst.op != YdspIrOp::phi)
                            continue;

                        inst.phiIncoming.erase (std::remove_if (inst.phiIncoming.begin(), inst.phiIncoming.end(), [&remap] (const auto& entry)
                        {
                            return entry.first < 0 || static_cast<size_t> (entry.first) >= remap.size()
                                || remap[static_cast<size_t> (entry.first)] < 0;
                        }), inst.phiIncoming.end());

                        for (auto& entry : inst.phiIncoming)
                            entry.first = remap[static_cast<size_t> (entry.first)];
                    }
                }
            }
        }

        removeUnreachableBlocks (fn);
    }

    return anyChanged || foldedBranch;
}
} // namespace yup
