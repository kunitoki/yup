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

namespace
{

constexpr int32_t aarch64ImmediateLimit = 4095;

void addGpImm (asmjit::a64::Compiler& cc, const asmjit::a64::Gp& dst, const asmjit::a64::Gp& src, int32_t imm)
{
    if (imm == 0)
    {
        cc.mov (dst, src);
        return;
    }

    asmjit::a64::Gp tmp = cc.new_gp64 ("imm");
    cc.mov (tmp, asmjit::Imm (imm));
    cc.add (dst, src, tmp);
}

asmjit::arm::CondCode toArmCondition (asmjit::arm::CondCode cond) noexcept
{
    switch (cond)
    {
        case asmjit::arm::CondCode::kEqual:
            return asmjit::arm::CondCode::kEQ;
        case asmjit::arm::CondCode::kNotEqual:
            return asmjit::arm::CondCode::kNE;
        case asmjit::arm::CondCode::kUnsignedLT:
            return asmjit::arm::CondCode::kLO;
        case asmjit::arm::CondCode::kUnsignedLE:
            return asmjit::arm::CondCode::kLS;
        case asmjit::arm::CondCode::kUnsignedGT:
            return asmjit::arm::CondCode::kGT;
        case asmjit::arm::CondCode::kUnsignedGE:
            return asmjit::arm::CondCode::kGE;
        case asmjit::arm::CondCode::kSignedLT:
            return asmjit::arm::CondCode::kLT;
        case asmjit::arm::CondCode::kSignedLE:
            return asmjit::arm::CondCode::kLE;
        case asmjit::arm::CondCode::kSignedGT:
            return asmjit::arm::CondCode::kGT;
        case asmjit::arm::CondCode::kSignedGE:
            return asmjit::arm::CondCode::kGE;
        default:
            return asmjit::arm::CondCode::kEQ;
    }
}

asmjit::Imm conditionImm (asmjit::arm::CondCode cond) noexcept
{
    return asmjit::Imm (static_cast<uint32_t> (toArmCondition (cond)));
}

} // namespace

//==============================================================================

YdspAsmJitCodegenARM64::YdspFp YdspAsmJitCodegenARM64::newFp (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32, name);
}

YdspAsmJitCodegenARM64::YdspFp YdspAsmJitCodegenARM64::newFp64 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat64, name);
}

YdspAsmJitCodegenARM64::YdspFp YdspAsmJitCodegenARM64::newFpVector (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x4, name);
}

//==============================================================================

YdspAsmJitCodegenARM64::YdspMem YdspAsmJitCodegenARM64::memPtr (const YdspGp& base, int32_t offset) const
{
    return asmjit::a64::ptr (base, offset);
}

YdspAsmJitCodegenARM64::YdspMem YdspAsmJitCodegenARM64::memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const
{
    (void) offset;
    return asmjit::a64::ptr (base, index, asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, static_cast<int> (scaleLog2)));
}

void YdspAsmJitCodegenARM64::prepareStateAddressing (const YdspIrFunction& fn)
{
    stateArrayBaseRegs.clear();

    const auto reserve = [this] (int value, int region)
    {
        if (value < 0 || static_cast<size_t> (value) >= valueTypes.size())
            return;

        const auto base = stateArrayBase (valueTypes[static_cast<size_t> (value)], region);

        if (stateArrayBaseRegs.count (base) != 0)
            return;

        if (base == 0)
        {
            stateArrayBaseRegs.emplace (base, stateArraysReg);
            return;
        }

        YdspGp addr = cc->new_gp64 ("arrayBase");
        addGpImm (*cc, addr, stateArraysReg, base);
        stateArrayBaseRegs.emplace (base, addr);
    };

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        for (size_t i = 0; i < fn.blocks[b].insts.size(); ++i)
        {
            if (arrayDisplacements.count ({ static_cast<int> (b), static_cast<int> (i) }) != 0)
                continue;
            const auto& inst = fn.blocks[b].insts[i];
            switch (inst.op)
            {
                case YdspIrOp::loadStateArrayF:
                case YdspIrOp::loadStateArrayI:
                    reserve (inst.result, inst.memIndex);
                    break;

                case YdspIrOp::storeStateArrayF:
                case YdspIrOp::storeStateArrayI:
                    reserve (inst.b, inst.memIndex);
                    break;

                default:
                    break;
            }
        }
    }
}

