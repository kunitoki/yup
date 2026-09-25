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

bool YdspAsmJitCodegenX64::isDoubleFloat (const YdspFp& reg) const
{
    if (! cc->is_virt_reg_valid (reg))
        return false;

    const auto type = cc->virt_reg_by_reg (reg)->type_id();
    return type == asmjit::TypeId::kFloat64 || type == asmjit::TypeId::kFloat64x1;
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
    if (activeVectorWidth > 4)
        cc->vmovups (dst, src);
    else
        cc->movups (dst, src);
}

void YdspAsmJitCodegenX64::storeVectorToMem (const YdspMem& dst, const YdspFp& src)
{
    if (activeVectorWidth > 4)
        cc->vmovups (dst, src);
    else
        cc->movups (dst, src);
}

void YdspAsmJitCodegenX64::moveVector (const YdspFp& dst, const YdspFp& src)
{
    if (activeVectorWidth > 4)
        cc->vmovaps (dst, src);
    else
        cc->movaps (dst, src);
}

void YdspAsmJitCodegenX64::vectorBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    if (activeVectorWidth > 4)
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
    if (activeVectorWidth > 4)
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
            moveVector (dst, src);
            cc->xorps (dst, packedConstMem (0x80000000ull, false));
            break;

        case YdspIrOp::absF:
            moveVector (dst, src);
            cc->andps (dst, packedConstMem (0x7FFFFFFFull, false));
            break;

        default:
            break;
    }
}

void YdspAsmJitCodegenX64::vectorFloatCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    if (activeVectorWidth > 4)
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

    moveVector (dst, swapOperands ? srcB : srcA);
    cc->cmpps (dst, swapOperands ? srcA : srcB, asmjit::Imm (predicate));
}

void YdspAsmJitCodegenX64::vectorSelectFloat (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    YdspFp selected = newFpVector ("sel");
    moveVector (selected, mask);
    if (activeVectorWidth > 4)
        cc->vandps (selected, selected, whenTrue);
    else
        cc->andps (selected, whenTrue);

    YdspFp inverse = newFpVector ("selInverse");
    moveVector (inverse, mask);
    if (activeVectorWidth > 4)
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

    moveVector (dst, src);
    cc->shufps (dst, dst, asmjit::Imm (0x00)); // every lane = lane 0
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
        cc->vmovss (dst, dst, swapped);
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
    switch (op)
    {
        case YdspIrOp::addI:
            moveGp (dst, srcA);
            cc->add (dst, srcB);
            break;
        case YdspIrOp::subI:
            moveGp (dst, srcA);
            cc->sub (dst, srcB);
            break;
        case YdspIrOp::mulI:
            moveGp (dst, srcA);
            cc->imul (dst, srcB);
            break;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            moveGp (dst, srcA);
            cc->and_ (dst, srcB);
            break;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            moveGp (dst, srcA);
            cc->or_ (dst, srcB);
            break;
        case YdspIrOp::xorI:
            moveGp (dst, srcA);
            cc->xor_ (dst, srcB);
            break;
        case YdspIrOp::shlI:
            moveGp (dst, srcA);
            cc->shl (dst, srcB);
            break;
        case YdspIrOp::shrI:
            moveGp (dst, srcA);
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
    moveGp (dst, whenFalse);
    cc->cmov (cond, dst, whenTrue);
}

void YdspAsmJitCodegenX64::emitSelectInt (const YdspGp& cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse)
{
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
    moveGp (dst, value);
    cc->add (dst, asmjit::Imm (1));
    cc->cmp (dst, asmjit::Imm (bound));

    const auto done = cc->new_label();

    cc->jl (done);
    cc->xor_ (dst, dst);
    cc->bind (done);
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
