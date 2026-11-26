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

void YdspAsmJitCodegenX64::configureTarget (const asmjit::Environment&, const YdspIrFunction& fn)
{
    const bool hasFmaInst = std::any_of (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return std::any_of (block.insts.begin(), block.insts.end(), [] (const YdspIrInst& inst)
        {
            return inst.op == YdspIrOp::fmaF || inst.op == YdspIrOp::fmsubF;
        });
    });

    hasAvx = activeVectorWidth > 4 || fn.packedFmaAvailable || hasFmaInst;
    hasSse4_1 = hasAvx;
}

bool YdspAsmJitCodegenX64::emitZeroFloatConst (const YdspFp& dst, bool is64)
{
    if (hasAvx)
    {
        if (is64)
            cc->vxorpd (dst, dst, dst);
        else
            cc->vxorps (dst, dst, dst);
    }
    else
    {
        if (is64)
            cc->xorpd (dst, dst);
        else
            cc->xorps (dst, dst);
    }

    return true;
}

bool YdspAsmJitCodegenX64::isDoubleFloat (const YdspFp& reg) const
{
    if (! cc->is_virt_reg_valid (reg))
        return false;

    const auto type = cc->virt_reg_by_reg (reg)->type_id();
    return type == asmjit::TypeId::kFloat64 || type == asmjit::TypeId::kFloat64x1;
}

bool YdspAsmJitCodegenX64::isIntVector (const YdspFp& reg) const
{
    if (! cc->is_virt_reg_valid (reg))
        return false;

    const auto type = cc->virt_reg_by_reg (reg)->type_id();
    return type == asmjit::TypeId::kInt32x4
        || type == asmjit::TypeId::kInt32x8
        || type == asmjit::TypeId::kInt32x16;
}

//==============================================================================

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newFp (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x1, name);
}

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newFp64 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat64x1, name);
}

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newFp128 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x4, name);
}

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newFpVector (const char* name)
{
    if (activeVectorWidth == 16)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x16, name);

    if (activeVectorWidth == 8)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x8, name);

    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x4, name);
}

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newInt128 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kInt32x4, name);
}

YdspAsmJitCodegenX64::YdspFp YdspAsmJitCodegenX64::newIntVector (const char* name)
{
    if (activeVectorWidth == 16)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kInt32x16, name);

    if (activeVectorWidth == 8)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kInt32x8, name);

    return cc->new_reg<YdspFp> (asmjit::TypeId::kInt32x4, name);
}

//==============================================================================

YdspAsmJitCodegenX64::YdspMem YdspAsmJitCodegenX64::memPtr (const YdspGp& base, int32_t offset) const
{
    return asmjit::x86::ptr (base, offset);
}

YdspAsmJitCodegenX64::YdspMem YdspAsmJitCodegenX64::memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const
{
    return asmjit::x86::ptr (base, index, scaleLog2, offset);
}

YdspAsmJitCodegenX64::YdspMem YdspAsmJitCodegenX64::emitStateMem (YdspValueType type, int base, int indexValue)
{
    const uint32_t scale = is64BitValueType (type) ? 3u : 2u;

    if (indexValue < 0)
        return asmjit::x86::ptr (stateReg, base);

    return asmjit::x86::ptr (stateArraysReg, gp (indexValue), scale, base);
}

YdspAsmJitCodegenX64::YdspMem YdspAsmJitCodegenX64::emitVectorStateMem (int base, int indexValue)
{
    return asmjit::x86::ptr (stateArraysReg, gp (indexValue), 2u, base);
}

YdspAsmJitCodegenX64::YdspMem YdspAsmJitCodegenX64::emitVectorStreamMem (const YdspGp& base, int indexValue)
{
    return asmjit::x86::ptr (base, gp (indexValue), 2u, 0);
}

//==============================================================================

void YdspAsmJitCodegenX64::loadGpFromMem (const YdspGp& dst, const YdspMem& src)
{
    cc->mov (dst, src);
}

void YdspAsmJitCodegenX64::storeGpToMem (const YdspMem& dst, const YdspGp& src)
{
    cc->mov (dst, src);
}

void YdspAsmJitCodegenX64::loadFloatFromMem (const YdspFp& dst, const YdspMem& src)
{
    if (isDoubleFloat (dst))
        cc->movsd (dst, src);
    else
        cc->movss (dst, src);
}

void YdspAsmJitCodegenX64::storeFloatToMem (const YdspMem& dst, const YdspFp& src)
{
    if (isDoubleFloat (src))
        cc->movsd (dst, src);
    else
        cc->movss (dst, src);
}

void YdspAsmJitCodegenX64::moveFloat (const YdspFp& dst, const YdspFp& src)
{
    if (isDoubleFloat (dst))
        cc->movsd (dst, src);
    else
        cc->movss (dst, src);
}

//==============================================================================

void YdspAsmJitCodegenX64::floatBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    const bool is64 = isDoubleFloat (dst);

    // The SSE forms compute into their left operand: with the destination as
    // the right one (`y = a - y`), loading the left into it first would lose it.
    if (dst == srcB && dst != srcA)
    {
        if (op == YdspIrOp::addF || op == YdspIrOp::mulF)
        {
            floatBinary (op, dst, srcB, srcA);
            return;
        }

        YdspFp result = is64 ? newFp64 ("binaryResult") : newFp ("binaryResult");
        floatBinary (op, result, srcA, srcB);
        moveFloat (dst, result);
        return;
    }

    if (dst != srcA)
        moveFloat (dst, srcA);

    if (is64)
    {
        switch (op)
        {
            case YdspIrOp::addF:
                cc->addsd (dst, srcB);
                break;
            case YdspIrOp::subF:
                cc->subsd (dst, srcB);
                break;
            case YdspIrOp::mulF:
                cc->mulsd (dst, srcB);
                break;
            case YdspIrOp::divF:
                cc->divsd (dst, srcB);
                break;
            case YdspIrOp::minF:
                cc->minsd (dst, srcB);
                break;
            case YdspIrOp::maxF:
                cc->maxsd (dst, srcB);
                break;
            default:
                break;
        }
    }
    else
    {
        switch (op)
        {
            case YdspIrOp::addF:
                cc->addss (dst, srcB);
                break;
            case YdspIrOp::subF:
                cc->subss (dst, srcB);
                break;
            case YdspIrOp::mulF:
                cc->mulss (dst, srcB);
                break;
            case YdspIrOp::divF:
                cc->divss (dst, srcB);
                break;
            case YdspIrOp::minF:
                cc->minss (dst, srcB);
                break;
            case YdspIrOp::maxF:
                cc->maxss (dst, srcB);
                break;
            default:
                break;
        }
    }
}

void YdspAsmJitCodegenX64::emitFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    const bool is64 = isDoubleFloat (dst);

    // VEX FMA reads every source before writing the destination, so an aliased
    // destination can be used in place - as long as the form picked does not
    // need a destructive preload that would clobber a live source.
    if (dst == b)
    {
        if (is64)
            cc->vfmadd213sd (dst, a, c); // dst = dst * a + c
        else
            cc->vfmadd213ss (dst, a, c);
        return;
    }

    if (dst == c)
    {
        if (is64)
            cc->vfmadd231sd (dst, a, b); // dst = a * b + dst
        else
            cc->vfmadd231ss (dst, a, b);
        return;
    }

    moveFloat (dst, a);

    if (is64)
        cc->vfmadd213sd (dst, b, c);
    else
        cc->vfmadd213ss (dst, b, c);
}

