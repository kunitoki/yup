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

/** Calls `callback (valueId)` for every operand of `inst` that really is a
    value id.

    Most opcodes carry value ids in a/b/c, but a handful use `a` for a
    param/state *slot* index instead. Treating one of those as a value id would
    make the analysis reject loops at random, depending on whether a slot number
    happened to collide with a live value id.
*/
template <typename Callback>
void forEachValueOperand (const YdspIrInst& inst, Callback&& callback)
{
    switch (inst.op)
    {
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
            return; // `a` is a slot index

        default:
            break;
    }

    if (inst.a >= 0)
        callback (inst.a);

    if (inst.b >= 0)
        callback (inst.b);

    if (inst.c >= 0)
        callback (inst.c);
}

/** The float operations both native backends lower to a packed form.

    Deliberately excludes the rounding family (`roundps` needs SSE4.1) and
    `modF` (which lowers through it), the transcendentals (a libm call per lane
    would need extract/insert) and every comparison and `select` (no lane mask
    yet). A loop using one of those on a widened value is left scalar.
*/
bool isPackedFloatOp (YdspIrOp op) noexcept
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
        case YdspIrOp::clampF:
        case YdspIrOp::lerpF:
        case YdspIrOp::movF:
            return true;

        default:
            return false;
    }
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

//==============================================================================
/** Widens one loop, or leaves it exactly as it was.

    Analysis never mutates the function: every legality test runs first and only
    a fully-proven loop reaches rewrite(), so a rejected loop costs nothing but
    the analysis.
*/
class LoopWidener
{
public:
    LoopWidener (YdspIrFunction& functionToWiden, const YdspIrLoop& loopToWiden, int width)
        : fn (functionToWiden)
        , loop (loopToWiden)
        , lanes (width)
    {
    }

    bool tryWiden()
    {
        if (! analyse())
            return false;

        rewrite();
        return true;
    }

private:
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
        // A constant trip count that is a whole number of vectors: no scalar
        // epilogue, and therefore no block to insert (see the class comment on
        // YdspVectorizer).
        if (loop.bound.kind != YdspLoopBoundKind::constant)
            return false;

        if (loop.bound.constant <= 0 || (loop.bound.constant % lanes) != 0)
            return false;

        header = loop.headerBlock;
        body = header + 1;
        exit = loop.exitBlock;
        preheader = header - 1;

        if (preheader < 0 || body < 0 || exit != body + 1)
            return false; // a multi-block (or nested) body

        if (static_cast<size_t> (exit) >= fn.blocks.size())
            return false;

        const auto& bodyBlock = fn.blocks[static_cast<size_t> (body)];

        if (bodyBlock.term != YdspIrTerm::branch || bodyBlock.termTarget != header)
            return false;

        numValues = static_cast<int> (fn.valueTypes.size());

        if (numValues <= 0)
            return false;

        defsInBody.assign (static_cast<size_t> (numValues), 0);
        defsElsewhere.assign (static_cast<size_t> (numValues), 0);
        usesInBody.assign (static_cast<size_t> (numValues), 0);
        usesElsewhere.assign (static_cast<size_t> (numValues), 0);
        definedInHeader.assign (static_cast<size_t> (numValues), false);

        if (! countDefsAndUses())
            return false;

        return findInductionUpdate()
            && checkInductionUses()
            && findReductions()
            && classifyBody()
            && checkSingleDefinitionAndLiveOut()
            && widenedAccessCount > 0; // nothing to gain otherwise
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
                        return false; // value ids must be covered by valueTypes

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

    /** Matches the `next = i + 1; i = next` pair the lowering appends to every
        loop body, which the rewrite re-steps to the vector width. */
    bool findInductionUpdate()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        if (insts.size() < 2)
            return false;

        const auto last = static_cast<int> (insts.size()) - 1;
        const auto& move = insts[static_cast<size_t> (last)];

        // The update has to be the last thing in the body, so every use of the
        // induction variable in the body sees the same iteration's value. The
        // AArch64 lowering relies on this to reuse one scaled index register
        // across every widened access in the block.
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