YdspAsmJitCodegenARM64::YdspMem YdspAsmJitCodegenARM64::emitStateMem (YdspValueType type, int base, int indexValue)
{
    const uint32_t scale = is64BitValueType (type) ? 3u : 2u;

    if (indexValue < 0)
    {
        const int32_t maxOffset = is64BitValueType (type) ? 32760 : 16380;

        if (base > maxOffset)
        {
            YdspGp addr = cc->new_gp64 ("addr");
            addGpImm (*cc, addr, stateReg, base);
            return asmjit::a64::ptr (addr, 0);
        }

        return asmjit::a64::ptr (stateReg, base);
    }

    const auto shift = asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, static_cast<int> (scale));

    if (const auto it = stateArrayBaseRegs.find (base); it != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (it->second, gp (indexValue), shift);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    return asmjit::a64::ptr (addr, gp (indexValue), shift);
}

YdspAsmJitCodegenARM64::YdspMem YdspAsmJitCodegenARM64::emitVectorStateMem (int base, int indexValue)
{
    auto scaled = vectorIndexRegs.find (indexValue);

    if (scaled == vectorIndexRegs.end())
    {
        YdspGp offset = cc->new_gp64 ("vecIndex");
        cc->sxtw (offset, gp (indexValue));
        cc->lsl (offset, offset, asmjit::Imm (2));

        scaled = vectorIndexRegs.emplace (indexValue, offset).first;
    }

    if (const auto region = stateArrayBaseRegs.find (base); region != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (region->second, scaled->second);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    return asmjit::a64::ptr (addr, scaled->second);
}

YdspAsmJitCodegenARM64::YdspMem YdspAsmJitCodegenARM64::emitVectorStreamMem (const YdspGp& base, int indexValue)
{
    auto scaled = vectorIndexRegs.find (indexValue);

    if (scaled == vectorIndexRegs.end())
    {
        YdspGp offset = cc->new_gp64 ("vecIndex");
        cc->sxtw (offset, gp (indexValue));
        cc->lsl (offset, offset, asmjit::Imm (2));

        scaled = vectorIndexRegs.emplace (indexValue, offset).first;
    }

    return asmjit::a64::ptr (base, scaled->second);
}

void YdspAsmJitCodegenARM64::beginBlock (int blockIndex)
{
    (void) blockIndex;
    vectorIndexRegs.clear();
}

void YdspAsmJitCodegenARM64::onValueRedefined (int value)
{
    vectorIndexRegs.erase (value);
}

//==============================================================================

void YdspAsmJitCodegenARM64::loadGpFromMem (const YdspGp& dst, const YdspMem& src)
{
    cc->ldr (dst, src);
}

void YdspAsmJitCodegenARM64::storeGpToMem (const YdspMem& dst, const YdspGp& src)
{
    cc->str (src, dst);
}

void YdspAsmJitCodegenARM64::loadFloatFromMem (const YdspFp& dst, const YdspMem& src)
{
    cc->ldr (dst, src);
}

void YdspAsmJitCodegenARM64::storeFloatToMem (const YdspMem& dst, const YdspFp& src)
{
    cc->str (src, dst);
}

void YdspAsmJitCodegenARM64::moveFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->fmov (dst, src);
}

//==============================================================================

void YdspAsmJitCodegenARM64::floatBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    switch (op)
    {
        case YdspIrOp::addF:
            cc->fadd (dst, srcA, srcB);
            break;
        case YdspIrOp::subF:
            cc->fsub (dst, srcA, srcB);
            break;
        case YdspIrOp::mulF:
            cc->fmul (dst, srcA, srcB);
            break;
        case YdspIrOp::divF:
            cc->fdiv (dst, srcA, srcB);
            break;
        case YdspIrOp::minF:
            cc->fmin (dst, srcA, srcB);
            break;
        case YdspIrOp::maxF:
            cc->fmax (dst, srcA, srcB);
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::emitFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    cc->fmadd (dst, a, b, c);
}