void YdspAsmJitCodegenX64::emitFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst == a || dst == b)
    {
        const auto other = dst == a ? b : a;
        if (isDoubleFloat (dst))
            cc->vfnmadd213sd (dst, other, c);
        else
            cc->vfnmadd213ss (dst, other, c);
        return;
    }

    if (dst != c)
        moveFloat (dst, c);

    if (isDoubleFloat (dst))
        cc->vfnmadd231sd (dst, a, b);
    else
        cc->vfnmadd231ss (dst, a, b);
}

void YdspAsmJitCodegenX64::emitVectorFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst == c)
    {
        cc->vfmadd231ps (dst, a, b);
        return;
    }

    if (dst == b)
    {
        cc->vfmadd213ps (dst, a, c);
        return;
    }

    if (dst != a)
        moveVector (dst, a);

    cc->vfmadd213ps (dst, b, c);
}

void YdspAsmJitCodegenX64::emitVectorFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst == a || dst == b)
    {
        cc->vfnmadd213ps (dst, dst == a ? b : a, c);
        return;
    }

    if (dst != c)
        moveVector (dst, c);

    cc->vfnmadd231ps (dst, a, b);
}

void YdspAsmJitCodegenX64::floatUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src, YdspValueType type)
{
    (void) type; // the register's own type id carries the width

    const bool is64 = isDoubleFloat (dst);

    switch (op)
    {
        case YdspIrOp::sqrtF:
            if (is64)
                cc->sqrtsd (dst, src);
            else
                cc->sqrtss (dst, src);
            break;
        case YdspIrOp::negF:
        {
            moveFloat (dst, src);

            if (is64)
                cc->xorpd (dst, packedConstMem (0x8000000000000000ull, true));
            else
                cc->xorps (dst, packedConstMem (0x80000000ull, false));

            break;
        }
        case YdspIrOp::absF:
        {
            moveFloat (dst, src);

            if (is64)
                cc->andpd (dst, packedConstMem (0x7FFFFFFFFFFFFFFFull, true));
            else
                cc->andps (dst, packedConstMem (0x7FFFFFFFull, false));

            break;
        }
        case YdspIrOp::rintF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        {
            const auto mode = op == YdspIrOp::rintF ? 0 : op == YdspIrOp::floorF ? 1
                                                                                 : 2;

            moveFloat (dst, src);
            roundFloat (dst, mode);
            break;
        }
        default:
            break;
    }
}

//==============================================================================

void YdspAsmJitCodegenX64::loadVectorFromMem (const YdspFp& dst, const YdspMem& src)
{
    if (isIntVector (dst))
    {
        if (hasAvx)
            cc->vmovdqu (dst, src);
        else
            cc->movdqu (dst, src);
        return;
    }

    if (hasAvx)
        cc->vmovups (dst, src);
    else
        cc->movups (dst, src);
}

void YdspAsmJitCodegenX64::storeVectorToMem (const YdspMem& dst, const YdspFp& src)
{
    if (isIntVector (src))
    {
        if (hasAvx)
            cc->vmovdqu (dst, src);
        else
            cc->movdqu (dst, src);
        return;
    }

    if (hasAvx)
        cc->vmovups (dst, src);
    else
        cc->movups (dst, src);
}

void YdspAsmJitCodegenX64::moveVector (const YdspFp& dst, const YdspFp& src)
{
    if (isIntVector (dst))
    {
        if (hasAvx)
            cc->vmovdqa (dst, src);
        else
            cc->movdqa (dst, src);
        return;
    }

    if (hasAvx)
        cc->vmovaps (dst, src);
    else
        cc->movaps (dst, src);
}

void YdspAsmJitCodegenX64::vectorBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    if (hasAvx)
    {
        switch (op)
        {
            case YdspIrOp::addF:
                cc->vaddps (dst, srcA, srcB);
                break;
            case YdspIrOp::subF:
                cc->vsubps (dst, srcA, srcB);
                break;
            case YdspIrOp::mulF:
                cc->vmulps (dst, srcA, srcB);
                break;
            case YdspIrOp::divF:
                cc->vdivps (dst, srcA, srcB);
                break;
            case YdspIrOp::minF:
                cc->vminps (dst, srcA, srcB);
                break;
            case YdspIrOp::maxF:
                cc->vmaxps (dst, srcA, srcB);
                break;
            default:
                break;
        }

        return;
    }

    if (dst == srcB && dst != srcA)
    {
        YdspFp result = newFpVector ("binaryResult");
        vectorBinary (op, result, srcA, srcB);
        moveVector (dst, result);
        return;
    }

    if (dst != srcA)
        moveVector (dst, srcA);

    switch (op)
    {
        case YdspIrOp::addF:
            cc->addps (dst, srcB);
            break;
        case YdspIrOp::subF:
            cc->subps (dst, srcB);
            break;
        case YdspIrOp::mulF:
            cc->mulps (dst, srcB);
            break;
        case YdspIrOp::divF:
            cc->divps (dst, srcB);
            break;
        case YdspIrOp::minF:
            cc->minps (dst, srcB);
            break;
        case YdspIrOp::maxF:
            cc->maxps (dst, srcB);
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenX64::vectorUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src)
{
    if (hasAvx)
    {
        switch (op)
        {
            case YdspIrOp::sqrtF:
                cc->vsqrtps (dst, src);
                break;

            case YdspIrOp::negF:
                cc->vxorps (dst, src, vectorConstMem (0x80000000u));
                break;

            case YdspIrOp::absF:
                cc->vandps (dst, src, vectorConstMem (0x7fffffffu));
                break;

            case YdspIrOp::rintF:
                cc->vroundps (dst, src, asmjit::Imm (8));
                break;

            case YdspIrOp::floorF:
                cc->vroundps (dst, src, asmjit::Imm (9));
                break;

            case YdspIrOp::ceilF:
                cc->vroundps (dst, src, asmjit::Imm (10));
                break;

            default:
                break;
        }

        return;
    }

    switch (op)
    {
        case YdspIrOp::sqrtF:
            cc->sqrtps (dst, src);
            break;

        case YdspIrOp::rintF:
            cc->roundps (dst, src, asmjit::Imm (8));
            break;

        case YdspIrOp::floorF:
            cc->roundps (dst, src, asmjit::Imm (9));
            break;

        case YdspIrOp::ceilF:
            cc->roundps (dst, src, asmjit::Imm (10));
            break;

        case YdspIrOp::negF:
            if (dst != src)
                moveVector (dst, src);
            cc->xorps (dst, packedConstMem (0x80000000ull, false));
            break;

        case YdspIrOp::absF:
            if (dst != src)
                moveVector (dst, src);
            cc->andps (dst, packedConstMem (0x7FFFFFFFull, false));
            break;

        default:
            break;
    }
}

