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

#pragma once

#if ASMJIT_ARCH_X86

#include "yup_YdspAsmJitCodegen.h"

namespace yup
{

//==============================================================================
/** x86-64 (SSE) lowering of the Ydsp IR.

    Implements every architecture hook of YdspAsmJitCodegenImpl with SSE2
    instructions; integer division/modulo by a zero divisor call the shared
    yupDspIdiv/yupDspImod helpers (which return 0) instead of trapping.

    @internal
*/
class YdspAsmJitCodegenX64 : public YdspAsmJitCodegenImpl
{
protected:
    YdspMem memPtr (const YdspGp& base, int32_t offset) const override;
    YdspMem memPtrIndexed (const YdspGp& base, const YdspGp& index, uint32_t scaleLog2, int32_t offset) const override;
    YdspMem emitStateMem (YdspValueType type, int base, int indexValue) override;
    YdspMem emitVectorStateMem (int base, int indexValue) override;

    void loadGpFromMem (const YdspGp& dst, const YdspMem& src) override;
    void storeGpToMem (const YdspMem& dst, const YdspGp& src) override;
    void loadFloatFromMem (const YdspFp& dst, const YdspMem& src) override;
    void storeFloatToMem (const YdspMem& dst, const YdspFp& src) override;
    void moveFloat (const YdspFp& dst, const YdspFp& src) override;
    void moveGpToFp (const YdspFp& dst, const YdspGp& src) override;

    void floatBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) override;
    void floatUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src, YdspValueType type) override;
    void emitFusedMultiplyAdd (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c) override;

    void loadVectorFromMem (const YdspFp& dst, const YdspMem& src) override;
    void storeVectorToMem (const YdspMem& dst, const YdspFp& src) override;
    void moveVector (const YdspFp& dst, const YdspFp& src) override;
    void vectorBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB) override;
    void vectorUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src) override;
    void emitSplatFloat (const YdspFp& dst, const YdspFp& src) override;
    void emitReduceAddFloat (const YdspFp& dst, const YdspFp& src) override;

    void roundFloat (const YdspFp& reg, int mode) override;
    void intBinary (YdspIrOp op, const YdspGp& dst, const YdspGp& srcA, const YdspGp& srcB) override;
    void intUnaryNeg (const YdspGp& dst, const YdspGp& src) override;
    void emitIntDivision (YdspIrOp op, const YdspGp& dst, const YdspGp& a, const YdspGp& b, bool is64) override;
    void emitNotB (const YdspGp& dst, const YdspGp& src) override;

    void emitFloatCompare (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst) override;
    void emitIntCompare (YdspCond cond, const YdspGp& a, const YdspGp& b, const YdspGp& dst) override;
    void emitFloatCompareToReg (YdspCond cond, const YdspFp& a, const YdspFp& b, const YdspGp& dst) override;

    void emitSelectFloat (const YdspGp& cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) override;
    void emitSelectInt (const YdspGp& cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse) override;
    void emitFloatCompareToFlags (const YdspFp& a, const YdspFp& b) override;
    void emitIntCompareToFlags (const YdspGp& a, const YdspGp& b) override;
    void emitSelectFloatOnFlags (YdspCond cond, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse) override;
    void emitSelectIntOnFlags (YdspCond cond, const YdspGp& dst, const YdspGp& whenTrue, const YdspGp& whenFalse) override;
    void emitWrapInt (const YdspGp& dst, const YdspGp& value, const YdspGp& bound) override;

    void emitIntToFloat (const YdspFp& dst, const YdspGp& src) override;
    void emitFloatToInt (const YdspGp& dst, const YdspFp& src) override;
    void emitExtendInt (const YdspGp& dst, const YdspGp& src) override;
    void emitTruncateInt (const YdspGp& dst, const YdspGp& src) override;
    void emitExtendFloat (const YdspFp& dst, const YdspFp& src) override;
    void emitTruncateFloat (const YdspFp& dst, const YdspFp& src) override;

    void jump (const asmjit::Label& target) override;
    void branchIfZero (const YdspGp& cond, const asmjit::Label& target) override;
    void branchIfNotZero (const YdspGp& cond, const asmjit::Label& target) override;

private:
    /** True when `reg` holds a float64 rather than a float32.

        Unlike AArch64, x86 has no distinct register class per float width: a
        f32 scalar, a f64 scalar and a packed f32 vector are all a 128-bit Xmm,
        so the operand itself cannot answer this. The width lives on the
        register's VirtReg, as the asmjit::TypeId it was created with. */
    bool isDoubleFloat (const YdspFp& reg) const;

    // Blends `whenTrue`/`whenFalse` using an all-ones / all-zeros 32-bit mask.
    void blendFloatOnMask (const YdspGp& maskBits, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse);

    // Reused invoke node for 32-bit integer division/modulo.
    asmjit::InvokeNode* divInvoke = nullptr;
};

} // namespace yup

#endif // ASMJIT_ARCH_X86