void YdspAsmJitCodegenARM64::emitFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    cc->fmsub (dst, a, b, c);
}

void YdspAsmJitCodegenARM64::emitVectorFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst != c)
        moveVector (dst, c);
    cc->fmla (dst.s4(), a.s4(), b.s4());
}

void YdspAsmJitCodegenARM64::emitVectorFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst != c)
        moveVector (dst, c);
    cc->fmls (dst.s4(), a.s4(), b.s4());
}

void YdspAsmJitCodegenARM64::floatUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src, YdspValueType type)
{
    (void) type;

    switch (op)
    {
        case YdspIrOp::sqrtF:
            cc->fsqrt (dst, src);
            break;
        case YdspIrOp::negF:
            cc->fneg (dst, src);
            break;
        case YdspIrOp::absF:
            cc->fabs (dst, src);
            break;
        case YdspIrOp::floorF:
            cc->frintm (dst, src);
            break;
        case YdspIrOp::ceilF:
            cc->frintp (dst, src);
            break;
        case YdspIrOp::rintF:
            cc->frintn (dst, src);
            break;
        default:
            break;
    }
}

//==============================================================================

void YdspAsmJitCodegenARM64::loadVectorFromMem (const YdspFp& dst, const YdspMem& src)
{
    cc->ldr (dst.q(), src);
}

void YdspAsmJitCodegenARM64::storeVectorToMem (const YdspMem& dst, const YdspFp& src)
{
    cc->str (src.q(), dst);
}

void YdspAsmJitCodegenARM64::moveVector (const YdspFp& dst, const YdspFp& src)
{
    cc->mov (dst.b16(), src.b16());
}

void YdspAsmJitCodegenARM64::vectorBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    floatBinary (op, dst.s4(), srcA.s4(), srcB.s4());
}

void YdspAsmJitCodegenARM64::vectorUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src)
{
    floatUnary (op, dst.s4(), src.s4(), YdspValueType::float32Type);
}