void YdspAsmJitCodegenX64::vectorFloatCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    if (hasAvx)
    {
        uint32_t predicate = 0;

        switch (op)
        {
            case YdspIrOp::eqF:
                predicate = 0x00;
                break;
            case YdspIrOp::neF:
                predicate = 0x0c;
                break;
            case YdspIrOp::ltF:
                predicate = 0x11;
                break;
            case YdspIrOp::leF:
                predicate = 0x12;
                break;
            case YdspIrOp::gtF:
                predicate = 0x1e;
                break;
            case YdspIrOp::geF:
                predicate = 0x1d;
                break;
            default:
                return;
        }

        cc->vcmpps (dst, srcA, srcB, asmjit::Imm (predicate));
        return;
    }

    uint32_t predicate = 0;
    bool swapOperands = false;

    switch (op)
    {
        case YdspIrOp::eqF:
            predicate = 0;
            break;
        case YdspIrOp::ltF:
            predicate = 1;
            break;
        case YdspIrOp::leF:
            predicate = 2;
            break;
        case YdspIrOp::neF:
            predicate = 4;
            break;
        case YdspIrOp::gtF:
            predicate = 1;
            swapOperands = true;
            break;
        case YdspIrOp::geF:
            predicate = 2;
            swapOperands = true;
            break;
        default:
            return;
    }

    const auto& lhs = swapOperands ? srcB : srcA;
    if (dst != lhs)
        moveVector (dst, lhs);
    cc->cmpps (dst, swapOperands ? srcA : srcB, asmjit::Imm (predicate));
}

void YdspAsmJitCodegenX64::vectorSelectFloat (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    if (hasAvx && activeVectorWidth <= 8)
    {
        cc->vblendvps (dst, whenFalse, whenTrue, mask);
        return;
    }

    if (! hasAvx && hasSse4_1)
    {
        YdspFp target = (dst == whenTrue || dst == mask) ? newFpVector ("sel") : dst;
        if (target != whenFalse)
            moveVector (target, whenFalse);
        cc->blendvps (target, whenTrue, mask);

        if (target != dst)
            moveVector (dst, target);

        return;
    }

    YdspFp selected = newFpVector ("sel");
    moveVector (selected, mask);
    if (hasAvx)
        cc->vandps (selected, selected, whenTrue);
    else
        cc->andps (selected, whenTrue);

    YdspFp inverse = newFpVector ("selInverse");
    moveVector (inverse, mask);
    if (hasAvx)
    {
        cc->vandnps (inverse, inverse, whenFalse);
        cc->vorps (selected, selected, inverse);
    }
    else
    {
        cc->andnps (inverse, whenFalse);
        cc->orps (selected, inverse);
    }
    moveVector (dst, selected);
}

void YdspAsmJitCodegenX64::emitSplatFloat (const YdspFp& dst, const YdspFp& src)
{
    if (activeVectorWidth > 4)
    {
        cc->vbroadcastss (dst, src);
        return;
    }

    if (hasAvx)
    {
        cc->vshufps (dst, src, src, asmjit::Imm (0x00));
        return;
    }

    if (dst != src)
        moveVector (dst, src);
    cc->shufps (dst, dst, asmjit::Imm (0x00)); // every lane = lane 0
}

void YdspAsmJitCodegenX64::emitSplatInt (const YdspFp& dst, const YdspGp& src)
{
    if (activeVectorWidth > 4)
    {
        YdspFp tmp = newInt128 ("splatTmp");
        cc->vmovd (tmp, src);
        cc->vpbroadcastd (dst, tmp);
        return;
    }

    if (hasAvx)
    {
        cc->vmovd (dst, src);
        cc->vpshufd (dst, dst, asmjit::Imm (0x00));
        return;
    }

    cc->movd (dst, src);
    cc->pshufd (dst, dst, asmjit::Imm (0x00));
}

void YdspAsmJitCodegenX64::emitSaturatingVectorIntAddSub (bool isAdd, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    YdspFp sum = newIntVector ("satSum");
    YdspFp xorAB = newIntVector ("satXorAB");
    YdspFp xorASum = newIntVector ("satXorASum");
    YdspFp overflow = newIntVector ("satOverflow");
    YdspFp limit = newIntVector ("satLimit");

    const auto intMax = vectorConstMem (0x7FFFFFFFu);

    if (hasAvx && activeVectorWidth <= 8)
    {
        if (isAdd)
            cc->vpaddd (sum, srcA, srcB);
        else
            cc->vpsubd (sum, srcA, srcB);

        cc->vpxor (xorAB, srcA, srcB);
        cc->vpxor (xorASum, srcA, sum);

        if (isAdd)
            cc->vpandn (overflow, xorAB, xorASum);
        else
            cc->vpand (overflow, xorAB, xorASum);

        cc->vpsrad (overflow, overflow, asmjit::Imm (31));
        cc->vpsrad (limit, srcA, asmjit::Imm (31));
        cc->vpxor (limit, limit, intMax);
        cc->vpblendvb (dst, sum, limit, overflow);
        return;
    }

    cc->movdqa (sum, srcA);
    if (isAdd)
        cc->paddd (sum, srcB);
    else
        cc->psubd (sum, srcB);

    cc->movdqa (xorAB, srcA);
    cc->pxor (xorAB, srcB);

    cc->movdqa (xorASum, srcA);
    cc->pxor (xorASum, sum);

    if (isAdd)
    {
        cc->movdqa (overflow, xorAB);
        cc->pandn (overflow, xorASum);
    }
    else
    {
        cc->movdqa (overflow, xorAB);
        cc->pand (overflow, xorASum);
    }

    cc->psrad (overflow, asmjit::Imm (31));
    cc->movdqa (limit, srcA);
    cc->psrad (limit, asmjit::Imm (31));
    cc->pxor (limit, intMax);
    vectorSelectInt (overflow, dst, limit, sum);
}

