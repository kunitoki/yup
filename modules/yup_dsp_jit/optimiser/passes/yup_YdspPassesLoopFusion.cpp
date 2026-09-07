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

enum class MemAccess
{
    none,
    read,
    write
};

struct MemKey
{
    int kind = 0; // MemKind family discriminator
    int base = 0; // slot/region/endpoint index within that family
};

enum MemKind
{
    kStream = 1,
    kStateF = 2,
    kStateI = 3,
    kStateArrayF = 4,
    kStateArrayI = 5,
    kParam = 6,
    kParamOut = 7,
    kEventField = 8,
    kEmitEvent = 9
};

bool isMemoryOp (const YdspIrInst& inst, MemAccess& access, MemKey& key)
{
    const auto op = inst.op;

    switch (op)
    {
        case YdspIrOp::loadInput:
        case YdspIrOp::loadOutput:
            access = MemAccess::read;
            key = { kStream, inst.memIndex };
            return true;

        case YdspIrOp::storeOutput:
            access = MemAccess::write;
            key = { kStream, inst.memIndex };
            return true;

        case YdspIrOp::loadStateF:
            access = MemAccess::read;
            key = { kStateF, inst.a };
            return true;
        case YdspIrOp::storeStateF:
            access = MemAccess::write;
            key = { kStateF, inst.memIndex };
            return true;

        case YdspIrOp::loadStateI:
            access = MemAccess::read;
            key = { kStateI, inst.a };
            return true;
        case YdspIrOp::storeStateI:
            access = MemAccess::write;
            key = { kStateI, inst.memIndex };
            return true;

        case YdspIrOp::loadStateArrayF:
            access = MemAccess::read;
            key = { kStateArrayF, inst.memIndex };
            return true;
        case YdspIrOp::storeStateArrayF:
            access = MemAccess::write;
            key = { kStateArrayF, inst.memIndex };
            return true;

        case YdspIrOp::loadStateArrayI:
            access = MemAccess::read;
            key = { kStateArrayI, inst.memIndex };
            return true;
        case YdspIrOp::storeStateArrayI:
            access = MemAccess::write;
            key = { kStateArrayI, inst.memIndex };
            return true;

        case YdspIrOp::loadParam:
            access = MemAccess::read;
            key = { kParam, inst.a };
            return true;
        case YdspIrOp::storeParam:
            access = MemAccess::write;
            key = { kParam, inst.memIndex };
            return true;

        case YdspIrOp::loadParamOut:
            access = MemAccess::read;
            key = { kParamOut, inst.a };
            return true;
        case YdspIrOp::storeParamOut:
            access = MemAccess::write;
            key = { kParamOut, inst.memIndex };
            return true;

        case YdspIrOp::loadEventFieldF:
        case YdspIrOp::loadEventFieldI:
            access = MemAccess::read;
            key = { kEventField, inst.memIndex };
            return true;
        case YdspIrOp::storeEventFieldF:
        case YdspIrOp::storeEventFieldI:
            access = MemAccess::write;
            key = { kEventField, inst.memIndex };
            return true;

        case YdspIrOp::emitEvent:
            access = MemAccess::write;
            key = { kEmitEvent, inst.memIndex };
            return true;

        default:
            return false;
    }
}

} // namespace

//==============================================================================
// Fuses two adjacent loops with identical bounds into one, halving the
// per-sample loop overhead (header compare + back edge).