    /** True when `value` is written exactly once in the whole function, by a
        `constI` carrying `expected`. */
    bool isConstantInt (int value, int64_t expected) const
    {
        if (value < 0 || value >= numValues)
            return false;

        if (defsInBody[static_cast<size_t> (value)] + defsElsewhere[static_cast<size_t> (value)] != 1)
            return false;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.result == value)
                    return inst.op == YdspIrOp::constI && inst.ivalue == expected;

        return false;
    }

    /** The induction variable may only be read as a state-array index and by
        its own increment - anything else (`float (i)`, `buf[i * 2]`, a use
        after the loop) would see a variable stepping by the vector width. */
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
                    continue; // the trip-count compare, still correct at any step

                if (static_cast<int> (b) != body)
                    return false;

                if (static_cast<int> (j) == incrementIndex)
                    continue;

                const bool indexOnly = isStateArrayAccess (inst.op)
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

    /** Finds the `acc = acc + x` accumulations, the one loop-carried dependence
        this pass accepts. Every other value written in the body has to be
        body-local (checkSingleDefinitionAndLiveOut). */
    bool findReductions()
    {
        const auto& insts = fn.blocks[static_cast<size_t> (body)].insts;

        for (size_t j = 0; j < insts.size(); ++j)
        {
            const auto& move = insts[j];

            if (move.op != YdspIrOp::movF || move.result < 0)
                continue;

            const auto accumulator = static_cast<size_t> (move.result);

            // A body-local move is ordinary work, not an accumulation.
            if (defsElsewhere[accumulator] == 0 && usesElsewhere[accumulator] == 0)
                continue;

            // It escapes the body, so it must be initialised before the loop and
            // match the accumulation shape exactly.
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

            // The partial sum must be private to the accumulation, or replacing
            // it with a vector would change something else's value.
            if (defsInBody[sum] != 1 || defsElsewhere[sum] != 0
                || usesInBody[sum] != 1 || usesElsewhere[sum] != 0)
                return false;

            // The accumulator itself must be read only by this add.
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

    /** Decides, in program order, which values the loop widens - and rejects
        the loop as soon as something would have to be scalarised. */
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
                // Both halves of the accumulation are rewritten wholesale, so
                // the generic operand rules do not apply to them.
                if (reduction->addIndex == static_cast<int> (j))
                {
                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                }

                continue;
            }

            switch (inst.op)
            {
                case YdspIrOp::loadStateArrayF:
                {
                    if (inst.a == loop.induction)
                    {
                        if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                            return false; // f64 lanes are not supported

                        widened[j] = true;
                        isVector[static_cast<size_t> (inst.result)] = true;
                        ++widenedAccessCount;
                    }
                    else if (! isLoopInvariant (inst.a))
                    {
                        return false; // an indirect index (buf[wp]) is not unit-stride
                    }

                    break;
                }

                case YdspIrOp::storeStateArrayF:
                {
                    // A store through anything but the induction variable writes
                    // the same element every iteration, so running the loop a
                    // quarter as often would change which write lands last.
                    if (inst.a != loop.induction)
                        return false;

                    if (inst.b < 0 || fn.valueTypes[static_cast<size_t> (inst.b)] != YdspValueType::float32Type)
                        return false;

                    widened[j] = true;
                    ++widenedAccessCount;
                    break;
                }

                case YdspIrOp::loadStateArrayI:
                {
                    // Integer lanes are not implemented; a loop-invariant index
                    // is just an ordinary scalar load.
                    if (! isLoopInvariant (inst.a))
                        return false;

                    break;
                }

                case YdspIrOp::loadInput:
                case YdspIrOp::loadOutput:
                {
                    // A stream read at the *sample* index is invariant across
                    // this loop, and `in` used inside a loop body lands here:
                    // loop-invariant code motion never hoists a load, so the
                    // read stays in the body even though its value cannot
                    // change. Running it once per vector instead of once per
                    // element is safe because nothing in a widened body writes
                    // a stream - storeOutput is rejected below.
                    //
                    // Reading at the induction variable would need a packed
                    // stream access, which this pass does not do.
                    if (! isLoopInvariant (inst.a))
                        return false;

                    break;
                }

                // Side effects whose count or order the widening would change.
                case YdspIrOp::storeStateArrayI:
                case YdspIrOp::storeStateF:
                case YdspIrOp::storeStateI:
                case YdspIrOp::storeParam:
                case YdspIrOp::storeParamOut:
                case YdspIrOp::storeOutput:
                case YdspIrOp::storeEventFieldF:
                case YdspIrOp::storeEventFieldI:
                case YdspIrOp::emitEvent:
                    return false;

                default:
                {
                    bool readsVector = false;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (isVector[static_cast<size_t> (value)])
                            readsVector = true;
                    });

                    if (! readsVector)
                        break; // ordinary scalar work, left alone

                    if (! isPackedFloatOp (inst.op) || inst.result < 0)
                        return false;

                    // Only float32 has a four-lane form here: a float64 operand
                    // would need two registers, and an int operand a different
                    // instruction family.
                    if (fn.valueTypes[static_cast<size_t> (inst.result)] != YdspValueType::float32Type)
                        return false;

                    bool operandsAreFloat32 = true;

                    forEachValueOperand (inst, [&] (int value)
                    {
                        if (fn.valueTypes[static_cast<size_t> (value)] != YdspValueType::float32Type)
                            operandsAreFloat32 = false;
                    });

                    if (! operandsAreFloat32)
                        return false;

                    widened[j] = true;
                    isVector[static_cast<size_t> (inst.result)] = true;
                    break;
                }
            }
        }

        return true;
    }

    /** Every value the body writes must be private to the body (the induction
        variable and the reduction accumulators excepted), or widening it - or
        simply running its definition a quarter as often - would be visible
        outside the loop. This is what keeps promoted `state` scalars, `'`, `@`
        and `smooth` slots out. */
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

        // A scalar the body reads from outside is broadcast in the preheader, so
        // the preheader has to see the same value the body does. It always
        // does: preheader -> header -> body is the only way in, and the loop
        // back-edge goes through the header too, so the header is the only block
        // that can redefine anything in between. A value the body reads may
        // well be redefined *after* the loop - the enclosing sample loop's own
        // induction variable, which every `loadInput` in the body is indexed
        // by, is exactly that - and that is not a reason to bail.
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
            if (reduction.addIndex == bodyIndex || reduction.moveIndex == bodyIndex)
                return &reduction;

        return nullptr;
    }

    bool isReductionAccumulator (int value) const
    {
        for (const auto& reduction : reductions)
            if (reduction.accumulator == value)
                return true;

        return false;
    }

    int newValue (YdspValueType type, int laneCount)
    {
        const auto id = static_cast<int> (fn.valueTypes.size());

        fn.valueTypes.push_back (type);
        fn.valueLanes.push_back (laneCount);

        return id;
    }

    /** Marks the body's loop-invariant scalar prelude for the preheader.

        `in` read inside a loop lands in the body as a `loadInput` and stays
        there: loop-invariant code motion never hoists a load, because in
        general the body could write that memory. In a *widened* body it
        provably cannot - `storeOutput` disqualifies the loop outright - and the
        index is already known to be loop-invariant, so the load and everything
        pure that depends only on it can run once per loop entry instead of once
        per element. In the modal-bank shape that is a load, a multiply, and the
        broadcast they feed, per vector iteration.

        LICM is the natural home for this, but its availability rule ("every
        definition of an operand dominates the preheader") cannot express it:
        the index is the enclosing sample loop's induction variable, which is
        also defined in a block *after* this loop. Loosening that rule is what
        reintroduced the frozen-state hang once already, whereas here the
        preconditions are all established by the analysis above.

        A state-array load is deliberately not hoistable: `z[j]` at a
        loop-invariant `j` is the same element as the widened `z[i]` store on
        whichever iteration `i == j`.

        Only reached for a loop with a constant, non-zero trip count, so
        nothing is speculated into a loop that might not run.
    */
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

            // Every side-effecting opcode in this IR writes through `memIndex`
            // and produces no result, so requiring a result is what keeps the
            // stores out.
            if (inst.result < 0
                || inst.op == YdspIrOp::loadStateArrayF
                || inst.op == YdspIrOp::loadStateArrayI)
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

        // The loop now advances a whole vector per iteration.
        const auto step = newValue (YdspValueType::int32Type, 1);
        preheaderInsts.push_back ({ YdspIrOp::constI, step, -1, -1, -1, -1, 0.0, lanes });

        // One vector accumulator per reduction, zeroed per entry to the loop.
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

        // A scalar feeding a widened operation is broadcast once: in the
        // preheader when it is loop-invariant (so it stays out of the loop), in
        // the body otherwise.
        const auto splatOf = [&] (int scalar)
        {
            if (isVector[static_cast<size_t> (scalar)])
                return scalar;

            // A hoisted value is defined in the preheader now, so its broadcast
            // belongs there too - which is the point of hoisting it.
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
                    // acc + x  ->  vectorAcc + broadcast/vector x
                    inst.a = reduction->vectorAcc;
                    inst.b = splatOf (reduction->addend);
                    fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;
                }
                else
                {
                    // The accumulator itself stays scalar and keeps its
                    // pre-loop value; the vector one is what the body updates.
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

            if (inst.op == YdspIrOp::storeStateArrayF)
            {
                inst.b = splatOf (inst.b);
            }
            else if (inst.op != YdspIrOp::loadStateArrayF) // its index stays scalar
            {
                if (isFloatOperand (inst.a))
                    inst.a = splatOf (inst.a);

                if (isFloatOperand (inst.b))
                    inst.b = splatOf (inst.b);

                if (isFloatOperand (inst.c))
                    inst.c = splatOf (inst.c);
            }

            if (inst.result >= 0)
                fn.valueLanes[static_cast<size_t> (inst.result)] = lanes;

            newBody.push_back (inst);
        }

        bodyInsts = std::move (newBody);

        // Fold the lanes back into the scalar accumulator, once, on the way out.
        std::vector<YdspIrInst> reduceOnExit;

        for (const auto& reduction : reductions)
        {
            const auto reduced = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::vreduceAddF, reduced, reduction.vectorAcc });

            const auto total = newValue (YdspValueType::float32Type, 1);
            reduceOnExit.push_back ({ YdspIrOp::addF, total, reduction.accumulator, reduced });
            reduceOnExit.push_back ({ YdspIrOp::movF, reduction.accumulator, total });
        }

        auto& exitInsts = fn.blocks[static_cast<size_t> (exit)].insts;
        exitInsts.insert (exitInsts.begin(), reduceOnExit.begin(), reduceOnExit.end());
    }

    //==============================================================================

    YdspIrFunction& fn;
    const YdspIrLoop& loop;
    const int lanes;

    int header = -1;
    int body = -1;
    int exit = -1;
    int preheader = -1;
    int numValues = 0;

    int incrementIndex = -1;
    int inductionMoveIndex = -1;
    int widenedAccessCount = 0;

    std::vector<int> defsInBody, defsElsewhere, usesInBody, usesElsewhere;
    std::vector<bool> definedInHeader;
    std::vector<bool> isVector;
    std::vector<bool> widened;
    std::vector<bool> hoistToPreheader; // per body instruction
    std::vector<bool> valueHoisted;     // per value id
    std::vector<Reduction> reductions;
};

} // namespace

//==============================================================================

bool YdspVectorizer::run (YdspIrFunction& fn)
{
    bool changed = false;

    for (const auto& loop : fn.loops)
        changed |= LoopWidener (fn, loop, vectorWidth).tryWiden();

    if (changed)
    {
        fn.vectorized = true;
        fn.vectorWidth = vectorWidth;
    }

    return changed;
}

} // namespace yup