void YdspAsmJitCodegenX64::emitSaturatingVectorIntMul (const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    YdspFp aOdd = newIntVector ("mulAOdd");
    YdspFp bOdd = newIntVector ("mulBOdd");
    YdspFp pEven = newIntVector ("mulPEven");
    YdspFp pOdd = newIntVector ("mulPOdd");
    YdspFp lo32 = newIntVector ("mulLo32");
    YdspFp hi32 = newIntVector ("mulHi32");
    YdspFp signLo = newIntVector ("mulSignLo");
    YdspFp ok = newIntVector ("mulOk");
    YdspFp limit = newIntVector ("mulLimit");

    const auto intMax = vectorConstMem (0x7FFFFFFFu);

    if (hasAvx && activeVectorWidth <= 8)
    {
        cc->vpmuldq (pEven, srcA, srcB);
        cc->vpshufd (aOdd, srcA, asmjit::Imm (0xF5));
        cc->vpshufd (bOdd, srcB, asmjit::Imm (0xF5));
        cc->vpmuldq (pOdd, aOdd, bOdd);

        cc->vshufps (lo32, pEven, pOdd, asmjit::Imm (0x88));
        cc->vpshufd (lo32, lo32, asmjit::Imm (0xD8));
        cc->vshufps (hi32, pEven, pOdd, asmjit::Imm (0xDD));
        cc->vpshufd (hi32, hi32, asmjit::Imm (0xD8));

        cc->vpsrad (signLo, lo32, asmjit::Imm (31));
        cc->vpcmpeqd (ok, hi32, signLo);

        cc->vpxor (limit, srcA, srcB);
        cc->vpsrad (limit, limit, asmjit::Imm (31));
        cc->vpxor (limit, limit, intMax);
        cc->vpblendvb (dst, limit, lo32, ok);
        return;
    }

    cc->movdqa (pEven, srcA);
    cc->pmuludq (pEven, srcB);
    cc->pshufd (aOdd, srcA, asmjit::Imm (0xF5));
    cc->pshufd (bOdd, srcB, asmjit::Imm (0xF5));
    cc->movdqa (pOdd, aOdd);
    cc->pmuludq (pOdd, bOdd);

    cc->movaps (lo32, pEven);
    cc->shufps (lo32, pOdd, asmjit::Imm (0x88));
    cc->pshufd (lo32, lo32, asmjit::Imm (0xD8));

    cc->movaps (hi32, pEven);
    cc->shufps (hi32, pOdd, asmjit::Imm (0xDD));
    cc->pshufd (hi32, hi32, asmjit::Imm (0xD8));

    // Convert unsigned 32x32->64 upper halves to signed upper halves:
    // hi_s = hi_u - ((srcA >> 31) & srcB) - ((srcB >> 31) & srcA)
    YdspFp corrA = newIntVector ("mulCorrA");
    YdspFp corrB = newIntVector ("mulCorrB");
    cc->movdqa (corrA, srcA);
    cc->psrad (corrA, asmjit::Imm (31));
    cc->pand (corrA, srcB);
    cc->movdqa (corrB, srcB);
    cc->psrad (corrB, asmjit::Imm (31));
    cc->pand (corrB, srcA);
    cc->paddd (corrA, corrB);
    cc->psubd (hi32, corrA);

    cc->movdqa (signLo, lo32);
    cc->psrad (signLo, asmjit::Imm (31));
    cc->movdqa (ok, hi32);
    cc->pcmpeqd (ok, signLo);

    cc->movdqa (limit, srcA);
    cc->pxor (limit, srcB);
    cc->psrad (limit, asmjit::Imm (31));
    cc->pxor (limit, intMax);
    vectorSelectInt (ok, dst, lo32, limit);
}

void YdspAsmJitCodegenX64::emitVectorIntMinMax (bool isMin, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    if (hasAvx)
    {
        if (isMin)
            cc->vpminsd (dst, srcA, srcB);
        else
            cc->vpmaxsd (dst, srcA, srcB);
        return;
    }

    YdspFp gtMask = newIntVector ("minMaxMask");
    cc->movdqa (gtMask, srcA);
    cc->pcmpgtd (gtMask, srcB);

    if (isMin)
        vectorSelectInt (gtMask, dst, srcB, srcA);
    else
        vectorSelectInt (gtMask, dst, srcA, srcB);
}

void YdspAsmJitCodegenX64::vectorIntBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB, bool saturating)
{
    if (saturating && (op == YdspIrOp::addI || op == YdspIrOp::subI))
    {
        emitSaturatingVectorIntAddSub (op == YdspIrOp::addI, dst, srcA, srcB);
        return;
    }

    if (saturating && op == YdspIrOp::mulI)
    {
        emitSaturatingVectorIntMul (dst, srcA, srcB);
        return;
    }

    if (op == YdspIrOp::minI || op == YdspIrOp::maxI)
    {
        emitVectorIntMinMax (op == YdspIrOp::minI, dst, srcA, srcB);
        return;
    }

    if (hasAvx && (activeVectorWidth > 4 || (op != YdspIrOp::shlI && op != YdspIrOp::shrI)))
    {
        switch (op)
        {
            case YdspIrOp::addI:
                cc->vpaddd (dst, srcA, srcB);
                break;
            case YdspIrOp::subI:
                cc->vpsubd (dst, srcA, srcB);
                break;
            case YdspIrOp::mulI:
                cc->vpmulld (dst, srcA, srcB);
                break;
            case YdspIrOp::andI:
            case YdspIrOp::andB:
                cc->vpand (dst, srcA, srcB);
                break;
            case YdspIrOp::orI:
            case YdspIrOp::orB:
                cc->vpor (dst, srcA, srcB);
                break;
            case YdspIrOp::xorI:
                cc->vpxor (dst, srcA, srcB);
                break;
            case YdspIrOp::shlI:
            case YdspIrOp::shrI:
            {
                YdspFp shiftVec = newIntVector ("vecShift");
                cc->vpand (shiftVec, srcB, vectorConstMem (31u));
                if (op == YdspIrOp::shlI)
                    cc->vpsllvd (dst, srcA, shiftVec);
                else
                    cc->vpsravd (dst, srcA, shiftVec);
                break;
            }
            default:
                break;
        }
        return;
    }

    if (op == YdspIrOp::mulI)
    {
        YdspFp t0 = newIntVector ("mulEven");
        YdspFp aOdd = newIntVector ("mulAOdd");
        YdspFp bOdd = newIntVector ("mulBOdd");
        cc->movdqa (t0, srcA);
        cc->pmuludq (t0, srcB);
        cc->pshufd (aOdd, srcA, asmjit::Imm (0xF5));
        cc->pshufd (bOdd, srcB, asmjit::Imm (0xF5));
        cc->pmuludq (aOdd, bOdd);
        cc->pshufd (t0, t0, asmjit::Imm (0x08));
        cc->pshufd (aOdd, aOdd, asmjit::Imm (0x08));
        cc->punpckldq (t0, aOdd);
        cc->movdqa (dst, t0);
        return;
    }

    if (op == YdspIrOp::shlI || op == YdspIrOp::shrI)
    {
        YdspMem scratch = cc->new_stack (32, 16, "shiftScratch");
        YdspMem scratchB = scratch;
        scratchB.add_offset (16);
        storeVectorToMem (scratch, srcA);
        storeVectorToMem (scratchB, srcB);

        YdspGp val = cc->new_gp32 ("shiftVal");
        YdspGp cnt = cc->new_gp32 ("shiftCnt");

        for (int lane = 0; lane < 4; ++lane)
        {
            YdspMem slotA = scratch;
            slotA.add_offset (lane * 4);
            YdspMem slotB = scratchB;
            slotB.add_offset (lane * 4);
            cc->mov (val, slotA);
            cc->mov (cnt, slotB);
            if (op == YdspIrOp::shlI)
                cc->shl (val, cnt);
            else
                cc->sar (val, cnt);
            cc->mov (slotA, val);
        }

        loadVectorFromMem (dst, scratch);
        return;
    }

    YdspFp target = (dst == srcB && dst != srcA) ? newIntVector ("intTmp") : dst;
    if (target != srcA)
        moveVector (target, srcA);

    switch (op)
    {
        case YdspIrOp::addI:
            cc->paddd (target, srcB);
            break;
        case YdspIrOp::subI:
            cc->psubd (target, srcB);
            break;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            cc->pand (target, srcB);
            break;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            cc->por (target, srcB);
            break;
        case YdspIrOp::xorI:
            cc->pxor (target, srcB);
            break;
        default:
            break;
    }

    if (target != dst)
        moveVector (dst, target);
}

