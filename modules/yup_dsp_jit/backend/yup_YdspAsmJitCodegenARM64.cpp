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

#if ASMJIT_ARCH_ARM

namespace yup
{

namespace
{

// Adds an arbitrary 32-bit immediate to a pointer register (used for array
// base offsets on AArch64, whose add-immediate is limited to 12 bits).
void addGpImm (YdspAsm& cc, const YdspGp& dst, const YdspGp& src, int32_t imm)
{
    if (imm == 0)
    {
        cc.mov (dst, src);
        return;
    }

    YdspGp tmp = cc.new_gp64 ("imm");
    cc.mov (tmp, asmjit::Imm (imm));
    cc.add (dst, src, tmp);
}

// The shared lowering names its conditions after the x86 condition codes; on
// AArch64 the float comparisons must select the *ordered* codes after FCMP
// (HI / HS would also fire on an unordered result), so this is not an enum
// passthrough. Integer comparisons map straight through.
asmjit::arm::CondCode toArmCondition (YdspCond cond) noexcept
{
    switch (cond)
    {
        case YdspCond::kEqual:
            return asmjit::arm::CondCode::kEQ;
        case YdspCond::kNotEqual:
            return asmjit::arm::CondCode::kNE;
        case YdspCond::kUnsignedLT:
            return asmjit::arm::CondCode::kLO;
        case YdspCond::kUnsignedLE:
            return asmjit::arm::CondCode::kLS;
        case YdspCond::kUnsignedGT:
            return asmjit::arm::CondCode::kGT;
        case YdspCond::kUnsignedGE:
            return asmjit::arm::CondCode::kGE;
        case YdspCond::kSignedLT:
            return asmjit::arm::CondCode::kLT;
        case YdspCond::kSignedLE:
            return asmjit::arm::CondCode::kLE;
        case YdspCond::kSignedGT:
            return asmjit::arm::CondCode::kGT;
        case YdspCond::kSignedGE:
            return asmjit::arm::CondCode::kGE;
        default:
            return asmjit::arm::CondCode::kEQ;
    }
}

asmjit::Imm conditionImm (YdspCond cond) noexcept
{
    return asmjit::Imm (static_cast<uint32_t> (toArmCondition (cond)));
}

} // namespace

//==============================================================================
// Memory addressing

YdspMem YdspAsmJitCodegenARM64::memPtr (const YdspGp& base, int32_t offset) const
{
    return asmjit::a64::ptr (base, offset);
}

YdspMem YdspAsmJitCodegenARM64::memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const
{
    // AArch64 has no base+index+offset form; callers materialize the offset.
    (void) offset;
    return asmjit::a64::ptr (base, index, asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, static_cast<int> (scaleLog2)));
}

