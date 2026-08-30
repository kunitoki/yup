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

#if ASMJIT_ARCH_X86

namespace yup
{

namespace
{

// Small helpers used by the generated code for integer division/modulo on x86
// (AArch64 lowers these natively with sdiv/msub). Division by zero yields 0
// for both the quotient and the remainder, matching the explicit zero-guard
// used in the AArch64 lowering.
int32_t yupDspIdiv (int32_t a, int32_t b)
{
    return b != 0 && ! (a == std::numeric_limits<int32_t>::min() && b == -1) ? a / b : 0;
}

int32_t yupDspImod (int32_t a, int32_t b)
{
    return b != 0 && ! (a == std::numeric_limits<int32_t>::min() && b == -1) ? a % b : 0;
}

// 64-bit integer div/mod helpers for the x86 backend (AArch64 lowers natively).
int64_t yupDspIdiv64 (int64_t a, int64_t b)
{
    return b != 0 && ! (a == std::numeric_limits<int64_t>::min() && b == -1) ? a / b : 0;
}

int64_t yupDspImod64 (int64_t a, int64_t b)
{
    return b != 0 && ! (a == std::numeric_limits<int64_t>::min() && b == -1) ? a % b : 0;
}

} // namespace

//==============================================================================

bool YdspAsmJitCodegenX64::isDoubleFloat (const YdspFp& reg) const
{
    // Every float register this lowering sees is a virtual one; a physical
    // register has no VirtReg and no TypeId, so treat it as the f32 default
    // rather than reading out of bounds.
    if (! cc->is_virt_reg_valid (reg))
        return false;

    const auto type = cc->virt_reg_by_reg (reg)->type_id();
    return type == asmjit::TypeId::kFloat64 || type == asmjit::TypeId::kFloat64x1;
}

//==============================================================================
// Register allocation

YdspFp YdspAsmJitCodegenX64::newFp (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x1, name);
}

YdspFp YdspAsmJitCodegenX64::newFp64 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat64x1, name);
}

YdspFp YdspAsmJitCodegenX64::newFp128 (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x4, name);
}

YdspFp YdspAsmJitCodegenX64::newFpVector (const char* name)
{
    if (activeVectorWidth == 16)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x16, name);

    if (activeVectorWidth == 8)
        return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x8, name);

    return cc->new_reg<YdspFp> (asmjit::TypeId::kFloat32x4, name);
}

//==============================================================================
// Memory addressing

YdspMem YdspAsmJitCodegenX64::memPtr (const YdspGp& base, int32_t offset) const
{
    return asmjit::x86::ptr (base, offset);
}

YdspMem YdspAsmJitCodegenX64::memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const
{
    return asmjit::x86::ptr (base, index, scaleLog2, offset);
}

YdspMem YdspAsmJitCodegenX64::emitStateMem (YdspValueType type, int base, int indexValue)
{
    const uint32_t scale = is64BitValueType (type) ? 3u : 2u;

    if (indexValue < 0)
        return asmjit::x86::ptr (stateReg, base);

    return asmjit::x86::ptr (stateArraysReg, gp (indexValue), scale, base);
}

YdspMem YdspAsmJitCodegenX64::emitVectorStateMem (int base, int indexValue)
{
    // A packed access reaches the same element, so the addressing mode is the
    // one a scalar float32 access uses - only the operand width differs.
    return asmjit::x86::ptr (stateArraysReg, gp (indexValue), 2u, base);
}

YdspMem YdspAsmJitCodegenX64::emitVectorStreamMem (const YdspGp& base, int indexValue)
{
    return asmjit::x86::ptr (base, gp (indexValue), 2u, 0);
}

//==============================================================================
// Loads, stores and moves

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

void YdspAsmJitCodegenX64::moveGpToFp (const YdspFp& dst, const YdspGp& src)
{
    if (isDoubleFloat (dst))
        cc->movq (dst, src);
    else
        cc->movd (dst, src);
}