void YdspAsmJitCodegenX64::vectorIntShift (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspGp& shiftAmount)
{
    YdspGp masked = cc->new_gp32 ("vecShift");
    cc->mov (masked, shiftAmount);
    cc->and_ (masked, asmjit::Imm (31));

    YdspFp shiftReg = newInt128 ("vecShiftXmm");

    if (hasAvx)
    {
        cc->vmovd (shiftReg, masked);
        if (op == YdspIrOp::shlI)
            cc->vpslld (dst, srcA, shiftReg);
        else
            cc->vpsrad (dst, srcA, shiftReg);
        return;
    }

    cc->movd (shiftReg, masked);
    if (dst != srcA)
        moveVector (dst, srcA);
    if (op == YdspIrOp::shlI)
        cc->pslld (dst, shiftReg);
    else
        cc->psrad (dst, shiftReg);
}

void YdspAsmJitCodegenX64::vectorIntUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src)
{
    if (op == YdspIrOp::notB)
    {
        YdspFp allOnes = newIntVector ("notMask");
        if (hasAvx)
        {
            cc->vpcmpeqd (allOnes, allOnes, allOnes);
            cc->vpxor (dst, src, allOnes);
        }
        else
        {
            cc->pcmpeqd (allOnes, allOnes);
            if (dst != src)
                moveVector (dst, src);
            cc->pxor (dst, allOnes);
        }
        return;
    }

    if (op == YdspIrOp::negI)
    {
        YdspFp res = newIntVector ("negRes");
        YdspFp fix = newIntVector ("negFix");
        const auto intMin = vectorConstMem (0x80000000u);

        if (hasAvx)
        {
            cc->vpxor (res, res, res);
            cc->vpsubd (res, res, src);
            cc->vpcmpeqd (fix, res, intMin);
            cc->vpaddd (dst, res, fix);
            return;
        }

        cc->pxor (res, res);
        cc->psubd (res, src);
        cc->movdqa (fix, res);
        cc->pcmpeqd (fix, intMin);
        cc->paddd (res, fix);
        moveVector (dst, res);
        return;
    }

    if (op == YdspIrOp::absI)
    {
        YdspFp mask = newIntVector ("absMask");
        YdspFp res = newIntVector ("absRes");
        YdspFp fix = newIntVector ("absFix");
        const auto intMin = vectorConstMem (0x80000000u);

        if (hasAvx)
        {
            cc->vpsrad (mask, src, asmjit::Imm (31));
            cc->vpxor (res, src, mask);
            cc->vpsubd (res, res, mask);
            cc->vpcmpeqd (fix, res, intMin);
            cc->vpaddd (dst, res, fix);
            return;
        }

        cc->movdqa (mask, src);
        cc->psrad (mask, asmjit::Imm (31));
        cc->movdqa (res, src);
        cc->pxor (res, mask);
        cc->psubd (res, mask);
        cc->movdqa (fix, res);
        cc->pcmpeqd (fix, intMin);
        cc->paddd (res, fix);
        moveVector (dst, res);
    }
}

void YdspAsmJitCodegenX64::vectorIntClamp (const YdspFp& dst, const YdspFp& src, const YdspFp& lo, const YdspFp& hi)
{
    YdspFp maxed = newIntVector ("clampMax");
    emitVectorIntMinMax (false, maxed, src, lo);
    emitVectorIntMinMax (true, dst, maxed, hi);
}

void YdspAsmJitCodegenX64::vectorIntCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    const auto invert = [this] (const YdspFp& target)
    {
        YdspFp allOnes = newIntVector ("cmpAllOnes");
        if (hasAvx)
        {
            cc->vpcmpeqd (allOnes, allOnes, allOnes);
            cc->vpxor (target, target, allOnes);
        }
        else
        {
            cc->pcmpeqd (allOnes, allOnes);
            cc->pxor (target, allOnes);
        }
    };

    if (hasAvx)
    {
        switch (op)
        {
            case YdspIrOp::eqI:
                cc->vpcmpeqd (dst, srcA, srcB);
                break;
            case YdspIrOp::neI:
                cc->vpcmpeqd (dst, srcA, srcB);
                invert (dst);
                break;
            case YdspIrOp::gtI:
                cc->vpcmpgtd (dst, srcA, srcB);
                break;
            case YdspIrOp::ltI:
                cc->vpcmpgtd (dst, srcB, srcA);
                break;
            case YdspIrOp::leI:
                cc->vpcmpgtd (dst, srcA, srcB);
                invert (dst);
                break;
            case YdspIrOp::geI:
                cc->vpcmpgtd (dst, srcB, srcA);
                invert (dst);
                break;
            default:
                break;
        }
        return;
    }

    YdspFp target = (dst == srcA || dst == srcB) ? newIntVector ("cmpTmp") : dst;

    switch (op)
    {
        case YdspIrOp::eqI:
            cc->movdqa (target, srcA);
            cc->pcmpeqd (target, srcB);
            break;
        case YdspIrOp::neI:
            cc->movdqa (target, srcA);
            cc->pcmpeqd (target, srcB);
            invert (target);
            break;
        case YdspIrOp::gtI:
            cc->movdqa (target, srcA);
            cc->pcmpgtd (target, srcB);
            break;
        case YdspIrOp::ltI:
            cc->movdqa (target, srcB);
            cc->pcmpgtd (target, srcA);
            break;
        case YdspIrOp::leI:
            cc->movdqa (target, srcA);
            cc->pcmpgtd (target, srcB);
            invert (target);
            break;
        case YdspIrOp::geI:
            cc->movdqa (target, srcB);
            cc->pcmpgtd (target, srcA);
            invert (target);
            break;
        default:
            break;
    }

    if (target != dst)
        moveVector (dst, target);
}

void YdspAsmJitCodegenX64::vectorSelectInt (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    if (hasAvx && activeVectorWidth <= 8)
    {
        cc->vpblendvb (dst, whenFalse, whenTrue, mask);
        return;
    }

    YdspFp selected = newIntVector ("sel");
    moveVector (selected, mask);
    if (hasAvx)
        cc->vpand (selected, selected, whenTrue);
    else
        cc->pand (selected, whenTrue);

    YdspFp inverse = newIntVector ("selInverse");
    moveVector (inverse, mask);
    if (hasAvx)
    {
        cc->vpandn (inverse, inverse, whenFalse);
        cc->vpor (selected, selected, inverse);
    }
    else
    {
        cc->pandn (inverse, whenFalse);
        cc->por (selected, inverse);
    }
    moveVector (dst, selected);
}

void YdspAsmJitCodegenX64::vectorIntToFloat (const YdspFp& dst, const YdspFp& src)
{
    if (hasAvx)
        cc->vcvtdq2ps (dst, src);
    else
        cc->cvtdq2ps (dst, src);
}

void YdspAsmJitCodegenX64::vectorFloatToInt (const YdspFp& dst, const YdspFp& src, bool bounded)
{
    YdspFp converted = newIntVector ("ftoi");

    if (hasAvx)
        cc->vcvttps2dq (converted, src);
    else
        cc->cvttps2dq (converted, src);

    if (! bounded)
    {
        const auto limit = vectorConstMem (0x4F000000u); // 2147483648.0f
        YdspFp overflow = newFpVector ("ftoiOverflow");
        YdspFp ordered = newFpVector ("ftoiOrdered");

        if (hasAvx)
        {
            cc->vmovups (overflow, limit);
            cc->vcmpps (overflow, overflow, src, asmjit::Imm (0x12)); // 2^31 <= src
            cc->vpxor (converted, converted, overflow);

            cc->vcmpps (ordered, src, src, asmjit::Imm (0x00)); // src == src (false for NaN)
            cc->vpand (converted, converted, ordered);
        }
        else
        {
            cc->movups (overflow, limit);
            cc->cmpps (overflow, src, asmjit::Imm (2)); // LE
            cc->pxor (converted, overflow);

            cc->movaps (ordered, src);
            cc->cmpps (ordered, src, asmjit::Imm (0)); // EQ
            cc->pand (converted, ordered);
        }
    }

    moveVector (dst, converted);
}

