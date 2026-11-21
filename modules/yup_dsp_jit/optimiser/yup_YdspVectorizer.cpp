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

//==============================================================================

template <typename Callback>
void forEachValueOperand (const YdspIrInst& inst, Callback&& callback)
{
    if (ydspFirstOperandIsSlot (inst.op))
        return;

    if (inst.a >= 0)
        callback (inst.a);

    if (inst.b >= 0)
        callback (inst.b);

    if (inst.c >= 0)
        callback (inst.c);
}

bool isPackedFloatOp (YdspIrOp op, bool allowVectorMath, bool allowPackedFma) noexcept
{
    switch (op)
    {
        case YdspIrOp::addF:
        case YdspIrOp::subF:
        case YdspIrOp::mulF:
        case YdspIrOp::divF:
        case YdspIrOp::minF:
        case YdspIrOp::maxF:
        case YdspIrOp::negF:
        case YdspIrOp::absF:
        case YdspIrOp::sqrtF:
        case YdspIrOp::floorF:
        case YdspIrOp::ceilF:
        case YdspIrOp::rintF:
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::movF:
            return true;

        default:
            break;
    }

    // A spelled fma()/fmsub() may widen only when the target actually has a
    // packed fused multiply-add: x86's vector FMA lowering emits vfmadd213ps
    // unconditionally, so an SSE2 target would fault on the widened loop (the
    // contraction pass forms widened fmaF under the same flag).
    if (allowPackedFma)
    {
        switch (op)
        {
            case YdspIrOp::fmaF:
            case YdspIrOp::fmsubF:
                return true;

            default:
                break;
        }
    }

    // The SLEEF-backed transcendental family: only reachable once the target
    // reported vector math (native builds link sleef_library). Each op here
    // must have a 4-lane Sleef_<name>f4 counterpart the codegen can call, or a
    // widened loop would silently lower to nothing.
    if (allowVectorMath)
    {
        switch (op)
        {
            case YdspIrOp::sinF:
            case YdspIrOp::cosF:
            case YdspIrOp::tanF:
            case YdspIrOp::asinF:
            case YdspIrOp::acosF:
            case YdspIrOp::atanF:
            case YdspIrOp::sinhF:
            case YdspIrOp::coshF:
            case YdspIrOp::tanhF:
            case YdspIrOp::asinhF:
            case YdspIrOp::acoshF:
            case YdspIrOp::atanhF:
            case YdspIrOp::expF:
            case YdspIrOp::logF:
            case YdspIrOp::log10F:
            case YdspIrOp::powF:
            case YdspIrOp::atan2F:
            case YdspIrOp::fmodF:
                return true;

            default:
                return false;
        }
    }

    return false;
}

bool isStateArrayAccess (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadStateArrayF:
        case YdspIrOp::storeStateArrayF:
        case YdspIrOp::loadStateArrayI:
        case YdspIrOp::storeStateArrayI:
            return true;

        default:
            return false;
    }
}

bool isStreamAccess (YdspIrOp op) noexcept
{
    switch (op)
    {
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
        case YdspIrOp::storeOutput:
            return true;

        default:
            return false;
    }
}

//==============================================================================
/** Widens one loop, or leaves it exactly as it was.

    Analysis never mutates the function: every legality test runs first and only
    a fully-proven loop reaches rewrite(), so a rejected loop costs nothing but
    the analysis.
*/
class LoopWidener
{
public:
    LoopWidener (YdspIrFunction& functionToWiden, const YdspIrLoop& loopToWiden, int width, bool scalarOnlyContractionToWiden)
        : fn (functionToWiden)
        , loop (loopToWiden)
        , lanes (width)
        , scalarOnlyContraction (scalarOnlyContractionToWiden)
    {
    }

    bool tryWiden (YdspVectorizationReason& reason)
    {
        if (! analyse())
        {
            reason = rejectionReason;
            return false;
        }

        rewrite();
        return true;
    }

private:
    /** Records why the loop was rejected; the only way a legality test may
        return false. */
    bool fail (YdspVectorizationReason reason) noexcept
    {
        rejectionReason = reason;
        return false;
    }

    /** True when the loop body holds a multiply whose only use is one add or
        subtract - exactly the chain contractMultiplyAdd fuses into
        fmaF/fmsubF if the loop stays scalar (same single-block scan, see
        yup_YdspPassesContractMultiplyAdd.cpp).

        The caller applies this only to an implicit per-sample (runtime
        blockSize-bound) stream loop: a target that contracts without a packed
        fused multiply-add cannot fuse the widened form of such a chain, so
        widening the loop would make it round twice. Keeping it scalar lets the
        contraction pass fuse it and lower it through the exact float64
        expansion that rounds once. Constant-bound bank loops are exempt - on
        such a target they stay widened and unfused by design.
    */
    bool hasContractionFusableChain() const
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        const auto isF32 = [this] (int value)
        {
            return value >= 0
                && static_cast<size_t> (value) < fn.valueTypes.size()
                && fn.valueTypes[static_cast<size_t> (value)] == YdspValueType::float32Type;
        };

        const auto findDef = [&insts] (int value, size_t upTo) -> size_t
        {
            for (size_t j = upTo; j-- > 0;)
                if (insts[j].result == value)
                    return j;

            return static_cast<size_t> (-1);
        };

        for (size_t i = 0; i < insts.size(); ++i)
        {
            const auto& add = insts[i];

            if ((add.op != YdspIrOp::addF && add.op != YdspIrOp::subF) || ! isF32 (add.result))
                continue;

            for (const auto operand : { add.a, add.b })
            {
                // Both `a*b + c` and `a*b - c` fuse: contractMultiplyAdd turns a
                // multiply on either side of a subF into fmaF, negating the
                // addend for `a*b - c`, so a multiply feeding a subF is a fusable
                // chain from either operand position.
                if (operand < 0 || ! isF32 (operand)
                    || defsInBody[static_cast<size_t> (operand)] != 1
                    || defsElsewhere[static_cast<size_t> (operand)] != 0
                    || usesInBody[static_cast<size_t> (operand)] != 1
                    || usesElsewhere[static_cast<size_t> (operand)] != 0)
                    continue;

                const auto mulIndex = findDef (operand, i);
                if (mulIndex == static_cast<size_t> (-1))
                    continue;

                const auto& mul = insts[mulIndex];

                if (mul.op != YdspIrOp::mulF || ! isF32 (mul.a) || ! isF32 (mul.b))
                    continue;

                // Nothing between the multiply and the add may redefine a factor.
                bool interference = false;

                for (size_t k = mulIndex + 1; k < i; ++k)
                    if (insts[k].result == mul.a || insts[k].result == mul.b)
                    {
                        interference = true;
                        break;
                    }

                if (interference)
                    continue;

                if (isF32 (operand == add.a ? add.b : add.a))
                    return true;
            }
        }