void YdspAsmJitCodegenARM64::vectorFloatCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    switch (op)
    {
        case YdspIrOp::eqF:
            cc->fcmeq (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::neF:
            cc->fcmeq (dst.s4(), srcA.s4(), srcB.s4());
            cc->not_ (dst.b16(), dst.b16());
            break;
        case YdspIrOp::ltF:
            cc->fcmgt (dst.s4(), srcB.s4(), srcA.s4());
            break;
        case YdspIrOp::leF:
            cc->fcmge (dst.s4(), srcB.s4(), srcA.s4());
            break;
        case YdspIrOp::gtF:
            cc->fcmgt (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::geF:
            cc->fcmge (dst.s4(), srcA.s4(), srcB.s4());
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::vectorSelectFloat (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    moveVector (dst, mask);
    cc->bsl (dst.b16(), whenTrue.b16(), whenFalse.b16());
}

void YdspAsmJitCodegenARM64::emitSplatFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->fmov (dst.s(), src);
    cc->dup (dst.s4(), dst.s (0));
}

void YdspAsmJitCodegenARM64::emitReduceAddFloat (const YdspFp& dst, const YdspFp& src)
{
    YdspFp pairs = newFpVector ("reducePairs");

    cc->faddp (pairs.s4(), src.s4(), src.s4());
    cc->faddp (dst, pairs.s2());
}

void YdspAsmJitCodegenARM64::roundFloat (const YdspFp& reg, int mode)
{
    switch (mode)
    {
        case 0:
            cc->frintn (reg, reg);
            break; // nearest
        case 1:
            cc->frintm (reg, reg);
            break; // floor
        case 2:
            cc->frintp (reg, reg);
            break; // ceil
        case 3:
            cc->frintz (reg, reg);
            break; // trunc
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::emitSaturatingIntBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB)
{
    jassert (op == YdspIrOp::addI || op == YdspIrOp::subI || op == YdspIrOp::mulI);
    const auto limit = integerSaturationLimit (srcA, srcB, op == YdspIrOp::mulI);
    const auto result = dst.is_gp64() ? cc->new_gp64 ("arithmetic") : cc->new_gp32 ("arithmetic");
    auto condition = asmjit::arm::CondCode::kVS;
    if (op == YdspIrOp::addI)
        cc->adds (result, srcA, srcB);
    else if (op == YdspIrOp::subI)
        cc->subs (result, srcA, srcB);
    else
    {
        const auto productHigh = cc->new_gp64 ("productHigh");
        const auto signExtended = cc->new_gp64 ("productSign");
        if (dst.is_gp64())
        {
            cc->smulh (productHigh, srcA, srcB);
            cc->mul (result, srcA, srcB);
            cc->asr (signExtended, result, asmjit::Imm (63));
        }
        else
        {
            cc->smull (productHigh, srcA, srcB);
            cc->mov (result, productHigh.w());
            cc->sxtw (signExtended, result);
        }
        cc->cmp (productHigh, signExtended);
        condition = asmjit::arm::CondCode::kNE;
    }
    cc->csel (dst, limit, result, asmjit::Imm (static_cast<uint32_t> (condition)));
}

void YdspAsmJitCodegenARM64::intBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB)
{
    switch (op)
    {
        case YdspIrOp::addI:
            cc->add (dst, srcA, srcB);
            break;
        case YdspIrOp::subI:
            cc->sub (dst, srcA, srcB);
            break;
        case YdspIrOp::mulI:
            cc->mul (dst, srcA, srcB);
            break;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            cc->and_ (dst, srcA, srcB);
            break;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            cc->orr (dst, srcA, srcB);
            break;
        case YdspIrOp::xorI:
            cc->eor (dst, srcA, srcB);
            break;
        case YdspIrOp::shlI:
            cc->lsl (dst, srcA, srcB);
            break;
        case YdspIrOp::shrI:
            cc->asr (dst, srcA, srcB);
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::intUnaryNeg (const YdspGp& dst, const YdspGp& src)
{
    cc->neg (dst, src);
}

void YdspAsmJitCodegenARM64::emitIntDivision (YdspIrOp op, const YdspGp& dst, const YdspGp& a, const YdspGp& b, bool is64)
{
    asmjit::Label zeroLabel = cc->new_label();
    asmjit::Label overflowLabel = cc->new_label();
    asmjit::Label divideLabel = cc->new_label();
    asmjit::Label doneLabel = cc->new_label();

    branchIfZero (b, zeroLabel);

    cc->cmn (b, asmjit::Imm (1));
    cc->b (asmjit::arm::CondCode::kNE, divideLabel);

    YdspGp limit = is64 ? cc->new_gp64 ("intMin") : cc->new_gp32 ("intMin");

    if (is64)
        cc->mov (limit, asmjit::Imm (std::numeric_limits<int64_t>::min()));
    else
        cc->mov (limit, asmjit::Imm (0x80000000u));

    cc->cmp (a, limit);
    cc->b (asmjit::arm::CondCode::kEQ, overflowLabel);

    cc->bind (divideLabel);

    YdspGp quotient = is64 ? cc->new_gp64 ("quot") : cc->new_gp32 ("quot");
    cc->sdiv (quotient, a, b);

    if (op == YdspIrOp::divI)
    {
        moveGp (dst, quotient);
    }
    else
    {
        cc->msub (dst, quotient, b, a);
    }

    jump (doneLabel);

    cc->bind (overflowLabel);
    cc->mov (dst, asmjit::Imm (op == YdspIrOp::divI
                                     ? (is64 ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max())
                                     : 0));
    jump (doneLabel);

    cc->bind (zeroLabel);
    cc->mov (dst, asmjit::Imm (0));

    cc->bind (doneLabel);
}

void YdspAsmJitCodegenARM64::emitNotB (const YdspGp& dst, const YdspGp& src)
{
    YdspGp one = cc->new_gp32 ("one");
    cc->mov (one, asmjit::Imm (1));

    cc->eor (dst, src, one);
}

//==============================================================================

void YdspAsmJitCodegenARM64::emitFloatCompare (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst)
{
    cc->fcmp (a, b);
    cc->cset (dst, conditionImm (cond));
}

void YdspAsmJitCodegenARM64::emitIntCompare (YdspCond cond, const YdspGp& a, const YdspGp& b, const YdspGp& dst)
{
    cc->cmp (a, b);
    cc->cset (dst, conditionImm (cond));
}

void YdspAsmJitCodegenARM64::emitFloatCompareToReg (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst)
{
    emitFloatCompare (cond, a, b, dst);
}

//==============================================================================

void YdspAsmJitCodegenARM64::emitSelectFloat (const YdspGp& cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    cc->cmp (cond, asmjit::Imm (0));
    cc->fcsel (dst, whenTrue, whenFalse, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kNE)));
}

void YdspAsmJitCodegenARM64::emitSelectInt (const YdspGp& cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse)
{
    cc->cmp (cond, asmjit::Imm (0));
    cc->csel (dst, whenTrue, whenFalse, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kNE)));
}