void YdspAsmJitCodegenX64::emitReduceAddFloat (const YdspFp& dst, const YdspFp& src)
{
    if (activeVectorWidth > 4)
    {
        YdspFp reduced = newFp128 ("reduce128");
        YdspFp part = newFp128 ("reducePart");

        if (activeVectorWidth == 16)
        {
            cc->vextractf32x4 (reduced, src, asmjit::Imm (0));

            for (int lane = 1; lane < 4; ++lane)
            {
                cc->vextractf32x4 (part, src, asmjit::Imm (lane));
                cc->vaddps (reduced, reduced, part);
            }
        }
        else
        {
            cc->vmovaps (reduced, src.xmm());
            cc->vextractf128 (part, src, asmjit::Imm (1));
            cc->vaddps (reduced, reduced, part);
        }

        YdspFp swapped = newFp128 ("reduceSwap");
        YdspFp halves = newFp128 ("reduceHalves");

        cc->vshufps (swapped, reduced, reduced, asmjit::Imm (0xB1)); // [b a d c]
        cc->vaddps (swapped, swapped, reduced);
        cc->vshufps (halves, swapped, swapped, asmjit::Imm (0x4E)); // swap the 64-bit halves
        cc->vaddss (swapped, swapped, halves);
        cc->vmovss (dst, swapped, swapped);
        return;
    }

    if (hasAvx)
    {
        YdspFp swapped = newFpVector ("reduceSwap");
        YdspFp halves = newFpVector ("reduceHalves");

        cc->vshufps (swapped, src, src, asmjit::Imm (0xB1));
        cc->vaddps (swapped, swapped, src);
        cc->vshufps (halves, swapped, swapped, asmjit::Imm (0x4E));
        cc->vaddss (swapped, swapped, halves);
        cc->vmovss (dst, swapped, swapped);
        return;
    }

    YdspFp swapped = newFpVector ("reduceSwap");
    YdspFp halves = newFpVector ("reduceHalves");

    moveVector (swapped, src);
    cc->shufps (swapped, swapped, asmjit::Imm (0xB1)); // [b a d c]
    cc->addps (swapped, src);

    moveVector (halves, swapped);
    cc->shufps (halves, halves, asmjit::Imm (0x4E)); // swap the 64-bit halves
    cc->addss (swapped, halves);

    cc->movss (dst, swapped);
}

void YdspAsmJitCodegenX64::emitInsertFloat (const YdspFp& dst, const YdspFp& srcVec, const YdspFp& srcScalar, int lane)
{
    jassert (lane >= 0 && lane < activeVectorWidth);
    const bool hasPrev = srcVec.is_valid();

    if (activeVectorWidth > 4)
    {
        const int chunk = lane >> 2;
        const int subLane = lane & 3;
        const auto zmask = static_cast<uint32_t> (0xF ^ (1 << subLane));

        if (! hasPrev)
        {
            if (chunk == 0)
            {
                cc->vinsertps (dst.xmm(), srcScalar, srcScalar, asmjit::Imm ((static_cast<uint32_t> (subLane) << 4) | zmask));
                return;
            }

            YdspFp part = newFp128 ("insPart");
            cc->vinsertps (part, srcScalar, srcScalar, asmjit::Imm ((static_cast<uint32_t> (subLane) << 4) | zmask));
            cc->vxorps (dst, dst, dst);

            if (activeVectorWidth == 16)
                cc->vinsertf32x4 (dst, dst, part, asmjit::Imm (chunk));
            else
                cc->vinsertf128 (dst, dst, part, asmjit::Imm (chunk));

            return;
        }

        YdspFp part = newFp128 ("insPart");

        if (chunk == 0)
        {
            cc->vinsertps (part, srcVec.xmm(), srcScalar, asmjit::Imm (subLane << 4));
        }
        else
        {
            if (activeVectorWidth == 16)
                cc->vextractf32x4 (part, srcVec, asmjit::Imm (chunk));
            else
                cc->vextractf128 (part, srcVec, asmjit::Imm (chunk));

            cc->vinsertps (part, part, srcScalar, asmjit::Imm (subLane << 4));
        }

        if (activeVectorWidth == 16)
            cc->vinsertf32x4 (dst, srcVec, part, asmjit::Imm (chunk));
        else
            cc->vinsertf128 (dst, srcVec, part, asmjit::Imm (chunk));

        return;
    }

    if (hasAvx)
    {
        if (! hasPrev)
        {
            const auto zmask = static_cast<uint32_t> (0xF ^ (1 << lane));
            cc->vinsertps (dst, srcScalar, srcScalar, asmjit::Imm ((static_cast<uint32_t> (lane) << 4) | zmask));
            return;
        }

        cc->vinsertps (dst, srcVec, srcScalar, asmjit::Imm (lane << 4));
        return;
    }

    if (hasSse4_1)
    {
        if (! hasPrev)
            cc->xorps (dst, dst);
        else if (dst != srcVec)
            moveVector (dst, srcVec);

        cc->insertps (dst, srcScalar, asmjit::Imm (lane << 4));
        return;
    }

    if (! hasPrev)
    {
        cc->xorps (dst, dst);
        cc->movss (dst, srcScalar);

        if (lane == 1)
            cc->shufps (dst, dst, asmjit::Imm (0x51));
        else if (lane == 2)
            cc->shufps (dst, dst, asmjit::Imm (0x45));
        else if (lane == 3)
            cc->shufps (dst, dst, asmjit::Imm (0x15));

        return;
    }

    if (lane == 0)
    {
        if (dst != srcVec)
            moveVector (dst, srcVec);

        cc->movss (dst, srcScalar);
        return;
    }

    YdspFp tmp = newFp128 ("insTmp");

    if (lane == 1)
    {
        cc->movaps (tmp, srcVec);
        cc->shufps (tmp, srcScalar, asmjit::Imm (0x00));
        cc->shufps (tmp, srcVec, asmjit::Imm (0xE8));
        moveVector (dst, tmp);
        return;
    }

    if (lane == 2)
    {
        cc->movaps (tmp, srcScalar);
        cc->shufps (tmp, srcVec, asmjit::Imm (0xF0));

        if (dst != srcVec)
            moveVector (dst, srcVec);

        cc->shufps (dst, tmp, asmjit::Imm (0x84));
        return;
    }

    cc->movaps (tmp, srcVec);
    cc->shufps (tmp, srcScalar, asmjit::Imm (0x0A));

    if (dst != srcVec)
        moveVector (dst, srcVec);

    cc->shufps (dst, tmp, asmjit::Imm (0x84));
}