void YdspOptimizer::loopFusion (YdspIrFunction& fn)
{
    auto& blocks = fn.blocks;

    for (size_t loopIndex = 0; loopIndex + 1 < fn.loops.size();)
    {
        const auto& first = fn.loops[loopIndex];
        const auto& second = fn.loops[loopIndex + 1];

        const bool fuseable =
            first.bound.kind == second.bound.kind && first.bound.constant == second.bound.constant
            && first.unrolled == false && second.unrolled == false;

        if (! fuseable)
        {
            ++loopIndex;
            continue;
        }

        // Structural fingerprint: single body blocks, laid out back to back as
        //   h1 b1 e1 h2 b2 e2
        // where e1 (the exit of the first loop, and preheader of the second)
        // may only initialize the second induction variable from the literal 0.
        const auto h1 = first.headerBlock;
        const auto b1 = h1 + 1;
        const auto e1 = h1 + 2;
        const auto h2 = second.headerBlock;
        const auto b2 = h2 + 1;
        const auto e2 = h2 + 2;

        if (h2 != e1 + 1 || b1 >= static_cast<int> (blocks.size()) || e2 >= static_cast<int> (blocks.size()))
        {
            ++loopIndex;
            continue;
        }

        const auto& header1 = blocks[static_cast<size_t> (h1)];
        const auto& body1 = blocks[static_cast<size_t> (b1)];
        const auto& exit1 = blocks[static_cast<size_t> (e1)];
        const auto& header2 = blocks[static_cast<size_t> (h2)];
        const auto& body2 = blocks[static_cast<size_t> (b2)];

        if (header1.term != YdspIrTerm::branchIf || header1.termTarget != b1 || header1.termTarget2 != e1
            || header2.term != YdspIrTerm::branchIf || header2.termTarget != b2 || header2.termTarget2 != e2
            || body1.term != YdspIrTerm::branch || body1.termTarget != h1
            || body2.term != YdspIrTerm::branch || body2.termTarget != h2
            || exit1.term != YdspIrTerm::fallthrough)
        {
            ++loopIndex;
            continue;
        }

        // The induction update at the end of each body must be the canonical
        // `next = i + 1; i = next` pair.
        const auto trailingUpdateEnd = [&] (const YdspIrBlock& block, int induction) -> int
        {
            if (block.insts.empty())
                return -1;

            const auto moveIndex = static_cast<int> (block.insts.size()) - 1;

            if (block.insts[static_cast<size_t> (moveIndex)].op != YdspIrOp::movI
                || block.insts[static_cast<size_t> (moveIndex)].result != induction
                || moveIndex < 1
                || block.insts[static_cast<size_t> (moveIndex - 1)].op != YdspIrOp::addI
                || block.insts[static_cast<size_t> (moveIndex - 1)].result != block.insts[static_cast<size_t> (moveIndex)].a)
                return -1;

            return moveIndex - 1; // index of the addI; both trailing insts are the update
        };

        const auto firstUpdate = trailingUpdateEnd (body1, first.induction);
        const auto secondUpdate = trailingUpdateEnd (body2, second.induction);

        if (firstUpdate < 0 || secondUpdate < 0)
        {
            ++loopIndex;
            continue;
        }

        // exit1 is the second loop's preheader: LICM may have parked pure,
        // side-effect-free invariants there (constants, arithmetic). Those
        // move into the merged body verbatim - recomputing an invariant per
        // iteration is equivalent, and it keeps their definitions alive. The
        // only thing dropped is the second induction's `i2 = 0` init chain.
        const auto isPureValueOp = [] (YdspIrOp op)
        {
            switch (op)
            {
                case YdspIrOp::constF:
                case YdspIrOp::constI:
                case YdspIrOp::constB:
                case YdspIrOp::movF:
                case YdspIrOp::movI:
                case YdspIrOp::addF:
                case YdspIrOp::addI:
                case YdspIrOp::subF:
                case YdspIrOp::subI:
                case YdspIrOp::mulF:
                case YdspIrOp::mulI:
                case YdspIrOp::negF:
                case YdspIrOp::negI:
                case YdspIrOp::absF:
                case YdspIrOp::absI:
                case YdspIrOp::minF:
                case YdspIrOp::minI:
                case YdspIrOp::maxF:
                case YdspIrOp::maxI:
                    return true;

                default:
                    return false;
            }
        };

        std::vector<YdspIrInst> hoisted;
        hoisted.reserve (exit1.insts.size());
        bool exitOk = true;

        const auto definesLiteralZero = [&fn] (int value)
        {
            if (value < 0)
                return false;

            bool sawDefinition = false;
            bool isZero = false;

            for (const auto& block : fn.blocks)
            {
                for (const auto& inst : block.insts)
                {
                    if (inst.result != value)
                        continue;

                    if (sawDefinition)
                        return false; // redefined: not a single literal

                    sawDefinition = true;
                    isZero = inst.op == YdspIrOp::constI && inst.ivalue == 0;
                }
            }

            return sawDefinition && isZero;
        };

        for (const auto& inst : exit1.insts)
        {
            if (inst.op == YdspIrOp::movI && inst.result == second.induction)
            {
                if (! definesLiteralZero (inst.a))
                {
                    exitOk = false;
                    break;
                }

                continue;
            }

            if (! isPureValueOp (inst.op))
            {
                exitOk = false; // something else lives between the loops
                break;
            }

            hoisted.push_back (inst);
        }

        if (! exitOk)
        {
            ++loopIndex;
            continue;
        }

        // Memory independence: nothing written by one body may be read or
        // written by the other, otherwise interleaving the iterations changes
        // which values the second loop observes.
        struct MemoryFootprint
        {
            std::vector<MemKey> reads;
            std::vector<MemKey> writes;
            bool emitsEvents = false;
        };

        const auto memoryKeys = [&] (const YdspIrBlock& block)
        {
            MemoryFootprint footprint;

            for (const auto& inst : block.insts)
            {
                MemAccess access = MemAccess::none;
                MemKey key;

                if (! isMemoryOp (inst, access, key))
                    continue;

                if (key.kind == kEmitEvent)
                {
                    footprint.emitsEvents = true;
                    continue;
                }

                if (access == MemAccess::read)
                    footprint.reads.push_back (key);
                else if (access == MemAccess::write)
                    footprint.writes.push_back (key);
            }

            return footprint;
        };

        const auto firstKeys = memoryKeys (body1);
        const auto secondKeys = memoryKeys (body2);

        if (firstKeys.emitsEvents || secondKeys.emitsEvents)
        {
            ++loopIndex;
            continue;
        }

        // Conflict when a write of one body meets a read OR a write of the
        // other (cross-write included: the second loop would observe the
        // first's intermediate values instead of its final ones).
        const auto conflicts = [] (const auto& writes, const auto& otherReads, const auto& otherWrites)
        {
            const auto touches = [] (const MemKey& w, const std::vector<MemKey>& others)
            {
                for (const auto& o : others)
                    if (w.kind == o.kind && w.base == o.base)
                        return true;

                return false;
            };

            for (const auto& w : writes)
                if (touches (w, otherReads) || touches (w, otherWrites))
                    return true;

            return false;
        };

        if (conflicts (firstKeys.writes, secondKeys.reads, secondKeys.writes)
            || conflicts (secondKeys.writes, firstKeys.reads, firstKeys.writes))
        {
            ++loopIndex;
            continue;
        }

        // Nothing kept after fusion may reference the second loop's induction
        // once its definitions vanish (all other values are moved or dropped
        // with their blocks).
        const auto referencedElsewhere = [&] (int value)
        {
            for (int blockIndex = 0; blockIndex < static_cast<int> (blocks.size()); ++blockIndex)
            {
                if (blockIndex == e1 || blockIndex == h2 || blockIndex == b2)
                    continue;

                for (const auto& inst : blocks[static_cast<size_t> (blockIndex)].insts)
                    if (inst.a == value || inst.b == value || inst.c == value)
                        return true;
            }

            return false;
        };

        if (referencedElsewhere (second.induction))
        {
            ++loopIndex;
            continue;
        }

        // ---- fuse ----
        // 1. Merge the second body into the first, before the first body's own
        //    induction update, rebinding the second induction to the first.
        std::vector<YdspIrInst> merged;
        merged.reserve (hoisted.size() + static_cast<size_t> (firstUpdate) + body2.insts.size() + 2);

        merged.insert (merged.end(), hoisted.begin(), hoisted.end());
        merged.insert (merged.end(), body1.insts.begin(), body1.insts.begin() + static_cast<size_t> (firstUpdate));

        for (size_t i = 0; i < static_cast<size_t> (secondUpdate); ++i)
        {
            auto inst = body2.insts[i];

            if (inst.a == second.induction)
                inst.a = first.induction;

            if (inst.b == second.induction)
                inst.b = first.induction;

            if (inst.c == second.induction)
                inst.c = first.induction;

            merged.push_back (inst);
        }

        merged.insert (merged.end(),
                       body1.insts.begin() + static_cast<size_t> (firstUpdate),
                       body1.insts.end());

        // 2. Drop the blocks e1 (dead init), h2 and b2 (folded into b1), and
        //    retarget the first loop's exit onto e2. termTarget2 is set to the
        //    *pre-remap* e2 here; the generic remap below folds it to e2 - 3,
        //    which is exactly where e2 lands once the three blocks are gone.
        const auto removedStart = e1;
        const auto newExit = e2 - 3; // three blocks removed before it

        auto remap = [removedStart] (int value) -> int
        {
            if (value < 0 || value < removedStart)
                return value;

            return value - 3;
        };

        std::vector<YdspIrBlock> newBlocks;
        newBlocks.reserve (blocks.size() - 3);

        for (int blockIndex = 0; blockIndex < static_cast<int> (blocks.size()); ++blockIndex)
        {
            if (blockIndex >= removedStart && blockIndex < removedStart + 3)
                continue;

            auto block = blocks[static_cast<size_t> (blockIndex)];

            if (blockIndex == b1)
                block.insts = std::move (merged);

            if (blockIndex == h1)
                block.termTarget2 = e2;

            block.termTarget = remap (block.termTarget);
            block.termTarget2 = remap (block.termTarget2);
            newBlocks.push_back (std::move (block));
        }

        fn.blocks = std::move (newBlocks);

        // 3. Fix up the loop records: erase the second loop and renumber.
        fn.loops.erase (fn.loops.begin() + static_cast<ptrdiff_t> (loopIndex + 1));

        for (auto& loop : fn.loops)
        {
            loop.headerBlock = remap (loop.headerBlock);
            loop.exitBlock = remap (loop.exitBlock);
        }

        for (size_t i = 0; i < fn.loops.size(); ++i)
            fn.loops[i].id = static_cast<int> (i);

        // First loop's exit is now e2's new location; its header already jumps there.
        fn.loops[loopIndex].exitBlock = newExit;

        // Keep `loopIndex` so a chain of three fusable loops merges in one pass.
    }
}

} // namespace yup