void YdspAsmJitCodegenARM64::emitFloatCompareToFlags (const YdspFp& a, const YdspFp& b)
{
    cc->fcmp (a, b);
}

void YdspAsmJitCodegenARM64::emitIntCompareToFlags (const YdspGp& a, const YdspGp& b)
{
    cc->cmp (a, b);
}

void YdspAsmJitCodegenARM64::emitSelectFloatOnFlags (YdspCond cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    cc->fcsel (dst, whenTrue, whenFalse, conditionImm (cond));
}

void YdspAsmJitCodegenARM64::emitSelectIntOnFlags (YdspCond cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse)
{
    cc->csel (dst, whenTrue, whenFalse, conditionImm (cond));
}

void YdspAsmJitCodegenARM64::emitWrapInt (const YdspGp& dst, const YdspGp& value, const YdspGp& bound)
{
    const auto zero = value.is_gp64() ? asmjit::a64::xzr : asmjit::a64::wzr;

    cc->cmp (value, bound);
    cc->csel (dst, value, zero, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kLT)));
}

void YdspAsmJitCodegenARM64::emitAdvanceWrapInt (const YdspGp& dst, const YdspGp& value, int32_t bound)
{
    if (bound > 0 && bound <= aarch64ImmediateLimit)
        cc->add (dst, value, asmjit::Imm (1));
    else
    {
        YdspGp boundReg = cc->new_gp32 ("wrapBound");
        cc->mov (boundReg, asmjit::Imm (bound));
        cc->add (dst, value, asmjit::Imm (1));
        cc->cmp (dst, boundReg);
        cc->csel (dst, dst, asmjit::a64::wzr, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kLT)));
        return;
    }
    cc->cmp (dst, asmjit::Imm (bound));
    cc->csel (dst, dst, asmjit::a64::wzr, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kLT)));
}

//==============================================================================

void YdspAsmJitCodegenARM64::emitIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    cc->scvtf (dst, src);
}

void YdspAsmJitCodegenARM64::emitFloatToInt (const YdspGp& dst, const YdspFp& src, bool)
{
    cc->fcvtzs (dst, src);
}

void YdspAsmJitCodegenARM64::emitExtendInt (const YdspGp& dst, const YdspGp& src)
{
    cc->sxtw (dst, src.w());
}

void YdspAsmJitCodegenARM64::emitTruncateInt (const YdspGp& dst, const YdspGp& src)
{
    cc->mov (dst.w(), src.w());
}

void YdspAsmJitCodegenARM64::emitExtendFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->fcvt (dst, src);
}

void YdspAsmJitCodegenARM64::emitTruncateFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->fcvt (dst, src);
}

//==============================================================================

void YdspAsmJitCodegenARM64::jump (const asmjit::Label& target)
{
    cc->b (target);
}

void YdspAsmJitCodegenARM64::branchIfZero (const YdspGp& cond, const asmjit::Label& target)
{
    cc->cbz (cond, target);
}

void YdspAsmJitCodegenARM64::branchIfNotZero (const YdspGp& cond, const asmjit::Label& target)
{
    cc->cbnz (cond, target);
}

void YdspAsmJitCodegenARM64::branchOnFlags (YdspCond cond, bool branchWhenTrue, const asmjit::Label& target)
{
    if (! branchWhenTrue)
        cond = static_cast<YdspCond> (static_cast<uint8_t> (cond) ^ uint8_t (1));

    cc->b (cond, target);
}

} // namespace yup