void YdspAsmJitCodegenX64::emitExtractFloat (const YdspFp& dst, const YdspFp& srcVec, int lane)
{
    jassert (lane >= 0 && lane < activeVectorWidth);

    if (activeVectorWidth > 4)
    {
        const int chunk = lane >> 2;
        const int subLane = lane & 3;

        if (chunk == 0)
        {
            if (subLane == 0)
                cc->vmovaps (dst, srcVec.xmm());
            else
                cc->vpshufd (dst, srcVec.xmm(), asmjit::Imm (subLane));

            return;
        }

        YdspFp part = newFp128 ("extPart");

        if (activeVectorWidth == 16)
            cc->vextractf32x4 (part, srcVec, asmjit::Imm (chunk));
        else
            cc->vextractf128 (part, srcVec, asmjit::Imm (chunk));

        if (subLane == 0)
            cc->vmovaps (dst, part);
        else
            cc->vpshufd (dst, part, asmjit::Imm (subLane));

        return;
    }

    if (lane == 0)
    {
        if (dst != srcVec)
        {
            if (hasAvx)
                cc->vmovaps (dst, srcVec);
            else
                cc->movaps (dst, srcVec);
        }

        return;
    }

    if (hasAvx)
        cc->vpshufd (dst, srcVec, asmjit::Imm (lane));
    else
        cc->pshufd (dst, srcVec, asmjit::Imm (lane));
}

void YdspAsmJitCodegenX64::emitBitcastFloatToInt (const YdspGp& dst, const YdspFp& src)
{
    if (hasAvx)
        cc->vmovd (dst, src);
    else
        cc->movd (dst, src);
}

void YdspAsmJitCodegenX64::emitBitcastIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    if (hasAvx)
        cc->vmovd (dst, src);
    else
        cc->movd (dst, src);
}

void YdspAsmJitCodegenX64::emitAddPointerImm (const YdspGp& dst, const YdspGp& src, int32_t imm)
{
    if (imm == 0)
    {
        if (dst != src)
            moveGp (dst, src);

        return;
    }

    cc->lea (dst, asmjit::x86::ptr (src, imm));
}

void YdspAsmJitCodegenX64::roundFloat (const YdspFp& reg, int mode)
{
    const bool is64 = isDoubleFloat (reg);

    if (is64)
        cc->roundsd (reg, reg, asmjit::Imm (mode | 8));
    else
        cc->roundss (reg, reg, asmjit::Imm (mode | 8));
}

void YdspAsmJitCodegenX64::emitSaturatingIntBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB)
{
    jassert (op == YdspIrOp::addI || op == YdspIrOp::subI || op == YdspIrOp::mulI);
    const auto limit = integerSaturationLimit (srcA, srcB, op == YdspIrOp::mulI);
    const auto result = dst.is_gp64() ? cc->new_gp64 ("arithmetic") : cc->new_gp32 ("arithmetic");
    moveGp (result, srcA);
    if (op == YdspIrOp::addI)
        cc->add (result, srcB);
    else if (op == YdspIrOp::subI)
        cc->sub (result, srcB);
    else
        cc->imul (result, srcB);
    cc->cmovo (result, limit);
    moveGp (dst, result);
}