        return false;
    }

    /** One host stream: an input or an output endpoint slot. */
    struct StreamRef
    {
        bool isInput = false;
        int slot = -1;

        bool operator== (const StreamRef& other) const noexcept { return isInput == other.isInput && slot == other.slot; }
    };

    /** One recognised `acc = acc + x` accumulation in the loop body. */
    struct Reduction
    {
        int accumulator = -1; // the scalar the loop accumulates into
        int addIndex = -1;    // body index of the accumulating addF
        int moveIndex = -1;   // body index of the `movF acc, sum`
        int addend = -1;      // the add's non-accumulator operand
        int vectorAcc = -1;   // the vector accumulator (filled in by the rewrite)
    };

    //==============================================================================
    // Analysis

    bool analyse()
    {
        if (loop.bound.kind != YdspLoopBoundKind::constant
            && loop.bound.kind != YdspLoopBoundKind::blockSize
            && loop.bound.kind != YdspLoopBoundKind::blockSizeMinusConst
            && loop.bound.kind != YdspLoopBoundKind::blockSizePlusConst)
        {
            return fail (YdspVectorizationReason::unsupportedLoopBound);
        }

        header = loop.headerBlock;
        body = header + 1;
        exit = loop.exitBlock;
        preheader = header - 1;

        if (preheader < 0 || body < 0 || exit != body + 1)
            return fail (YdspVectorizationReason::multiBlockBody); // a multi-block (or nested) body

        if (static_cast<size_t> (exit) >= fn.blocks.size())
            return fail (YdspVectorizationReason::multiBlockBody);

        const auto& bodyBlock = fn.blocks[static_cast<size_t> (body)];

        if (bodyBlock.term != YdspIrTerm::branch || bodyBlock.termTarget != header)
            return fail (YdspVectorizationReason::multiBlockBody);

        numValues = static_cast<int> (fn.valueTypes.size());

        if (numValues <= 0)
            return fail (YdspVectorizationReason::notVectorizable);

        defsInBody.assign (static_cast<size_t> (numValues), 0);
        defsElsewhere.assign (static_cast<size_t> (numValues), 0);
        usesInBody.assign (static_cast<size_t> (numValues), 0);
        usesElsewhere.assign (static_cast<size_t> (numValues), 0);
        definedInHeader.assign (static_cast<size_t> (numValues), false);

        if (! countDefsAndUses())
            return fail (YdspVectorizationReason::notVectorizable);

        if (loop.bound.kind == YdspLoopBoundKind::constant)
        {
            if (loop.bound.constant <= 0)
                return fail (YdspVectorizationReason::unsupportedLoopBound);

            if (! findLoopStart())
                return fail (YdspVectorizationReason::nonConstantStart);

            const auto span = loop.bound.constant - start;

            if (span < lanes)
                return fail (YdspVectorizationReason::shortTripCount); // no full vector trip: the loop is all tail

            tail = span % lanes;
        }
        else
        {
            if (! findHeaderCompare())
                return fail (YdspVectorizationReason::missingHeaderCompare);

            if (! findLoopStart())
                return fail (YdspVectorizationReason::nonConstantStart);

            if (start < 0)
                return fail (YdspVectorizationReason::nonzeroStart);
        }

        if (! findInductionUpdate())
            return fail (YdspVectorizationReason::unrecognizedInductionUpdate);

        findInductionRamps();

        if (! checkInductionUses())
            return fail (YdspVectorizationReason::inductionUsedAsValue);

        if (! findReductions())
            return fail (YdspVectorizationReason::unrecognizedAccumulation);

        if (! classifyBody())
            return false; // rejectionReason was set by the failing case

        for (const auto& read : fn.blocks[static_cast<size_t> (body)].insts)
        {
            if ((read.op != YdspIrOp::loadStateArrayF && read.op != YdspIrOp::loadOutput)
                || read.a == loop.induction)
                continue;

            const auto store = read.op == YdspIrOp::loadOutput ? YdspIrOp::storeOutput : YdspIrOp::storeStateArrayF;
            for (const auto& write : fn.blocks[static_cast<size_t> (body)].insts)
                if (write.op == store && write.memIndex == read.memIndex)
                    return fail (YdspVectorizationReason::indirectAccess);
        }

        if (! collectStreamGuards())
            return fail (YdspVectorizationReason::streamOverlapUnprovable);

        if (! checkSingleDefinitionAndLiveOut())
            return fail (YdspVectorizationReason::loopCarriedValue);

        if (! checkTailFeasibility())
            return fail (YdspVectorizationReason::runtimeBoundWithoutStreams);

        if (widenedAccessCount == 0)
            return fail (YdspVectorizationReason::nothingToWiden);

        // Only the implicit per-sample stream loop - a runtime blockSize bound
        // - is kept scalar when it holds a fusable mul->add/sub chain and the
        // target contracts without a packed fused multiply-add: that chain is
        // the language-level expression the contraction pass fuses, so it must
        // round once. A constant-bound bank `for i in 0..N` loop stays a SIMD
        // shape: it is widened and left unfused on such a target (see
        // hasContractionFusableChain), with per-lane mul/add rounding twice by
        // design.
        if (scalarOnlyContraction
            && loop.bound.kind != YdspLoopBoundKind::constant
            && hasContractionFusableChain())
            return fail (YdspVectorizationReason::keptScalarForContraction);

        return true;
    }

    bool findLoopStart()
    {
        for (const auto& inst : fn.blocks[static_cast<size_t> (preheader)].insts)
            if (inst.op == YdspIrOp::movI && inst.result == loop.induction && inst.a >= 0)
                return readConstantInt (inst.a, start);

        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            if (static_cast<int> (b) == body)
                continue;

            for (const auto& inst : fn.blocks[b].insts)
                if (inst.result == loop.induction && inst.op == YdspIrOp::constI)
                {
                    start = static_cast<int> (inst.ivalue);
                    return true;
                }
        }

        return false;
    }

    bool readConstantInt (int value, int& out) const
    {
        if (value < 0 || value >= numValues)
            return false;

        if (defsInBody[static_cast<size_t> (value)] + defsElsewhere[static_cast<size_t> (value)] != 1)
            return false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result == value)
                {
                    if (inst.op != YdspIrOp::constI)
                        return false;

                    out = static_cast<int> (inst.ivalue);
                    return true;
                }
            }
        }

        return false;
    }

    bool findHeaderCompare()
    {
        int matches = 0;

        for (const auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == loop.induction && inst.b >= 0)
                ++matches;

        return matches == 1;
    }

    /** Finds the stream pairs a widened body could reorder if the host
        buffers overlap.

        The scalar loop interleaves its accesses sample by sample; the widened
        loop reads and writes `lanes` samples at a time, so a store can land on
        a sample another access of the same vector has yet to reach, or has
        already passed. That needs one of the two accesses to be a store and
        their streams to sit closer than one vector without being the same
        buffer, which only the run time addresses can tell.

        Returns false when no run time check can clear the loop: a store
        beside a read at a fixed index (the rewrite hoists that read out of the
        loop entirely), or a constant-bound loop, which has no scalar remainder
        to fall back to.
    */
    bool collectStreamGuards()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        std::vector<StreamRef> stores, accesses;
        bool hasFixedIndexRead = false;

        const auto addUnique = [] (std::vector<StreamRef>& list, StreamRef stream)
        {
            if (std::find (list.begin(), list.end(), stream) == list.end())
                list.push_back (stream);
        };

        for (const auto& inst : insts)
        {
            if (! isStreamAccess (inst.op))
                continue;

            const StreamRef stream { inst.op == YdspIrOp::loadInput, inst.memIndex };

            if (inst.a != loop.induction)
            {
                hasFixedIndexRead = true;
                continue;
            }

            addUnique (accesses, stream);

            if (inst.op == YdspIrOp::storeOutput)
                addUnique (stores, stream);
        }

        for (const auto& store : stores)
            for (const auto& other : accesses)
                if (! (store == other)
                    && std::find (streamGuards.begin(), streamGuards.end(), std::make_pair (other, store)) == streamGuards.end())
                    streamGuards.emplace_back (store, other);

        if (stores.empty())
            return true;

        return ! hasFixedIndexRead
            && (streamGuards.empty() || loop.bound.kind != YdspLoopBoundKind::constant);
    }

    bool checkTailFeasibility() const
    {
        if (loop.bound.kind != YdspLoopBoundKind::constant && ! hasStreamAccess)
            return false;

        return true;
    }

    bool countDefsAndUses()
    {
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const bool inBody = (static_cast<int> (b) == body);
            const auto& block = fn.blocks[b];

            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0)
                {
                    if (inst.result >= numValues)
                        return false;

                    ++(inBody ? defsInBody : defsElsewhere)[static_cast<size_t> (inst.result)];

                    if (static_cast<int> (b) == header)
                        definedInHeader[static_cast<size_t> (inst.result)] = true;
                }

                bool operandsValid = true;

                forEachValueOperand (inst, [&] (int value)
                {
                    if (value >= numValues)
                    {
                        operandsValid = false;
                        return;
                    }

                    ++(inBody ? usesInBody : usesElsewhere)[static_cast<size_t> (value)];
                });

                if (! operandsValid)
                    return false;
            }

            if (block.termCond >= 0)
            {
                if (block.termCond >= numValues)
                    return false;

                ++(inBody ? usesInBody : usesElsewhere)[static_cast<size_t> (block.termCond)];
            }
        }

        return true;
    }

    bool findInductionUpdate()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        if (insts.size() < 2)
            return false;

        const auto last = static_cast<int> (insts.size()) - 1;
        const auto& move = insts[static_cast<size_t> (last)];

        if (move.op != YdspIrOp::movI || move.result != loop.induction || move.a < 0)
            return false;

        inductionMoveIndex = last;

        for (int j = last; j-- > 0;)
        {
            if (insts[static_cast<size_t> (j)].result == move.a)
            {
                incrementIndex = j;
                break;
            }
        }

        if (incrementIndex < 0)
            return false;

        const auto& increment = insts[static_cast<size_t> (incrementIndex)];

        if (increment.op != YdspIrOp::addI || increment.a != loop.induction)
            return false;

        if (! isConstantInt (increment.b, 1))
            return false;

        const auto step = static_cast<size_t> (increment.result);

        return defsInBody[step] == 1 && defsElsewhere[step] == 0
            && usesInBody[step] == 1 && usesElsewhere[step] == 0;
    }

    bool isConstantInt (int value, int64_t expected) const
    {
        if (value < 0 || value >= numValues)
            return false;

        if (defsInBody[static_cast<size_t> (value)] + defsElsewhere[static_cast<size_t> (value)] != 1)
            return false;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.result == value)
                    return inst.op == YdspIrOp::constI && inst.ivalue == expected;
            }
        }

        return false;
    }

    bool checkInductionUses() const
    {
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];

            for (size_t j = 0; j < block.insts.size(); ++j)
            {
                const auto& inst = block.insts[j];

                bool readsInduction = false;

                forEachValueOperand (inst, [&] (int value)
                {
                    if (value == loop.induction)
                        readsInduction = true;
                });

                if (! readsInduction)
                    continue;

                if (static_cast<int> (b) == header)
                    continue;

                if (static_cast<int> (b) != body)
                    return false;

                if (static_cast<int> (j) == incrementIndex || isRamp[j] || isRampOffset[j])
                    continue;

                const bool indexOnly = (isStateArrayAccess (inst.op) || isStreamAccess (inst.op))
                                    && inst.a == loop.induction
                                    && inst.b != loop.induction
                                    && inst.c != loop.induction;

                if (! indexOnly)
                    return false;
            }

            if (block.termCond == loop.induction && static_cast<int> (b) != header)
                return false;
        }

        return true;
    }

    /** Finds the conversions of the loop variable to float: `float (i)` and
        `float (i + c)`, which widen to a vector counting up from the scalar.

        Only for a constant bound, where every value involved is far below
        2^24: there `float (i + c + k)` and `float (i + c) + k` are the same
        number, so the widened loop rounds like the scalar one.
    */
    void findInductionRamps()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        isRamp.assign (insts.size(), false);
        isRampOffset.assign (insts.size(), false);

        constexpr int largestExactCount = 1 << 20;

        if (! fn.vectorRampAvailable || loop.bound.kind != YdspLoopBoundKind::constant || loop.bound.constant > largestExactCount)
            return;

        const auto isFloat32Conversion = [&] (const YdspIrInst& inst, int source)
        {
            return inst.op == YdspIrOp::itof && inst.a == source
                && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type;
        };

        // The offset value may feed nothing but float32 conversions: any other
        // reader would see one value per vector instead of one per element.
        const auto convertsOnly = [&] (int value, size_t definedAt)
        {
            const auto v = static_cast<size_t> (value);

            if (defsInBody[v] != 1 || defsElsewhere[v] != 0 || usesElsewhere[v] != 0)
                return false;

            for (size_t k = 0; k < insts.size(); ++k)
            {
                const auto& inst = insts[k];

                if (inst.a != value && inst.b != value && inst.c != value)
                    continue;

                if (k < definedAt || ! isFloat32Conversion (inst, value))
                    return false;
            }

            return true;
        };

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& inst = insts[j];

            if (static_cast<int> (j) == incrementIndex || inst.result < 0)
                continue;

            if (isFloat32Conversion (inst, loop.induction))
            {
                isRamp[j] = true;
                continue;
            }

            int offset = 0;

            const bool offsetsInduction = (inst.op == YdspIrOp::addI && inst.a == loop.induction && readConstantInt (inst.b, offset))
                                       || (inst.op == YdspIrOp::addI && inst.b == loop.induction && readConstantInt (inst.a, offset))
                                       || (inst.op == YdspIrOp::subI && inst.a == loop.induction && readConstantInt (inst.b, offset));

            if (! offsetsInduction || std::abs (offset) > largestExactCount || ! convertsOnly (inst.result, j))
                continue;

            isRampOffset[j] = true;

            for (size_t k = j + 1; k < insts.size(); ++k)
                if (isFloat32Conversion (insts[k], inst.result))
                    isRamp[k] = true;
        }
    }

    bool findReductions()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& move = insts[j];

            if (move.op != YdspIrOp::movF || move.result < 0)
                continue;

            const auto accumulator = static_cast<size_t> (move.result);

            if (defsElsewhere[accumulator] == 0 && usesElsewhere[accumulator] == 0)
                continue;

            if (defsElsewhere[accumulator] == 0 || defsInBody[accumulator] != 1)
                return false;

            if (fn.valueTypes[accumulator] != YdspValueType::float32Type)
                return false;

            int addIndex = -1;

            for (size_t k = j; k-- > 0;)
            {
                if (insts[k].result == move.a)
                {
                    addIndex = static_cast<int> (k);
                    break;
                }
            }

            if (addIndex < 0)
                return false;

            const auto& add = insts[static_cast<size_t> (addIndex)];

            if (add.op != YdspIrOp::addF)
                return false;

            const auto sum = static_cast<size_t> (add.result);

            if (defsInBody[sum] != 1 || defsElsewhere[sum] != 0
                || usesInBody[sum] != 1 || usesElsewhere[sum] != 0)
                return false;

            if (usesInBody[accumulator] != 1)
                return false;

            const auto addend = add.a == move.result ? add.b
                              : add.b == move.result ? add.a
                                                     : -1;

            if (addend < 0 || addend == move.result)
                return false;

            if (fn.valueTypes[static_cast<size_t> (addend)] != YdspValueType::float32Type
                || fn.valueTypes[sum] != YdspValueType::float32Type)
                return false;

            reductions.push_back ({ move.result, addIndex, static_cast<int> (j), addend, -1 });
        }

        return true;
    }

    bool classifyBody()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        isVector.assign (static_cast<size_t> (numValues), false);
        widened.assign (insts.size(), false);

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& inst = insts[j];

            if (static_cast<int> (j) == incrementIndex || static_cast<int> (j) == inductionMoveIndex)
                continue;

            if (const auto* reduction = reductionAt (static_cast<int> (j)))
            {
                if (reduction->addIndex == static_cast<int> (j))
                {
                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                }

                continue;
            }

            if (isRamp[j])
            {
                widened[j] = true;
                isVector[static_cast<size_t> (inst.result)] = true;
                continue;
            }

            switch (inst.op)
            {
                case YdspIrOp::eqF:
                case YdspIrOp::neF:
                case YdspIrOp::ltF:
                case YdspIrOp::leF:
                case YdspIrOp::gtF:
                case YdspIrOp::geF:
                {
                    bool readsVector = false;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        readsVector = readsVector || isVector[static_cast<size_t> (value)];
                    });

                    if (! readsVector)
                        break;

                    if (inst.result < 0 || fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::boolType)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                    break;
                }

                case YdspIrOp::selectB:
                {
                    bool readsVector = false;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        readsVector = readsVector || isVector[static_cast<size_t> (value)];
                    });

                    if (! readsVector)
                        break;

                    if (inst.result < 0 || inst.a < 0 || inst.b < 0 || inst.c < 0
                        || ! isVector[static_cast<size_t> (inst.a)]
                        || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type
                        || fn.valueTypes[static_cast<size_t> (inst.c)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                    break;
                }

                case YdspIrOp::loadStateArrayF:
                {
                    if (inst.a == loop.induction)
                    {
                        if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                            return fail (YdspVectorizationReason::unsupportedWidenedType);

                        widened[j] = true;
                        isVector[static_cast<size_t> (inst.result)] = true;
                        ++widenedAccessCount;
                    }
                    else if (! isLoopInvariant (inst.a))
                    {
                        return fail (YdspVectorizationReason::indirectAccess);
                    }

                    break;
                }

                case YdspIrOp::storeStateArrayF:
                {
                    if (inst.a != loop.induction)
                        return fail (YdspVectorizationReason::indirectAccess);

                    if (inst.b < 0 || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    ++widenedAccessCount;
                    break;
                }

                case YdspIrOp::loadStateArrayI:
                {
                    if (! isLoopInvariant (inst.a))
                        return fail (YdspVectorizationReason::indirectAccess);

                    break;
                }

                case YdspIrOp::loadInput:
                case YdspIrOp::loadOutput:
                {
                    if (inst.a == loop.induction)
                    {
                        if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                            return fail (YdspVectorizationReason::unsupportedWidenedType);

                        widened[j] = true;
                        isVector[static_cast<size_t> (inst.result)] = true;
                        hasStreamAccess = true;
                        ++widenedAccessCount;
                    }
                    else if (! isLoopInvariant (inst.a))
                    {
                        return fail (YdspVectorizationReason::indirectAccess);
                    }

                    break;
                }

                case YdspIrOp::storeOutput:
                {
                    if (inst.a != loop.induction)
                        return fail (YdspVectorizationReason::invariantStreamStore);

                    if (inst.b < 0 || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    hasStreamAccess = true;
                    ++widenedAccessCount;
                    break;
                }

                case YdspIrOp::storeStateArrayI:
                case YdspIrOp::storeStateF:
                case YdspIrOp::storeStateI:
                case YdspIrOp::storeParam:
                case YdspIrOp::storeParamOut:
                case YdspIrOp::storeEventFieldF:
                case YdspIrOp::storeEventFieldI:
                    return fail (YdspVectorizationReason::stateWriteInBody);

                case YdspIrOp::traceInt:
                case YdspIrOp::traceFloat:
                case YdspIrOp::traceCommit:
                    return fail (YdspVectorizationReason::traceInBody);

                case YdspIrOp::emitEvent:
                    return fail (YdspVectorizationReason::emitInBody);

                default:
                {
                    bool readsVector = false;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (isVector[static_cast<size_t> (value)])
                            readsVector = true;
                    });

                    if (! readsVector)
                        break;

                    if (! isPackedFloatOp (inst.op, fn.vectorMathEnabled, fn.packedFmaAvailable) || inst.result < 0)
                        return fail (YdspVectorizationReason::unsupportedWidenedOp);

                    if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    bool operandsAreFloat32 = true;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (fn.valueTypes[static_cast<size_t> (value)] != YdspValueType::float32Type)
                            operandsAreFloat32 = false;
                    });

                    if (! operandsAreFloat32)
                        return fail (YdspVectorizationReason::unsupportedWidenedType);

                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                    break;
                }
            }
        }

        return true;
    }

    bool checkSingleDefinitionAndLiveOut() const
    {
        for (const auto& inst : fn.blocks[static_cast<size_t> (body)].insts)
        {
            if (inst.result < 0)
                continue;

            const auto result = static_cast<size_t> (inst.result);

            if (defsInBody[result] != 1)
                return false;

            if (inst.result == loop.induction || isReductionAccumulator (inst.result))
                continue;

            if (defsElsewhere[result] != 0 || usesElsewhere[result] != 0)
                return false;
        }

        for (const auto& inst : fn.blocks[static_cast<size_t> (body)].insts)
        {
            bool available = true;

            forEachValueOperand (inst, [&] (int value)
            {
                if (defsInBody[static_cast<size_t> (value)] == 0
                    && definedInHeader[static_cast<size_t> (value)])
                    available = false;
            });

            if (! available)
                return false;
        }

        return true;
    }

    bool isLoopInvariant (int value) const
    {
        return value >= 0
            && value < numValues
            && value != loop.induction
            && defsInBody[static_cast<size_t> (value)] == 0;
    }

    //==============================================================================
    // Rewrite

    const Reduction* reductionAt (int bodyIndex) const
    {
        for (const auto& reduction : reductions)
        {
            if (reduction.addIndex == bodyIndex || reduction.moveIndex == bodyIndex)
                return &reduction;
        }

        return nullptr;
    }

    bool isReductionAccumulator (int value) const
    {
        for (const auto& reduction : reductions)
        {
            if (reduction.accumulator == value)
                return true;
        }

        return false;
    }

    int newValue (YdspValueType type, int laneCount)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (type);
        fn.valueLanes.push_back (laneCount);

        return id;
    }

    void planHoisting (const std::vector<YdspIrInst>& insts)
    {
        hoistToPreheader.assign (insts.size(), false);
        valueHoisted.assign (static_cast<size_t> (numValues), false);

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& inst = insts[j];

            if (widened[j]
                || static_cast<int> (j) == incrementIndex
                || static_cast<int> (j) == inductionMoveIndex
                || reductionAt (static_cast<int> (j)) != nullptr)
                continue;

            if (inst.result < 0)
                continue;

            // An element the body never writes - a coefficient table - is read
            // once, ahead of the loop. One it does write must be read in place.
            const bool isArrayLoad = inst.op == YdspIrOp::loadStateArrayF || inst.op == YdspIrOp::loadStateArrayI;

            if (isArrayLoad && std::any_of (insts.begin(), insts.end(), [&inst] (const YdspIrInst& other)
                {
                    return other.memIndex == inst.memIndex
                        && other.op == (inst.op == YdspIrOp::loadStateArrayF ? YdspIrOp::storeStateArrayF : YdspIrOp::storeStateArrayI);
                }))
                continue;

            bool operandsReady = true;

            forEachValueOperand (inst, [&] (int value)
            {
                if (defsInBody[static_cast<size_t> (value)] > 0 && ! valueHoisted[static_cast<size_t> (value)])
                    operandsReady = false;
            });

            if (! operandsReady)
                continue;

            hoistToPreheader[j] = true;
            valueHoisted[static_cast<size_t> (inst.result)] = true;
        }
    }

    void rewrite()
    {
        if (fn.valueLanes.size() < fn.valueTypes.size())
            fn.valueLanes.resize (fn.valueTypes.size(), 1);

        auto& preheaderInsts = fn.blocks[static_cast<size_t> (preheader)].insts;
        auto& bodyInsts = fn.blocks[static_cast<size_t> (body)].insts;

        const auto oldBody = bodyInsts;

        planHoisting (oldBody);

        const auto step = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::constI, step, -1, -1, -1, -1, 0.0, lanes });

        for (auto& reduction : reductions)
        {
            const auto zero = newValue (YdspValueType::float32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::constF, zero, -1, -1, -1, -1, 0.0 });

            reduction.vectorAcc = newValue (YdspValueType::float32Type, lanes);
            preheaderInsts.push_back ({ YdspIrOp::vsplat, reduction.vectorAcc, zero });
        }

        std::vector<YdspIrInst> newBody;
        newBody.reserve (oldBody.size() + 4);

        std::map<int, int> preheaderSplats;
        std::map<int, int> bodySplats;

        const auto splatOf = [&] (int scalar)
        {
            if (scalar < 0 || scalar >= numValues)
                return scalar;

            if (isVector[static_cast<size_t> (scalar)])
                return scalar;

            const bool definedInBody = defsInBody[static_cast<size_t> (scalar)] > 0
                                    && ! valueHoisted[static_cast<size_t> (scalar)];

            auto& cache = definedInBody ? bodySplats : preheaderSplats;

            if (const auto existing = cache.find (scalar); existing != cache.end())
                return existing->second;

            const auto splat = newValue (YdspValueType::float32Type, lanes);

            if (definedInBody)
                newBody.push_back ({ YdspIrOp::vsplat, splat, scalar });
            else
                preheaderInsts.push_back ({ YdspIrOp::vsplat, splat, scalar });

            cache[scalar] = splat;
            return splat;
        };

        const auto isFloatOperand = [this] (int value)
        {
            return value >= 0 && isFloatValueType (fn.valueTypes[static_cast<size_t> (value)]);
        };

        for (size_t j = 0; j < oldBody.size(); ++j)
        {
            auto inst = oldBody[j];

            if (hoistToPreheader[j])
            {
                preheaderInsts.push_back (inst);
                continue;
            }

            if (static_cast<int> (j) == incrementIndex)
            {
                inst.b = step;
                newBody.push_back (inst);
                continue;
            }

            if (const auto* reduction = reductionAt (static_cast<int> (j)))
            {
                if (reduction->addIndex == static_cast<int> (j))
                {
                    inst.a = reduction->vectorAcc;
                    inst.b = splatOf (reduction->addend);
                    fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;
                }
                else
                {
                    inst.result = reduction->vectorAcc;
                }

                newBody.push_back (inst);
                continue;
            }

            if (! widened[j])
            {
                newBody.push_back (inst);
                continue;
            }

            if (isRamp[j])
            {
                // The scalar conversion gives the first lane; the rest count up.
                const auto first = newValue (YdspValueType::float32Type, 1);
                newBody.push_back ({ YdspIrOp::itof, first, inst.a });
                newBody.push_back ({ YdspIrOp::vrampF, inst.result, first });
                fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;
                continue;
            }

            if (inst.op == YdspIrOp::storeStateArrayF || inst.op == YdspIrOp::storeOutput)
            {
                inst.b = splatOf (inst.b);
            }
            else if (inst.op != YdspIrOp::loadStateArrayF
                  && inst.op != YdspIrOp::loadInput
                  && inst.op != YdspIrOp::loadOutput)
            {
                if (isFloatOperand (inst.a))
                    inst.a = splatOf (inst.a);

                if (isFloatOperand (inst.b))
                    inst.b = splatOf (inst.b);

                if (isFloatOperand (inst.c))
                    inst.c = splatOf (inst.c);
            }

            if (isFloatComparison (inst.op))
            {
                if (isFloatOperand (inst.a))
                    inst.a = splatOf (inst.a);

                if (isFloatOperand (inst.b))
                    inst.b = splatOf (inst.b);
            }

            if (inst.result >= 0)
                fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;

            newBody.push_back (inst);
        }

        bodyInsts = std::move (newBody);

        tailRemap.assign (static_cast<size_t> (numValues), -1);
        int foldBlock = exit;

        if (loop.bound.kind == YdspLoopBoundKind::constant)
        {
            if (tail > 0)
                emitPeel (oldBody);
        }
        else
        {
            foldBlock = emitRuntimeEpilogue (oldBody);
        }

        std::vector<YdspIrInst> reduceOnExit;

        for (const auto& reduction : reductions)
        {
            const auto reduced = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::vreduceAddF, reduced, reduction.vectorAcc });

            const auto total = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::addF, total, reduction.accumulator, reduced });
            reduceOnExit.push_back ({ YdspIrOp::movF, reduction.accumulator, total });
        }

        auto& exitInsts = fn.blocks[static_cast<size_t> (foldBlock)].insts;
        exitInsts.insert (exitInsts.begin(), reduceOnExit.begin(), reduceOnExit.end());
    }

    int tailId (int value)
    {
        if (value < 0)
            return -1;

        if (value == loop.induction || isReductionAccumulator (value))
            return value;

        const auto v = static_cast<size_t> (value);

        if (defsInBody[v] == 0 || valueHoisted[v])
            return value;

        auto& slot = tailRemap[v];

        if (slot < 0)
            slot = newValue (fn.valueTypes[v], 1);

        return slot;
    }

    void emitPeel (const std::vector<YdspIrInst>& oldBody)
    {
        auto& preheaderInsts = fn.blocks[static_cast<size_t> (preheader)].insts;

        const auto newStart = newValue (YdspValueType::int32Type, 1);
        bool rebased = false;

        for (size_t j = 0; j < preheaderInsts.size(); ++j)
        {
            if (preheaderInsts[j].op != YdspIrOp::movI || preheaderInsts[j].result != loop.induction)
                continue;

            preheaderInsts.insert (preheaderInsts.begin() + static_cast<ptrdiff_t> (j),
                                   YdspIrInst { YdspIrOp::constI, newStart, -1, -1, -1, -1, 0.0, start + tail });
            preheaderInsts[j + 1].a = newStart;
            rebased = true;
            break;
        }

        if (! rebased)
        {
            for (size_t b = 0; b < fn.blocks.size() && ! rebased; ++b)
            {
                if (static_cast<int> (b) == body)
                    continue;

                for (auto& inst : fn.blocks[b].insts)
                {
                    if (inst.result == loop.induction && inst.op == YdspIrOp::constI)
                    {
                        inst.ivalue = start + tail;
                        rebased = true;
                        break;
                    }
                }
            }
        }

        for (int k = 0; k < tail; ++k)
        {
            tailRemap.assign (static_cast<size_t> (numValues), -1);

            const auto index = newValue (YdspValueType::int32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::constI, index, -1, -1, -1, -1, 0.0, start + k });

            for (size_t j = 0; j < oldBody.size(); ++j)
            {
                if (static_cast<int> (j) == incrementIndex || static_cast<int> (j) == inductionMoveIndex || hoistToPreheader[j])
                    continue;

                auto inst = oldBody[j];

                if (inst.result == loop.induction)
                    continue;

                if (inst.result >= 0)
                    inst.result = tailId (inst.result);

                const auto replaceOperand = [&] (int operand) -> int
                {
                    return operand == loop.induction ? index : tailId (operand);
                };

                inst.a = replaceOperand (inst.a);
                inst.b = replaceOperand (inst.b);
                inst.c = replaceOperand (inst.c);

                preheaderInsts.push_back (inst);
            }
        }
    }

    /** Emits the test for collectStreamGuards' pairs and returns its bool
        value: true when two of the streams are different buffers less than one
        vector apart, in either direction.
    */
    /** Emits the run time test for stream buffers that overlap within one
        vector, and returns the boolean it leaves behind.

        Buffers whose addresses agree modulo the vector size are either the
        same buffer or at least one vector apart, which one pass over the
        addresses settles - the case of every host that hands out aligned
        buffers, or offsets them all alike to split a block. Only when they
        disagree does the pair by pair comparison run, in a block of its own
        that the preheader branches around. Two blocks are inserted ahead of
        the header for that: the comparison, and the new preheader.
    */
    int emitStreamOverlapTest()
    {
        const auto vectorBytes = static_cast<int64_t> (lanes) * 4;

        const auto emit = [this] (int block, YdspIrOp op, YdspValueType type, int a = -1, int b = -1, int64_t literal = 0, int slot = -1)
        {
            const auto result = newValue (type, 1);
            fn.blocks[static_cast<size_t> (block)].insts.push_back ({ op, result, a, b, -1, slot, 0.0, literal });
            return result;
        };

        const auto wide = YdspValueType::int64Type;
        const auto flag = YdspValueType::boolType;
        const int entry = preheader;

        std::vector<std::pair<StreamRef, int>> addresses;

        const auto addressOf = [&] (StreamRef stream)
        {
            for (const auto& [known, value] : addresses)
                if (known == stream)
                    return value;

            addresses.emplace_back (stream, emit (entry, stream.isInput ? YdspIrOp::loadInputAddress : YdspIrOp::loadOutputAddress, wide, -1, -1, 0, stream.slot));
            return addresses.back().second;
        };

        for (const auto& [first, second] : streamGuards)
        {
            addressOf (first);
            addressOf (second);
        }

        int differing = -1;

        for (size_t i = 1; i < addresses.size(); ++i)
        {
            const auto difference = emit (entry, YdspIrOp::xorI, wide, addresses[i].second, addresses[0].second);
            differing = differing < 0 ? difference : emit (entry, YdspIrOp::orI, wide, differing, difference);
        }

        const auto zero = emit (entry, YdspIrOp::constI, wide);
        const auto residues = emit (entry, YdspIrOp::andI, wide, differing, emit (entry, YdspIrOp::constI, wide, -1, -1, vectorBytes - 1));
        const auto misaligned = emit (entry, YdspIrOp::neI, flag, residues, zero);

        const auto overlap = newValue (flag, 1);
        fn.blocks[static_cast<size_t> (entry)].insts.push_back ({ YdspIrOp::constB, overlap });

        // entry -> [pairs] -> new preheader -> header
        const int pairs = header;

        fn.blocks.insert (fn.blocks.begin() + header, 2, YdspIrBlock {});

        for (auto& block : fn.blocks)
        {
            if (block.termTarget >= pairs)
                block.termTarget += 2;

            if (block.termTarget2 >= pairs)
                block.termTarget2 += 2;
        }

        for (auto& loopEntry : fn.loops)
        {
            if (loopEntry.headerBlock >= pairs)
                loopEntry.headerBlock += 2;

            if (loopEntry.exitBlock >= pairs)
                loopEntry.exitBlock += 2;
        }

        header += 2;
        body += 2;
        exit += 2;
        preheader = header - 1;

        auto& entryBlock = fn.blocks[static_cast<size_t> (entry)];
        entryBlock.term = YdspIrTerm::branchIf;
        entryBlock.termCond = misaligned;
        entryBlock.termTarget = pairs;
        entryBlock.termTarget2 = preheader;

        const auto bias = emit (pairs, YdspIrOp::constI, wide, -1, -1, vectorBytes - 1);
        const auto window = emit (pairs, YdspIrOp::constI, wide, -1, -1, 2 * vectorBytes - 1);

        int found = -1;

        for (const auto& [first, second] : streamGuards)
        {
            // 0 < |distance| < vectorBytes, the upper bound as one unsigned compare.
            const auto distance = emit (pairs, YdspIrOp::subI, wide, addressOf (first), addressOf (second));
            const auto withinVector = emit (pairs, YdspIrOp::ltUI, flag, emit (pairs, YdspIrOp::addI, wide, distance, bias), window);
            const auto pair = emit (pairs, YdspIrOp::andB, flag, withinVector, emit (pairs, YdspIrOp::neI, flag, distance, zero));

            found = found < 0 ? pair : emit (pairs, YdspIrOp::orB, flag, found, pair);
        }

        fn.blocks[static_cast<size_t> (pairs)].insts.push_back ({ YdspIrOp::movB, overlap, found });

        return overlap;
    }

    int emitRuntimeEpilogue (const std::vector<YdspIrInst>& oldBody)
    {
        const auto loopId = loop.id;
        const auto induction = loop.induction;

        int boundValue = -1;

        for (const auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == induction)
                boundValue = inst.b;

        // Looked up at each use: the overlap test below moves the preheader.
        const auto preheaderInstructions = [this]() -> std::vector<YdspIrInst>&
        {
            return fn.blocks[static_cast<size_t> (preheader)].insts;
        };

        int endValue = boundValue;
        int spanValue = boundValue;

        if (start > 0)
        {
            auto& preheaderInsts = preheaderInstructions();

            const auto startValue = newValue (YdspValueType::int32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::constI, startValue, -1, -1, -1, -1, 0.0, start });

            const auto empty = newValue (YdspValueType::boolType, 1);
            preheaderInsts.push_back ({ YdspIrOp::ltI, empty, boundValue, startValue });
            endValue = newValue (YdspValueType::int32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::selectB, endValue, empty, startValue, boundValue });

            // Clamp before subtracting so empty ranges cannot wrap into a
            // positive vector trip count, even for a negative runtime bound.
            spanValue = newValue (YdspValueType::int32Type, 1);
            preheaderInsts.push_back ({ YdspIrOp::subI, spanValue, endValue, startValue });
        }

        const auto mask = newValue (YdspValueType::int32Type, 1);
        preheaderInstructions().push_back ({ YdspIrOp::constI, mask, -1, -1, -1, -1, 0.0, lanes - 1 });

        const auto tailValue = newValue (YdspValueType::int32Type, 1);
        preheaderInstructions().push_back ({ YdspIrOp::andI, tailValue, spanValue, mask });

        // Buffers that overlap leave the whole span to the scalar remainder:
        // the widened loop then runs no iteration at all.
        const auto scalarCount = streamGuards.empty() ? tailValue : newValue (YdspValueType::int32Type, 1);

        if (! streamGuards.empty())
        {
            const auto overlap = emitStreamOverlapTest();
            preheaderInstructions().push_back ({ YdspIrOp::selectB, scalarCount, overlap, spanValue, tailValue });
        }

        const auto vectorN = newValue (YdspValueType::int32Type, 1);
        preheaderInstructions().push_back ({ YdspIrOp::subI, vectorN, endValue, scalarCount });

        for (auto& inst : fn.blocks[static_cast<size_t> (header)].insts)
            if (inst.op == YdspIrOp::ltI && inst.a == induction)
                inst.b = vectorN;

        fn.blocks.insert (fn.blocks.begin() + exit, 2, YdspIrBlock {});

        for (auto& block : fn.blocks)
        {
            if (block.termTarget >= exit)
                block.termTarget += 2;

            if (block.termTarget2 >= exit)
                block.termTarget2 += 2;
        }

        for (auto& loopEntry : fn.loops)
        {
            if (loopEntry.headerBlock >= exit)
                loopEntry.headerBlock += 2;

            if (loopEntry.exitBlock >= exit)
                loopEntry.exitBlock += 2;
        }

        fn.loops[static_cast<size_t> (loopId)].exitBlock = exit;
        fn.blocks[static_cast<size_t> (header)].termTarget2 = exit;

        auto& tailHeaderInsts = fn.blocks[static_cast<size_t> (exit)].insts;

        const auto tailCond = newValue (YdspValueType::boolType, 1);
        tailHeaderInsts.push_back ({ YdspIrOp::ltI, tailCond, induction, boundValue });

        auto& tailHeader = fn.blocks[static_cast<size_t> (exit)];
        tailHeader.term = YdspIrTerm::branchIf;
        tailHeader.termCond = tailCond;
        tailHeader.termTarget = exit + 1;
        tailHeader.termTarget2 = exit + 2;

        auto& tailBodyInsts = fn.blocks[static_cast<size_t> (exit + 1)].insts;

        for (const auto& inst : oldBody)
        {
            auto copy = inst;

            if (copy.result >= 0)
                copy.result = tailId (copy.result);

            copy.a = tailId (copy.a);
            copy.b = tailId (copy.b);
            copy.c = tailId (copy.c);

            tailBodyInsts.push_back (copy);
        }

        auto& tailBody = fn.blocks[static_cast<size_t> (exit + 1)];
        tailBody.term = YdspIrTerm::branch;
        tailBody.termTarget = exit;

        YdspIrLoop tailLoop;
        tailLoop.id = static_cast<int> (fn.loops.size());
        tailLoop.headerBlock = exit;
        tailLoop.exitBlock = exit + 2;
        tailLoop.induction = induction;
        tailLoop.bound = { YdspLoopBoundKind::blockSize, 0 };
        fn.loops.push_back (tailLoop);

        return exit + 2;
    }

    //==============================================================================

    YdspIrFunction& fn;
    const YdspIrLoop loop;
    const int lanes;

    // When the target contracts fused multiply-add without a packed fused
    // instruction, an implicit per-sample stream loop whose body holds a
    // fusable mul->add/sub chain is kept scalar so the chain still rounds once
    // (see analyse). Constant-bound bank loops widen and stay unfused.
    const bool scalarOnlyContraction = false;

    YdspVectorizationReason rejectionReason = YdspVectorizationReason::notVectorizable;

    int header = -1;
    int body = -1;
    int exit = -1;
    int preheader = -1;
    int numValues = 0;

    int incrementIndex = -1;
    int inductionMoveIndex = -1;
    int widenedAccessCount = 0;

    int start = 0;
    int tail = 0;

    bool hasStreamAccess = false;

    std::vector<int> defsInBody, defsElsewhere, usesInBody, usesElsewhere;
    std::vector<bool> definedInHeader;
    std::vector<bool> isVector;
    std::vector<bool> widened;
    std::vector<bool> isRamp;
    std::vector<bool> isRampOffset;
    std::vector<bool> hoistToPreheader;
    std::vector<bool> valueHoisted;
    std::vector<int> tailRemap;
    std::vector<Reduction> reductions;
    std::vector<std::pair<StreamRef, StreamRef>> streamGuards;
};

} // namespace