//==============================================================================
// Arithmetic

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
    // `vfmadd213ss x, y, z` is x = y * x + z, so the accumulator has to be in
    // `dst` before the multiply operands are named.
    //
    // This is VEX-encoded and the rest of this backend is legacy SSE. Mixing
    // the two carries a documented transition penalty on several Intel
    // microarchitectures, which is a reason to measure this target
    // specifically rather than to assume the AArch64 result transfers - the
    // compiler only reaches this when the host reported FMA3 (see
    // YdspOptimizer::setTargetHasFusedMultiplyAdd), so the fallback is one
    // switch away if it turns out to be a loss.
    moveFloat (dst, a);
    if (isDoubleFloat (dst))
        cc->vfmadd213sd (dst, b, c);
    else
        cc->vfmadd213ss (dst, b, c);
}

void YdspAsmJitCodegenX64::emitFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    moveFloat (dst, c);
    if (isDoubleFloat (dst))
        cc->vfnmadd231sd (dst, a, b);
    else
        cc->vfnmadd231ss (dst, a, b);
}

void YdspAsmJitCodegenX64::emitVectorFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    // `vfmadd213ps x, y, z` is x = y * x + z, so seed dst from a first.
    // This is only emitted for an AVX2+FMA target selected by the compiler.
    moveVector (dst, a);
    cc->vfmadd213ps (dst, b, c);
}

void YdspAsmJitCodegenX64::emitVectorFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
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
            // Flip the sign bit rather than computing 0 - x: one instruction
            // instead of three, and it negates zero correctly (0 - 0 is +0).
            moveFloat (dst, src);

            if (is64)
                cc->xorpd (dst, packedConstMem (0x8000000000000000ull, true));
            else
                cc->xorps (dst, packedConstMem (0x80000000ull, false));

            break;
        }
        case YdspIrOp::absF:
        {
            // Mask off the sign bit straight from the constant pool: no GP
            // register and no domain-crossing move.
            moveFloat (dst, src);

            if (is64)
                cc->andpd (dst, packedConstMem (0x7FFFFFFFFFFFFFFFull, true));
            else
                cc->andps (dst, packedConstMem (0x7FFFFFFFull, false));

            break;
        }
        default:
            break;
    }
}

//==============================================================================
// Packed float32 (SSE2 / AVX2)
//
// Loads and stores are unaligned unconditionally: a state array's region base
// is only 4-byte aligned, and correctness must never depend on where the
// allocator happened to put it. On any CPU this backend targets, movups on a
// naturally-aligned address costs the same as movaps.

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

        case YdspIrOp::negF:
            // The sign masks already come from a 16-byte pool entry with the
            // pattern replicated across the whole lane, which is what the packed
            // bitwise ops need - so these are the scalar forms unchanged.
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
    uint32_t predicate = 0;

    switch (op)
    {
        case YdspIrOp::eqF: predicate = 0x00; break;
        case YdspIrOp::neF: predicate = 0x0c; break;
        case YdspIrOp::ltF: predicate = 0x11; break;
        case YdspIrOp::leF: predicate = 0x12; break;
        case YdspIrOp::gtF: predicate = 0x1e; break;
        case YdspIrOp::geF: predicate = 0x1d; break;
        default: return;
    }

    if (activeVectorWidth > 4)
        cc->vcmpps (dst, srcA, srcB, asmjit::Imm (predicate));
    else
        cc->cmpps (dst, srcA, asmjit::Imm (predicate));
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
        // AVX2 has no horizontal eight-lane add. First combine its two 128-bit
        // halves, then use a 128-bit tree. AVX-512 follows the same tree over
        // four extracted 128-bit quarters. The tree is intentionally explicit:
        // it keeps the reassociation local to the vectoriser's documented
        // reduction contract rather than relying on a target-dependent hadd.
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

    // SSE2 has no horizontal add (haddps is SSE3), so fold the four lanes with
    // two shuffles: [a b c d] -> [a+b a+b c+d c+d] -> (a+b) + (c+d) in lane 0.
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
    if (is64)
    {
        YdspGp target = cc->new_gp64 ("fn");
        cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (op == YdspIrOp::divI ? reinterpret_cast<void*> (&yupDspIdiv64) : reinterpret_cast<void*> (&yupDspImod64))));

        asmjit::InvokeNode* node = nullptr;
        auto err = cc->invoke (asmjit::Out (node), target, asmjit::FuncSignature::build<int64_t, int64_t, int64_t>());

        if (err == asmjit::kErrorOk && node != nullptr)
        {
            node->set_arg (0, a);
            node->set_arg (1, b);
            node->set_ret (0, dst);
        }
    }
    else
    {
        YdspGp target = cc->new_gp64 ("fn");
        cc->mov (target, asmjit::Imm (ydspFnPtrToInt64 (op == YdspIrOp::divI
                                                            ? reinterpret_cast<void*> (&yupDspIdiv)
                                                            : reinterpret_cast<void*> (&yupDspImod))));

        auto err = cc->invoke (asmjit::Out (divInvoke),
                               target,
                               asmjit::FuncSignature::build<int32_t, int32_t, int32_t>());

        if (err == asmjit::kErrorOk && divInvoke != nullptr)
        {
            divInvoke->set_arg (0, a);
            divInvoke->set_arg (1, b);
            divInvoke->set_ret (0, dst);
        }
    }
}