void YdspAsmJitCodegenX64::intBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB)
{
    // As in floatBinary: `n = a - n` must not load `a` over the right operand.
    if (dst == srcB && dst != srcA)
    {
        if (op != YdspIrOp::subI && op != YdspIrOp::shlI && op != YdspIrOp::shrI)
        {
            intBinary (op, dst, srcB, srcA);
            return;
        }

        YdspGp result = dst.is_gp64() ? cc->new_gp64 ("binaryResult") : cc->new_gp32 ("binaryResult");
        intBinary (op, result, srcA, srcB);
        moveGp (dst, result);
        return;
    }

    if (dst != srcA)
        moveGp (dst, srcA);

    switch (op)
    {
        case YdspIrOp::addI:
            cc->add (dst, srcB);
            break;
        case YdspIrOp::subI:
            cc->sub (dst, srcB);
            break;
        case YdspIrOp::mulI:
            cc->imul (dst, srcB);
            break;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            cc->and_ (dst, srcB);
            break;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            cc->or_ (dst, srcB);
            break;
        case YdspIrOp::xorI:
            cc->xor_ (dst, srcB);
            break;
        case YdspIrOp::shlI:
            cc->shl (dst, srcB);
            break;
        case YdspIrOp::shrI:
            cc->sar (dst, srcB);
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenX64::intUnaryNeg (const YdspGp& dst, const YdspGp& src)
{
    cc->mov (dst, src);
    cc->neg (dst);
}

void YdspAsmJitCodegenX64::emitIntDivision (YdspIrOp op, const YdspGp& dst, const YdspGp& a, const YdspGp& b, bool is64)
{
    asmjit::Label zeroLabel = cc->new_label();
    asmjit::Label overflowLabel = cc->new_label();
    asmjit::Label divideLabel = cc->new_label();
    asmjit::Label doneLabel = cc->new_label();

    branchIfZero (b, zeroLabel);

    cc->cmp (b, asmjit::Imm (-1));
    cc->jne (divideLabel);

    if (is64)
    {
        YdspGp limit = cc->new_gp64 ("intMin");
        cc->mov (limit, asmjit::Imm (std::numeric_limits<int64_t>::min()));
        cc->cmp (a, limit);
    }
    else
    {
        cc->cmp (a, asmjit::Imm (std::numeric_limits<int32_t>::min()));
    }

    cc->je (overflowLabel);

    cc->bind (divideLabel);

    YdspGp quotient = is64 ? cc->new_gp64 ("quot") : cc->new_gp32 ("quot");
    YdspGp remainder = is64 ? cc->new_gp64 ("rem") : cc->new_gp32 ("rem");

    cc->mov (quotient, a);

    if (is64)
        cc->cqo (remainder, quotient);
    else
        cc->cdq (remainder, quotient);

    cc->idiv (remainder, quotient, b);
    cc->mov (dst, op == YdspIrOp::divI ? quotient : remainder);

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

void YdspAsmJitCodegenX64::emitNotB (const YdspGp& dst, const YdspGp& src)
{
    YdspGp one = cc->new_gp32 ("one");
    cc->mov (one, asmjit::Imm (1));

    cc->mov (dst, src);
    cc->xor_ (dst, one);
}

//==============================================================================

void YdspAsmJitCodegenX64::emitFloatCompare (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst)
{
    if (isDoubleFloat (a))
        cc->comisd (a, b);
    else
        cc->comiss (a, b);

    YdspGp tmp = cc->new_gp32 ("cmp");
    cc->set (cond, tmp.r8());
    cc->movzx (dst, tmp.r8());
}

void YdspAsmJitCodegenX64::emitIntCompare (YdspCond cond, const YdspGp& a, const YdspGp& b, const YdspGp& dst)
{
    cc->cmp (a, b);
    YdspGp tmp = cc->new_gp32 ("cmp");
    cc->set (cond, tmp.r8());
    cc->movzx (dst, tmp.r8());
}

void YdspAsmJitCodegenX64::emitFloatCompareToReg (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst)
{
    const bool is64 = isDoubleFloat (b);

    if (is64)
        cc->comisd (a, b);
    else
        cc->comiss (a, b);

    cc->set (cond, dst.r8());
    cc->movzx (dst, dst.r8());
}

//==============================================================================

void YdspAsmJitCodegenX64::emitSelectFloat (const YdspGp& cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    const bool is64 = isDoubleFloat (dst);
    YdspGp selected = is64 ? cc->new_gp64 ("sel") : cc->new_gp32 ("sel");
    YdspGp whenTrueBits = is64 ? cc->new_gp64 ("selTrue") : cc->new_gp32 ("selTrue");
    YdspGp whenFalseBits = is64 ? cc->new_gp64 ("selFalse") : cc->new_gp32 ("selFalse");

    if (is64)
    {
        cc->movq (whenTrueBits, whenTrue);
        cc->movq (whenFalseBits, whenFalse);
    }
    else
    {
        cc->movd (whenTrueBits, whenTrue);
        cc->movd (whenFalseBits, whenFalse);
    }

    cc->mov (selected, whenFalseBits);
    cc->test (cond, cond);
    cc->cmov (YdspCond::kNotEqual, selected, whenTrueBits);

    if (is64)
        cc->movq (dst, selected);
    else
        cc->movd (dst, selected);
}

void YdspAsmJitCodegenX64::emitFloatCompareToFlags (const YdspFp& a, const YdspFp& b)
{
    if (isDoubleFloat (a))
        cc->comisd (a, b);
    else
        cc->comiss (a, b);
}

void YdspAsmJitCodegenX64::emitIntCompareToFlags (const YdspGp& a, const YdspGp& b)
{
    cc->cmp (a, b);
}

void YdspAsmJitCodegenX64::emitSelectFloatOnFlags (YdspCond cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    const bool is64 = isDoubleFloat (dst);
    YdspGp selected = is64 ? cc->new_gp64 ("sel") : cc->new_gp32 ("sel");
    YdspGp whenTrueBits = is64 ? cc->new_gp64 ("selTrue") : cc->new_gp32 ("selTrue");
    YdspGp whenFalseBits = is64 ? cc->new_gp64 ("selFalse") : cc->new_gp32 ("selFalse");

    if (is64)
    {
        cc->movq (whenTrueBits, whenTrue);
        cc->movq (whenFalseBits, whenFalse);
    }
    else
    {
        cc->movd (whenTrueBits, whenTrue);
        cc->movd (whenFalseBits, whenFalse);
    }

    cc->mov (selected, whenFalseBits);
    cc->cmov (cond, selected, whenTrueBits);

    if (is64)
        cc->movq (dst, selected);
    else
        cc->movd (dst, selected);
}

void YdspAsmJitCodegenX64::emitSelectIntOnFlags (YdspCond cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse)
{
    if (dst == whenTrue)
    {
        if (dst != whenFalse)
            cc->cmov (static_cast<YdspCond> (static_cast<uint8_t> (cond) ^ uint8_t (1)), dst, whenFalse);

        return;
    }

    if (dst != whenFalse)
        moveGp (dst, whenFalse);

    cc->cmov (cond, dst, whenTrue);
}

void YdspAsmJitCodegenX64::emitSelectInt (const YdspGp& cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse)
{
    if (dst == whenTrue)
    {
        if (dst != whenFalse)
        {
            cc->test (cond, cond);
            cc->cmov (YdspCond::kZero, dst, whenFalse);
        }

        return;
    }

    if (dst != whenFalse)
        moveGp (dst, whenFalse);

    cc->test (cond, cond);
    cc->cmov (YdspCond::kNotZero, dst, whenTrue);
}

void YdspAsmJitCodegenX64::emitWrapInt (const YdspGp& dst, const YdspGp& value, const YdspGp& bound)
{
    YdspGp zero = dst.is_gp64() ? cc->new_gp64 ("zero") : cc->new_gp32 ("zero");
    cc->xor_ (zero, zero);

    moveGp (dst, value);
    cc->cmp (dst, bound);
    cc->cmov (YdspCond::kSignedGE, dst, zero);
}

void YdspAsmJitCodegenX64::emitAdvanceWrapInt (const YdspGp& dst, const YdspGp& value, int32_t bound)
{
    YdspGp zero = dst.is_gp64() ? cc->new_gp64 ("zero") : cc->new_gp32 ("zero");
    cc->xor_ (zero, zero);

    if (dst == value)
        cc->add (dst, asmjit::Imm (1));
    else
        cc->lea (dst, asmjit::x86::ptr (value.r64(), 1));

    cc->cmp (dst, asmjit::Imm (bound));
    cc->cmov (YdspCond::kSignedGE, dst, zero);
}

//==============================================================================

void YdspAsmJitCodegenX64::emitIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    if (isDoubleFloat (dst))
        cc->cvtsi2sd (dst, src);
    else
        cc->cvtsi2ss (dst, src);
}

void YdspAsmJitCodegenX64::emitFloatToInt (const YdspGp& dst, const YdspFp& src, bool bounded)
{
    if (isDoubleFloat (src))
        cc->cvttsd2si (dst, src);
    else
        cc->cvttss2si (dst, src);

    if (bounded)
        return;

    const bool wide = dst.is_gp64();
    const bool doublePrecision = isDoubleFloat (src);
    const auto upper = doublePrecision ? newFp64 ("conversionUpper") : newFp ("conversionUpper");
    loadFloatConst (upper, wide ? 9223372036854775808.0 : 2147483648.0,
                    doublePrecision ? YdspValueType::float64Type : YdspValueType::float32Type);
    const auto limit = wide ? cc->new_gp64 ("conversionLimit") : cc->new_gp32 ("conversionLimit");
    cc->mov (limit, asmjit::Imm (wide ? std::numeric_limits<int64_t>::max() : std::numeric_limits<int32_t>::max()));
    emitFloatCompareToFlags (src, upper);
    cc->cmovae (dst, limit);
    cc->mov (limit, asmjit::Imm (0));
    emitFloatCompareToFlags (src, src);
    cc->cmovp (dst, limit);
}

void YdspAsmJitCodegenX64::emitExtendInt (const YdspGp& dst, const YdspGp& src)
{
    cc->movsxd (dst, src);
}

void YdspAsmJitCodegenX64::emitTruncateInt (const YdspGp& dst, const YdspGp& src)
{
    cc->mov (dst, src.r32());
}

void YdspAsmJitCodegenX64::emitExtendFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->cvtss2sd (dst, src);
}

void YdspAsmJitCodegenX64::emitTruncateFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->cvtsd2ss (dst, src);
}

//==============================================================================

void YdspAsmJitCodegenX64::jump (const asmjit::Label& target)
{
    cc->jmp (target);
}

void YdspAsmJitCodegenX64::branchIfZero (const YdspGp& cond, const asmjit::Label& target)
{
    cc->test (cond, cond);
    cc->jz (target);
}

void YdspAsmJitCodegenX64::branchIfNotZero (const YdspGp& cond, const asmjit::Label& target)
{
    cc->test (cond, cond);
    cc->jnz (target);
}

void YdspAsmJitCodegenX64::branchOnFlags (YdspCond cond, bool branchWhenTrue, const asmjit::Label& target)
{
    if (! branchWhenTrue)
        cond = static_cast<YdspCond> (static_cast<uint8_t> (cond) ^ uint8_t (1));

    cc->j (cond, target);
}

} // namespace yup