//==============================================================================

bool YdspVectorizer::run (YdspIrFunction& fn)
{
    return run (fn, vectorWidth);
}

bool YdspVectorizer::run (YdspIrFunction& fn, int targetVectorWidth, bool scalarOnlyContraction)
{
    YdspVectorizationReport report;
    return run (fn, targetVectorWidth, report, scalarOnlyContraction);
}

bool YdspVectorizer::run (YdspIrFunction& fn, int targetVectorWidth, YdspVectorizationReport& report, bool scalarOnlyContraction)
{
    if (targetVectorWidth != 4 && targetVectorWidth != 8 && targetVectorWidth != 16)
        return false;

    bool changed = false;
    const auto loopCount = fn.loops.size();

    // If-conversion leaves empty arm blocks and straight-line joins. Collapse
    // only single-entry loop bodies; genuine branches and nested loops remain.
    for (size_t i = 0; i < loopCount; ++i)
    {
        const int first = fn.loops[i].headerBlock + 1;
        const int end = fn.loops[i].exitBlock;
        if (end <= first + 1 || end > static_cast<int> (fn.blocks.size()))
            continue;

        bool linear = true;
        for (size_t b = 0; b < fn.blocks.size(); ++b)
        {
            const auto& block = fn.blocks[b];
            if (static_cast<int> (b) >= first && static_cast<int> (b) < end)
            {
                const int next = static_cast<int> (b) + 1 == end ? fn.loops[i].headerBlock : static_cast<int> (b) + 1;
                if (! ((block.term == YdspIrTerm::fallthrough && next == static_cast<int> (b) + 1)
                       || (block.term == YdspIrTerm::branch && block.termTarget == next)))
                    linear = false;
            }
            for (const int target : { block.termTarget, block.termTarget2 })
                if (target >= first && target < end && static_cast<int> (b) != target - 1)
                    linear = false;
        }
        for (size_t j = 0; j < loopCount; ++j)
            if (j != i && fn.loops[j].headerBlock >= first && fn.loops[j].headerBlock < end)
                linear = false;
        if (! linear)
            continue;

        auto& merged = fn.blocks[static_cast<size_t> (first)];
        for (int b = first + 1; b < end; ++b)
        {
            const auto& insts = fn.blocks[static_cast<size_t> (b)].insts;
            merged.insts.insert (merged.insts.end(), insts.begin(), insts.end());
        }
        merged.term = YdspIrTerm::branch;
        merged.termCond = -1;
        merged.termTarget = fn.loops[i].headerBlock;
        merged.termTarget2 = -1;
        const int removed = end - first - 1;
        fn.blocks.erase (fn.blocks.begin() + first + 1, fn.blocks.begin() + end);
        const auto rebase = [end, removed] (int& block)
        {
            if (block >= end)
                block -= removed;
        };
        for (auto& block : fn.blocks)
        {
            rebase (block.termTarget);
            rebase (block.termTarget2);
        }
        for (auto& loop : fn.loops)
        {
            rebase (loop.headerBlock);
            rebase (loop.exitBlock);
        }
        changed = true;
    }

    fn.vectorizationResults.clear();
    fn.vectorizationResults.reserve (loopCount);
    bool widened = false;

    for (size_t i = 0; i < loopCount; ++i)
    {
        YdspVectorizationReason reason = YdspVectorizationReason::notVectorizable;

        if (LoopWidener (fn, fn.loops[i], targetVectorWidth, scalarOnlyContraction).tryWiden (reason))
        {
            changed = true;
            widened = true;
            fn.vectorizationResults.push_back ({ static_cast<int> (i), YdspVectorizationReason::widened, targetVectorWidth });
        }
        else
        {
            fn.vectorizationResults.push_back ({ static_cast<int> (i), reason, 1 });
        }
    }

    report.loops = fn.vectorizationResults;

    if (widened)
    {
        fn.vectorized = true;
        fn.vectorWidth = targetVectorWidth;
    }

    return changed;
}

} // namespace yup