void YdspAsmJitCodegenARM64::prepareStateAddressing (const YdspIrFunction& fn)
{
    // AArch64 has no base + index + offset addressing mode, so every array
    // access has to reach `stateArrays + regionBase` in a register first. That
    // base is a compile-time constant per region, so materialize one register
    // per distinct region here instead of re-deriving it (a mov plus an add,
    // and two fresh virtual registers) at every access - which, in a loop over
    // parallel state arrays, is most of the emitted instructions.
    stateArrayBaseRegs.clear();

    const auto reserve = [this] (int value, int region)
    {
        if (value < 0 || static_cast<size_t> (value) >= valueTypes.size())
            return;

        const auto base = stateArrayBase (valueTypes[static_cast<size_t> (value)], region);

        if (stateArrayBaseRegs.count (base) != 0)
            return;

        // The first region starts at the array pointer itself, so it needs no
        // register of its own.
        if (base == 0)
        {
            stateArrayBaseRegs.emplace (base, stateArraysReg);
            return;
        }

        YdspGp addr = cc->new_gp64 ("arrayBase");
        addGpImm (*cc, addr, stateArraysReg, base);
        stateArrayBaseRegs.emplace (base, addr);
    };

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
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

YdspMem YdspAsmJitCodegenARM64::emitStateMem (YdspValueType type, int base, int indexValue)
{
    const uint32_t scale = is64BitValueType (type) ? 3u : 2u;

    if (indexValue < 0)
    {
        // AArch64 LDR/STR immediate offsets are limited to 0..16380
        // (32-bit) / 0..32760 (64-bit); the ldur fallback covers only
        // +/-256. Scalars live in the layout's head so they are small
        // in practice, but materialize the effective address for
        // pathological scalar counts that exceed the range.
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

    // prepareStateAddressing() reserved a register for every region the IR
    // touches; the fallback only covers an address built outside that scan.
    if (const auto it = stateArrayBaseRegs.find (base); it != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (it->second, gp (indexValue), shift);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    return asmjit::a64::ptr (addr, gp (indexValue), shift);
}

YdspMem YdspAsmJitCodegenARM64::emitVectorStateMem (int base, int indexValue)
{
    // LDR/STR of a Q register with a register offset only encodes a shift of 0
    // or 4, so `[base, index, lsl #2]` - the form every scalar float access
    // uses - is not available here. Keep the index pre-scaled to bytes in a
    // register instead and address with no shift at all.
    auto scaled = vectorIndexRegs.find (indexValue);

    if (scaled == vectorIndexRegs.end())
    {
        YdspGp offset = cc->new_gp64 ("vecIndex");
        cc->sxtw (offset, gp (indexValue));
        cc->lsl (offset, offset, asmjit::Imm (2));

        scaled = vectorIndexRegs.emplace (indexValue, offset).first;
    }

    // prepareStateAddressing() reserved a register per region the IR touches.
    if (const auto region = stateArrayBaseRegs.find (base); region != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (region->second, scaled->second);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    return asmjit::a64::ptr (addr, scaled->second);
}

void YdspAsmJitCodegenARM64::beginBlock (int blockIndex)
{
    // The scaled index registers are emitted inside the block that uses them,
    // so they are only valid for as long as that block is being emitted.
    (void) blockIndex;
    vectorIndexRegs.clear();
}

void YdspAsmJitCodegenARM64::onValueRedefined (int value)
{
    // The memo below holds `index << 2` in a register, which is only that
    // index's scaling for as long as the index register holds that index. A
    // fully unrolled loop writes its induction variable once per copy, all
    // within one block, so the block-scoped clear above is not enough.
    vectorIndexRegs.erase (value);
}

//==============================================================================
// Loads, stores and moves

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

void YdspAsmJitCodegenARM64::moveGpToFp (const YdspFp& dst, const YdspGp& src)
{
    cc->fmov (dst, src);
}

//==============================================================================
// Arithmetic

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
    // `fmadd d, n, m, a` is d = n * m + a, one instruction and one rounding.
    // Scalar FP is mandatory in ARMv8-A, so unlike x86-64 there is no feature
    // to test and no fallback path on this target.
    cc->fmadd (dst, a, b, c);
}

void YdspAsmJitCodegenARM64::emitVectorFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    // FMLA is dst += a * b, so seed the accumulator from c.
    moveVector (dst, c);
    cc->fmla (dst.s4(), a.s4(), b.s4());
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
        default:
            break;
    }
}

//==============================================================================
// Packed float32 (ASIMD)
//
// The mnemonics are the same as the scalar ones - the vector form is an operand
// width change, so each register is re-typed to `.4s` at the use site.

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

void YdspAsmJitCodegenARM64::emitSplatFloat (const YdspFp& dst, const YdspFp& src)
{
    // DUP's source is a vector *element*, so it has to be addressed through a
    // 128-bit view. Stage the scalar in dst's own lane 0 first, so that view is
    // of a register the allocator already knows is 128-bit - `src` is only ever
    // used at the width it was allocated with.
    cc->fmov (dst.s(), src);
    cc->dup (dst.s4(), dst.s (0));
}

void YdspAsmJitCodegenARM64::emitReduceAddFloat (const YdspFp& dst, const YdspFp& src)
{
    // ASIMD has no float horizontal add (ADDV is integer only), so fold the
    // four lanes pairwise: [a b c d] -> [a+b c+d a+b c+d] -> (a+b) + (c+d).
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
    // SDIV returns 0 for a zero divisor, but MSUB would then compute
    // a - (0 * b) = a for the remainder, not 0. Guard explicitly so
    // both div and mod return 0 on a zero divisor, matching the x86 path.
    asmjit::Label zeroLabel = cc->new_label();
    asmjit::Label doneLabel = cc->new_label();

    branchIfZero (b, zeroLabel);

    YdspGp quotient = is64 ? cc->new_gp64 ("quot") : cc->new_gp32 ("quot");
    cc->sdiv (quotient, a, b);

    if (op == YdspIrOp::divI)
    {
        moveGp (dst, quotient);
    }
    else
    {
        // remainder = a - (a / b) * b
        cc->msub (dst, quotient, b, a);
    }

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
// Comparisons

// FCMP/CMP followed by CSET: two instructions and no branch, where a branch
// diamond would cost five and an unpredictable branch per evaluation.
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
// Branchless select and ring wrap

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

//==============================================================================
// Conversions

void YdspAsmJitCodegenARM64::emitIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    cc->scvtf (dst, src);
}

void YdspAsmJitCodegenARM64::emitFloatToInt (const YdspGp& dst, const YdspFp& src)
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
// Control flow

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

} // namespace yup

#endif // ASMJIT_ARCH_ARM
