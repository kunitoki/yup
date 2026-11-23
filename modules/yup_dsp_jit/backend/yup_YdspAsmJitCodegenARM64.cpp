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
        if (dst != src)
            cc.mov (dst, src);
        return;
    }

    if (imm > 0 && imm <= aarch64ImmediateLimit)
    {
        cc.add (dst, src, asmjit::Imm (imm));
        return;
    }

    if (imm > aarch64ImmediateLimit && imm <= 0xFFFFFF)
    {
        const int32_t lo = imm & aarch64ImmediateLimit;
        const int32_t hi = (imm >> 12) & aarch64ImmediateLimit;

        cc.add (dst, src, asmjit::Imm (hi), asmjit::arm::Shift (asmjit::arm::ShiftOp::kLSL, 12));

        if (lo != 0)
            cc.add (dst, dst, asmjit::Imm (lo));

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
        case asmjit::arm::CondCode::kSign:
            return asmjit::arm::CondCode::kMI;
        case asmjit::arm::CondCode::kNotSign:
            return asmjit::arm::CondCode::kPL;
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

bool YdspAsmJitCodegenARM64::emitZeroFloatConst (const YdspFp& dst, bool is64)
{
    if (is64)
        cc->fmov (dst.d(), asmjit::a64::xzr);
    else
        cc->fmov (dst.s(), asmjit::a64::wzr);

    return true;
}

bool YdspAsmJitCodegenARM64::emitZeroVector (const YdspFp& dst)
{
    cc->movi (dst.b16(), asmjit::Imm (0));
    return true;
}

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

YdspAsmJitCodegenARM64::YdspFp YdspAsmJitCodegenARM64::newIntVector (const char* name)
{
    return cc->new_reg<YdspFp> (asmjit::TypeId::kInt32x4, name);
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

int YdspAsmJitCodegenARM64::estimateLiveGpRegs (const YdspIrFunction& fn, int scope) const
{
    const int numBlocks = static_cast<int> (fn.blocks.size());

    const auto inScope = [&] (int block) noexcept -> bool
    {
        if (block < 0 || block >= numBlocks)
            return false;

        if (scope < 0)
            return blockLoopScope[static_cast<size_t> (block)] < 0;

        const auto& loop = fn.loops[static_cast<size_t> (scope)];
        return block >= loop.headerBlock && block < std::min (loop.exitBlock, numBlocks);
    };

    bool usesStateArrays = false;
    bool usesState = false;
    bool usesBlockSize = false;
    bool usesParams = false;
    bool usesParamOut = false;

    for (int b = 0; b < numBlocks; ++b)
    {
        if (! inScope (b))
            continue;

        for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
        {
            usesStateArrays = usesStateArrays || inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::storeStateArrayF
                           || inst.op == YdspIrOp::loadStateArrayI || inst.op == YdspIrOp::storeStateArrayI;
            usesState = usesState || inst.op == YdspIrOp::loadStateF || inst.op == YdspIrOp::storeStateF
                     || inst.op == YdspIrOp::loadStateI || inst.op == YdspIrOp::storeStateI;
            usesBlockSize = usesBlockSize || inst.op == YdspIrOp::loadBlockSize;
            usesParams = usesParams || inst.op == YdspIrOp::loadParam || inst.op == YdspIrOp::storeParam;
            usesParamOut = usesParamOut || inst.op == YdspIrOp::loadParamOut || inst.op == YdspIrOp::storeParamOut;
        }
    }

    int streamPtrs = 0;

    for (const auto& [key, reg] : streamBaseRegs)
    {
        (void) reg;
        if (std::get<0> (key) == scope)
            ++streamPtrs;
    }

    const int pointerRegs = streamPtrs + (usesStateArrays ? 1 : 0) + (usesState ? 1 : 0)
                          + (usesBlockSize ? 1 : 0) + (usesParams ? 1 : 0) + (usesParamOut ? 1 : 0);

    const auto gpRegIdOf = [this] (int value) noexcept -> uint32_t
    {
        if (value < 0 || static_cast<size_t> (value) >= regs.size() || elidedIntegerConstants.count (value) != 0)
            return asmjit::Globals::kInvalidId;

        const auto& reg = regs[static_cast<size_t> (value)];
        return (reg.is_valid() && reg.is_gp()) ? reg.id() : asmjit::Globals::kInvalidId;
    };

    std::unordered_map<uint32_t, int> defBlockInScope;
    std::unordered_set<uint32_t> defOutScope;
    std::unordered_set<uint32_t> crossScopeGpIds;

    for (int b = 0; b < numBlocks; ++b)
    {
        const bool inside = inScope (b);

        for (const auto& inst : fn.blocks[static_cast<size_t> (b)].insts)
        {
            const uint32_t id = gpRegIdOf (inst.result);

            if (id == asmjit::Globals::kInvalidId)
                continue;

            if (! inside)
            {
                defOutScope.insert (id);
                continue;
            }

            if (const auto [it, inserted] = defBlockInScope.emplace (id, b); ! inserted && it->second != b)
                crossScopeGpIds.insert (id);
        }
    }

    forEachValueUse (fn, [&] (int block, int, int, int value)
    {
        const uint32_t id = gpRegIdOf (value);

        if (id == asmjit::Globals::kInvalidId)
            return;

        if (inScope (block))
        {
            if (defOutScope.count (id) != 0)
            {
                crossScopeGpIds.insert (id);
                return;
            }

            if (const auto def = defBlockInScope.find (id); def != defBlockInScope.end() && def->second != block)
                crossScopeGpIds.insert (id);
        }
        else if (defBlockInScope.count (id) != 0)
        {
            crossScopeGpIds.insert (id);
        }
    });

    if (scope >= 0)
    {
        const uint32_t inductionId = gpRegIdOf (fn.loops[static_cast<size_t> (scope)].induction);

        if (inductionId != asmjit::Globals::kInvalidId)
            crossScopeGpIds.insert (inductionId);
    }

    int maxLocalGp = 0;

    for (int b = 0; b < numBlocks; ++b)
    {
        if (! inScope (b))
            continue;

        const auto& blk = fn.blocks[static_cast<size_t> (b)];
        const int numInsts = static_cast<int> (blk.insts.size());

        if (numInsts == 0)
            continue;

        std::unordered_map<uint32_t, std::pair<int, int>> intervals;

        const auto recordLocal = [&] (int value, int pos)
        {
            const uint32_t id = gpRegIdOf (value);

            if (id == asmjit::Globals::kInvalidId || crossScopeGpIds.count (id) != 0)
                return;

            auto [it, inserted] = intervals.emplace (id, std::make_pair (pos, pos));

            if (! inserted)
            {
                it->second.first = std::min (it->second.first, pos);
                it->second.second = std::max (it->second.second, pos);
            }
        };

        for (int i = 0; i < numInsts; ++i)
        {
            const auto& inst = blk.insts[static_cast<size_t> (i)];

            if (inst.op == YdspIrOp::constI && elidedIntegerConstants.count (inst.result) != 0)
                continue;

            recordLocal (inst.result, i);

            for (int opIdx = 0; opIdx < 3; ++opIdx)
            {
                if (! isValueIdOperand (inst.op, opIdx))
                    continue;

                const int operand = opIdx == 0 ? inst.a : (opIdx == 1 ? inst.b : inst.c);
                recordLocal (operand, i);
            }
        }

        if (blk.term == YdspIrTerm::branchIf && blk.termCond >= 0)
            recordLocal (blk.termCond, numInsts);

        for (int i = 0; i < numInsts; ++i)
        {
            int liveAtInst = 0;

            for (const auto& [id, span] : intervals)
            {
                (void) id;
                if (span.first <= i && i < span.second)
                    ++liveAtInst;
            }

            maxLocalGp = std::max (maxLocalGp, liveAtInst);
        }
    }

    return pointerRegs + static_cast<int> (crossScopeGpIds.size()) + maxLocalGp;
}

void YdspAsmJitCodegenARM64::prepareStateAddressing (const YdspIrFunction& fn)
{
    stateArrayBaseRegs.clear();
    loopArrayBases.clear();
    lastUnhoistedArrayBase = -1;
    lastUnhoistedArrayBaseReg.reset();

    // A base address made in the prologue is live for the whole kernel, so a
    // call anywhere - a block-rate `exp()`, say - leaves it in memory, to be
    // read back at every element access of a sample loop that calls nothing.
    // Each outermost loop gets its own instead, made in its preheader.
    //
    // Cap hoisted bases per scope based on estimated GP register pressure so
    // high-array-count kernels (e.g. 20-32 delay lines) avoid spilling ring
    // indices or base pointers to the stack; unhoisted bases use zero-temp
    // immediate `add` encoding via addGpImm().
    std::map<std::pair<int, int>, double> references;

    const auto record = [this, &references] (int value, int region, int block)
    {
        if (value < 0 || static_cast<size_t> (value) >= valueTypes.size())
            return;

        const auto scope = blockLoopScope[static_cast<size_t> (block)];
        const auto base = stateArrayBase (valueTypes[static_cast<size_t> (value)], region);
        references[{ scope, base }] += referenceCost (block);
    };

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        for (size_t i = 0; i < fn.blocks[b].insts.size(); ++i)
        {
            const YdspInstPosition pos { static_cast<int> (b), static_cast<int> (i) };

            if (arrayDisplacements.count (pos) != 0 || loopPointerAccesses.count (pos) != 0)
                continue;

            const auto& inst = fn.blocks[b].insts[i];
            switch (inst.op)
            {
                case YdspIrOp::loadStateArrayF:
                case YdspIrOp::loadStateArrayI:
                    record (inst.result, inst.memIndex, static_cast<int> (b));
                    break;

                case YdspIrOp::storeStateArrayF:
                case YdspIrOp::storeStateArrayI:
                    record (inst.b, inst.memIndex, static_cast<int> (b));
                    break;

                default:
                    break;
            }
        }
    }

    struct BaseCandidate
    {
        int base = 0;
        double refs = 0.0;
    };

    std::map<int, std::vector<BaseCandidate>> candidatesByScope;

    for (const auto& [key, refs] : references)
    {
        const auto [scope, base] = key;

        if (base == 0)
        {
            stateArrayBaseRegs.emplace (key, stateArraysReg);
            continue;
        }

        candidatesByScope[scope].push_back ({ base, refs });
    }

    for (auto& [scope, candidates] : candidatesByScope)
    {
        const int estimatedLiveGpRegs = estimateLiveGpRegs (fn, scope);
        const int maxHoistedBases = estimatedLiveGpRegs >= 22
                                      ? 0
                                      : std::clamp (14 - estimatedLiveGpRegs, 2, 8);

        if (maxHoistedBases == 0)
            continue;

        std::sort (candidates.begin(), candidates.end(), [] (const BaseCandidate& a, const BaseCandidate& b)
        {
            if (a.refs != b.refs)
                return a.refs > b.refs;

            return a.base < b.base;
        });

        const int count = std::min (static_cast<int> (candidates.size()), maxHoistedBases);

        for (int idx = 0; idx < count; ++idx)
        {
            const int base = candidates[static_cast<size_t> (idx)].base;
            const double refs = candidates[static_cast<size_t> (idx)].refs;

            YdspGp addr = cc->new_gp64 ("arrayBase");
            stateArrayBaseRegs.emplace (std::make_pair (scope, base), addr);

            if (scope < 0)
                addGpImm (*cc, addr, stateArraysReg, base);
            else
                loopArrayBases[fn.loops[static_cast<size_t> (scope)].headerBlock - 1].emplace_back (base, addr);

            if (auto* virt = cc->virt_reg_by_reg (addr))
                virt->set_weight (weightFor (refs));
        }
    }
}

void YdspAsmJitCodegenARM64::endBlock (int blockIndex)
{
    if (const auto bases = loopArrayBases.find (blockIndex); bases != loopArrayBases.end())
        for (const auto& [base, reg] : bases->second)
            addGpImm (*cc, reg, stateArraysReg, base);
}

void YdspAsmJitCodegenARM64::prepareWrapBounds (const YdspIrFunction& fn)
{
    wrapLastRegs.clear();

    std::unordered_map<int32_t, double> wrapRefs;
    std::unordered_set<int> affectedScopes;

    for (size_t b = 0; b < fn.blocks.size(); ++b)
    {
        for (const auto& inst : fn.blocks[b].insts)
        {
            const auto last = static_cast<int32_t> (inst.ivalue) - 1;

            if (inst.op != YdspIrOp::advanceWrapI || (last >= 0 && last <= aarch64ImmediateLimit))
                continue;

            wrapRefs[last] += referenceCost (static_cast<int> (b));
            affectedScopes.insert (blockLoopScope[b]);
        }
    }

    if (wrapRefs.empty())
        return;

    int maxScopePressure = 0;

    for (const int scope : affectedScopes)
    {
        int hoistedBases = 0;

        for (const auto& [key, reg] : stateArrayBaseRegs)
            if (key.first == scope && reg != stateArraysReg)
                ++hoistedBases;

        maxScopePressure = std::max (maxScopePressure, estimateLiveGpRegs (fn, scope) + hoistedBases);
    }

    const int maxHoistedWraps = maxScopePressure >= 22
                                  ? 0
                                  : std::clamp (20 - maxScopePressure, 2, 6);

    if (maxHoistedWraps == 0)
        return;

    struct WrapCandidate
    {
        int32_t last = 0;
        double refs = 0.0;
    };

    std::vector<WrapCandidate> candidates;
    candidates.reserve (wrapRefs.size());

    for (const auto& [last, refs] : wrapRefs)
        candidates.push_back ({ last, refs });

    std::sort (candidates.begin(), candidates.end(), [] (const WrapCandidate& a, const WrapCandidate& b)
    {
        if (a.refs != b.refs)
            return a.refs > b.refs;

        const bool aNeedsTwoMoves = a.last < 0 || a.last > 0xFFFF;
        const bool bNeedsTwoMoves = b.last < 0 || b.last > 0xFFFF;

        if (aNeedsTwoMoves != bNeedsTwoMoves)
            return aNeedsTwoMoves;

        return a.last < b.last;
    });

    const int count = std::min (static_cast<int> (candidates.size()), maxHoistedWraps);

    for (int idx = 0; idx < count; ++idx)
    {
        const int32_t last = candidates[static_cast<size_t> (idx)].last;
        const double refs = candidates[static_cast<size_t> (idx)].refs;

        YdspGp lastReg = cc->new_gp32 ("wrapLast");
        cc->mov (lastReg, asmjit::Imm (last));
        wrapLastRegs.emplace (last, lastReg);

        if (auto* virt = cc->virt_reg_by_reg (lastReg))
            virt->set_weight (weightFor (refs));
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

    if (const auto it = stateArrayBaseRegs.find ({ blockLoopScope[static_cast<size_t> (currentBlock)], base }); it != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (it->second, gp (indexValue), shift);

    if (lastUnhoistedArrayBase == base && lastUnhoistedArrayBaseReg.is_valid())
        return asmjit::a64::ptr (lastUnhoistedArrayBaseReg, gp (indexValue), shift);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    lastUnhoistedArrayBase = base;
    lastUnhoistedArrayBaseReg = addr;
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

    if (const auto region = stateArrayBaseRegs.find ({ blockLoopScope[static_cast<size_t> (currentBlock)], base }); region != stateArrayBaseRegs.end())
        return asmjit::a64::ptr (region->second, scaled->second);

    if (lastUnhoistedArrayBase == base && lastUnhoistedArrayBaseReg.is_valid())
        return asmjit::a64::ptr (lastUnhoistedArrayBaseReg, scaled->second);

    YdspGp addr = cc->new_gp64 ("addr");
    addGpImm (*cc, addr, stateArraysReg, base);
    lastUnhoistedArrayBase = base;
    lastUnhoistedArrayBaseReg = addr;
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
    lastUnhoistedArrayBase = -1;
    lastUnhoistedArrayBaseReg.reset();
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
    // A whole-register move is resolved at rename, where `fmov` executes:
    // the state rotations of a recurrence sit on its feedback path.
    cc->mov (dst.b16(), src.b16());
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
    if (dst == c)
    {
        cc->fmla (dst.s4(), a.s4(), b.s4());
        return;
    }

    if (dst == a || dst == b)
    {
        YdspFp tmp = newFpVector ("fmaTmp");
        moveVector (tmp, c);
        cc->fmla (tmp.s4(), a.s4(), b.s4());
        moveVector (dst, tmp);
        return;
    }

    moveVector (dst, c);
    cc->fmla (dst.s4(), a.s4(), b.s4());
}

void YdspAsmJitCodegenARM64::emitVectorFusedMultiplySubtract (const YdspFp& dst, const YdspFp& a, const YdspFp& b, const YdspFp& c)
{
    if (dst == c)
    {
        cc->fmls (dst.s4(), a.s4(), b.s4());
        return;
    }

    if (dst == a || dst == b)
    {
        YdspFp tmp = newFpVector ("fmsTmp");
        moveVector (tmp, c);
        cc->fmls (tmp.s4(), a.s4(), b.s4());
        moveVector (dst, tmp);
        return;
    }

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

void YdspAsmJitCodegenARM64::vectorIntBinary (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB, bool saturating)
{
    switch (op)
    {
        case YdspIrOp::addI:
            if (saturating)
                cc->sqadd (dst.s4(), srcA.s4(), srcB.s4());
            else
                cc->add (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::subI:
            if (saturating)
                cc->sqsub (dst.s4(), srcA.s4(), srcB.s4());
            else
                cc->sub (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::mulI:
            if (saturating)
            {
                YdspFp lo = newIntVector ("mulLo");
                YdspFp hi = newIntVector ("mulHi");
                cc->smull (lo.d2(), srcA.s2(), srcB.s2());
                cc->smull2 (hi.d2(), srcA.s4(), srcB.s4());
                cc->sqxtn (dst.s2(), lo.d2());
                cc->sqxtn2 (dst.s4(), hi.d2());
            }
            else
            {
                cc->mul (dst.s4(), srcA.s4(), srcB.s4());
            }
            break;
        case YdspIrOp::minI:
            cc->smin (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::maxI:
            cc->smax (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::andI:
        case YdspIrOp::andB:
            cc->and_ (dst.b16(), srcA.b16(), srcB.b16());
            break;
        case YdspIrOp::orI:
        case YdspIrOp::orB:
            cc->orr (dst.b16(), srcA.b16(), srcB.b16());
            break;
        case YdspIrOp::xorI:
            cc->eor (dst.b16(), srcA.b16(), srcB.b16());
            break;
        case YdspIrOp::shlI:
        case YdspIrOp::shrI:
        {
            YdspGp mask31 = cc->new_gp32 ("shiftMask31");
            cc->mov (mask31, asmjit::Imm (31));
            YdspFp maskVec = newIntVector ("shiftMaskV");
            cc->dup (maskVec.s4(), mask31);
            YdspFp shiftVec = newIntVector ("vecShiftV");
            cc->and_ (shiftVec.b16(), srcB.b16(), maskVec.b16());
            if (op == YdspIrOp::shrI)
                cc->neg (shiftVec.s4(), shiftVec.s4());
            cc->sshl (dst.s4(), srcA.s4(), shiftVec.s4());
            break;
        }
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::vectorIntShift (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspGp& shiftAmount)
{
    YdspGp shift = cc->new_gp32 ("vecShift");
    cc->and_ (shift, shiftAmount, asmjit::Imm (31));

    if (op == YdspIrOp::shrI)
        cc->neg (shift, shift);

    YdspFp shiftVec = newIntVector ("vecShiftV");
    cc->dup (shiftVec.s4(), shift);
    cc->sshl (dst.s4(), srcA.s4(), shiftVec.s4());
}

void YdspAsmJitCodegenARM64::vectorIntUnary (YdspIrOp op, const YdspFp& dst, const YdspFp& src)
{
    switch (op)
    {
        case YdspIrOp::negI:
            cc->sqneg (dst.s4(), src.s4());
            break;
        case YdspIrOp::absI:
            cc->sqabs (dst.s4(), src.s4());
            break;
        case YdspIrOp::notB:
            cc->not_ (dst.b16(), src.b16());
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::vectorIntClamp (const YdspFp& dst, const YdspFp& src, const YdspFp& lo, const YdspFp& hi)
{
    YdspFp maxed = newIntVector ("clampMax");
    cc->smax (maxed.s4(), src.s4(), lo.s4());
    cc->smin (dst.s4(), maxed.s4(), hi.s4());
}

void YdspAsmJitCodegenARM64::vectorIntCompare (YdspIrOp op, const YdspFp& dst, const YdspFp& srcA, const YdspFp& srcB)
{
    switch (op)
    {
        case YdspIrOp::eqI:
            cc->cmeq (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::neI:
            cc->cmeq (dst.s4(), srcA.s4(), srcB.s4());
            cc->not_ (dst.b16(), dst.b16());
            break;
        case YdspIrOp::ltI:
            cc->cmgt (dst.s4(), srcB.s4(), srcA.s4());
            break;
        case YdspIrOp::leI:
            cc->cmge (dst.s4(), srcB.s4(), srcA.s4());
            break;
        case YdspIrOp::gtI:
            cc->cmgt (dst.s4(), srcA.s4(), srcB.s4());
            break;
        case YdspIrOp::geI:
            cc->cmge (dst.s4(), srcA.s4(), srcB.s4());
            break;
        default:
            break;
    }
}

void YdspAsmJitCodegenARM64::vectorSelectInt (const YdspFp& mask, const YdspFp& dst, const YdspFp& whenTrue, const YdspFp& whenFalse)
{
    YdspFp selected = newIntVector ("sel");
    moveVector (selected, mask);
    cc->bsl (selected.b16(), whenTrue.b16(), whenFalse.b16());
    moveVector (dst, selected);
}

void YdspAsmJitCodegenARM64::vectorIntToFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->scvtf (dst.s4(), src.s4());
}

void YdspAsmJitCodegenARM64::vectorFloatToInt (const YdspFp& dst, const YdspFp& src, bool)
{
    cc->fcvtzs (dst.s4(), src.s4());
}

void YdspAsmJitCodegenARM64::emitSplatFloat (const YdspFp& dst, const YdspFp& src)
{
    cc->fmov (dst.s(), src);
    cc->dup (dst.s4(), dst.s (0));
}

void YdspAsmJitCodegenARM64::emitSplatInt (const YdspFp& dst, const YdspGp& src)
{
    cc->dup (dst.s4(), src);
}

void YdspAsmJitCodegenARM64::emitReduceAddFloat (const YdspFp& dst, const YdspFp& src)
{
    YdspFp pairs = newFpVector ("reducePairs");

    cc->faddp (pairs.s4(), src.s4(), src.s4());
    cc->faddp (dst, pairs.s2());
}

void YdspAsmJitCodegenARM64::emitInsertFloat (const YdspFp& dst, const YdspFp& srcVec, const YdspFp& srcScalar, int lane)
{
    jassert (lane >= 0 && lane < 4);

    if (! srcVec.is_valid())
    {
        if (lane == 0)
        {
            cc->fmov (dst.s(), srcScalar.s());
            return;
        }

        emitZeroVector (dst);
    }
    else if (dst != srcVec)
    {
        moveVector (dst, srcVec);
    }

    cc->ins (dst.s (static_cast<uint32_t> (lane)), srcScalar.s (0));
}

void YdspAsmJitCodegenARM64::emitExtractFloat (const YdspFp& dst, const YdspFp& srcVec, int lane)
{
    jassert (lane >= 0 && lane < 4);

    if (lane == 0)
    {
        cc->fmov (dst.s(), srcVec.s());
        return;
    }

    cc->dup (dst.s(), srcVec.s (static_cast<uint32_t> (lane)));
}

void YdspAsmJitCodegenARM64::emitBitcastFloatToInt (const YdspGp& dst, const YdspFp& src)
{
    cc->fmov (dst.w(), src.s());
}

void YdspAsmJitCodegenARM64::emitBitcastIntToFloat (const YdspFp& dst, const YdspGp& src)
{
    cc->fmov (dst.s(), src.w());
}

void YdspAsmJitCodegenARM64::emitAddPointerImm (const YdspGp& dst, const YdspGp& src, int32_t imm)
{
    if (imm >= 0)
    {
        addGpImm (*cc, dst, src, imm);
        return;
    }

    YdspGp tmp = cc->new_gp64 ("imm");
    cc->mov (tmp, asmjit::Imm (imm));
    cc->add (dst, src, tmp);
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
    // Tests the old value so the increment folds into the select: the ring
    // position is carried from sample to sample through two instructions.
    const auto last = bound - 1;

    if (last >= 0 && last <= aarch64ImmediateLimit)
    {
        cc->cmp (value, asmjit::Imm (last));
    }
    else if (const auto hoisted = wrapLastRegs.find (last); hoisted != wrapLastRegs.end())
    {
        cc->cmp (value, hoisted->second);
    }
    else
    {
        YdspGp lastReg = cc->new_gp32 ("wrapLast");
        cc->mov (lastReg, asmjit::Imm (last));
        cc->cmp (value, lastReg);
    }

    cc->csinc (dst, asmjit::a64::wzr, value, asmjit::Imm (static_cast<uint32_t> (asmjit::arm::CondCode::kGE)));
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