void YdspAsmJitCodegenX64::emitNotB (const YdspGp& dst, const YdspGp& src)
{
    YdspGp one = cc->new_gp32 ("one");
    cc->mov (one, asmjit::Imm (1));

    cc->mov (dst, src);
    cc->xor_ (dst, one);
}

//==============================================================================
// Comparisons

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
// Branchless select and ring wrap
//
// The two integer forms below write `dst` before reading their operands, so
// they would corrupt the result if `dst` ever aliased one of them. It cannot
// today: newValue() always allocates a fresh result id and no pass rewrites a
// result to alias an operand. Anything that coalesces value ids (CSE, a
// vectoriser) has to give these a temporary first - the same constraint
// lerpF/clampF carry in the shared lowering.

// SSE has no float conditional move (and the non-VEX blendvps hardwires its
// mask to xmm0, which fights the register allocator), so blend with
// and/andn/or against an all-ones / all-zeros mask.
void YdspAsmJitCodegenX64::blendFloatOnMask (const YdspGp& maskBits, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    const bool is64 = isDoubleFloat (dst);
    const auto floatType = is64 ? YdspValueType::float64Type : YdspValueType::float32Type;

    YdspFp mask = newFpOfType (floatType, "selMaskV");

    if (is64)
    {
        // movq needs a 64-bit source, and the sign extension turns the 32-bit
        // all-ones pattern into a 64-bit one.
        YdspGp wide = cc->new_gp64 ("selMask64");
        cc->movsxd (wide, maskBits);
        moveGpToFp (mask, wide);
    }
    else
    {
        moveGpToFp (mask, maskBits);
    }

    YdspFp blended = newFpOfType (floatType, "sel");
    moveFloat (blended, mask);

    if (is64)
    {
        cc->andpd (blended, whenTrue); // mask & whenTrue
        cc->andnpd (mask, whenFalse);  // ~mask & whenFalse
        cc->orpd (blended, mask);
    }
    else
    {
        cc->andps (blended, whenTrue);
        cc->andnps (mask, whenFalse);
        cc->orps (blended, mask);
    }

    moveFloat (dst, blended);
}

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
    // `mov` leaves the flags alone, so the cmov still sees the comparison.
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
// Conversions

void YdspAsmJitCodegenX64::emitIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    if (isDoubleFloat (dst))
        cc->cvtsi2sd (dst, src);
    else
        cc->cvtsi2ss (dst, src);
}

void YdspAsmJitCodegenX64::emitFloatToInt (const YdspGp& dst, const YdspFp& src)
{
    if (isDoubleFloat (src))
        cc->cvttsd2si (dst, src);
    else
        cc->cvttss2si (dst, src);
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
// Control flow

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

} // namespace yup

#endif // ASMJIT_ARCH_X86
