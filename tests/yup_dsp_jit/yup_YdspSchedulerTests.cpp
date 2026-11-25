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

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace yup;

//==============================================================================

class YdspSchedulerTests : public ::testing::Test
{
protected:
    struct SimState
    {
        std::vector<float> regs;
        std::vector<int> intRegs;
        std::vector<float> stateF;
        std::vector<int> stateI;
        std::vector<float> stateArrayF;
        std::vector<float> outputStream;
        std::vector<float> inputStream;
    };

    static int addValue (YdspIrFunction& fn, YdspValueType type, int lanes = 1)
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);
        if (! fn.valueLanes.empty() || lanes > 1)
        {
            fn.valueLanes.resize (fn.valueTypes.size() - 1, 1);
            fn.valueLanes.push_back (lanes);
        }
        return id;
    }

    static int findInstIndex (const YdspIrBlock& block, YdspIrOp op, int result = -1, int memIndex = -1)
    {
        for (size_t i = 0; i < block.insts.size(); ++i)
        {
            const auto& inst = block.insts[i];
            if (inst.op != op)
                continue;
            if (result >= 0 && inst.result != result)
                continue;
            if (memIndex >= 0 && inst.memIndex != memIndex)
                continue;
            return static_cast<int> (i);
        }
        return -1;
    }

    static int computePeakLiveFp (const YdspIrFunction& fn, const YdspIrBlock& block)
    {
        const size_t n = block.insts.size();
        const auto types = resolveValueTypes (fn);
        const size_t numValues = types.size();

        // Track last use of each definition inside the block.
        std::vector<int> lastUseOfInst (n, -1);
        std::vector<int> lastDef (numValues, -1);

        for (size_t i = 0; i < n; ++i)
        {
            const auto& inst = block.insts[i];
            const int operands[] { inst.a, inst.b, inst.c };

            for (int opIdx = 0; opIdx < 3; ++opIdx)
            {
                if (opIdx == 0 && (inst.op == YdspIrOp::loadParam || inst.op == YdspIrOp::loadParamOut
                                   || inst.op == YdspIrOp::loadStateF || inst.op == YdspIrOp::loadStateI))
                    continue;

                const int val = operands[opIdx];
                if (val >= 0 && static_cast<size_t> (val) < numValues && lastDef[static_cast<size_t> (val)] >= 0)
                    lastUseOfInst[static_cast<size_t> (lastDef[static_cast<size_t> (val)])] = static_cast<int> (i);
            }

            if (inst.result >= 0 && static_cast<size_t> (inst.result) < numValues)
                lastDef[static_cast<size_t> (inst.result)] = static_cast<int> (i);
        }

        if (block.term == YdspIrTerm::branchIf && block.termCond >= 0
            && static_cast<size_t> (block.termCond) < numValues
            && lastDef[static_cast<size_t> (block.termCond)] >= 0)
            lastUseOfInst[static_cast<size_t> (lastDef[static_cast<size_t> (block.termCond)])] = static_cast<int> (n);

        int live = 0;
        int peak = 0;

        for (size_t i = 0; i < n; ++i)
        {
            const auto& inst = block.insts[i];

            for (size_t j = 0; j < i; ++j)
            {
                if (lastUseOfInst[j] == static_cast<int> (i))
                {
                    const int res = block.insts[j].result;
                    if (res >= 0 && (fn.laneCountOf (res) > 1 || isFloatValueType (types[static_cast<size_t> (res)])))
                        --live;
                }
            }

            if (inst.result >= 0 && lastUseOfInst[i] > static_cast<int> (i))
            {
                if (fn.laneCountOf (inst.result) > 1 || isFloatValueType (types[static_cast<size_t> (inst.result)]))
                    ++live;
            }

            peak = std::max (peak, live);
        }

        return peak;
    }

    static SimState simulateBlock (const YdspIrBlock& block, SimState state)
    {
        for (const auto& inst : block.insts)
        {
            switch (inst.op)
            {
                case YdspIrOp::constF:
                    state.regs[static_cast<size_t> (inst.result)] = static_cast<float> (inst.fvalue);
                    break;
                case YdspIrOp::constI:
                    state.intRegs[static_cast<size_t> (inst.result)] = static_cast<int> (inst.ivalue);
                    break;
                case YdspIrOp::loadStateF:
                    state.regs[static_cast<size_t> (inst.result)] = state.stateF[static_cast<size_t> (inst.a)];
                    break;
                case YdspIrOp::storeStateF:
                    state.stateF[static_cast<size_t> (inst.memIndex)] = state.regs[static_cast<size_t> (inst.a)];
                    break;
                case YdspIrOp::loadStateArrayF:
                {
                    const int idx = state.intRegs[static_cast<size_t> (inst.a)];
                    state.regs[static_cast<size_t> (inst.result)] = state.stateArrayF[static_cast<size_t> (inst.memIndex + idx)];
                    break;
                }
                case YdspIrOp::storeStateArrayF:
                {
                    const int idx = state.intRegs[static_cast<size_t> (inst.a)];
                    state.stateArrayF[static_cast<size_t> (inst.memIndex + idx)] = state.regs[static_cast<size_t> (inst.b)];
                    break;
                }
                case YdspIrOp::loadInput:
                {
                    const int idx = state.intRegs[static_cast<size_t> (inst.a)];
                    state.regs[static_cast<size_t> (inst.result)] = state.inputStream[static_cast<size_t> (inst.memIndex * 16 + idx)];
                    break;
                }
                case YdspIrOp::storeOutput:
                {
                    const int idx = state.intRegs[static_cast<size_t> (inst.a)];
                    state.outputStream[static_cast<size_t> (inst.memIndex * 16 + idx)] = state.regs[static_cast<size_t> (inst.b)];
                    break;
                }
                case YdspIrOp::addF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)] + state.regs[static_cast<size_t> (inst.b)];
                    break;
                case YdspIrOp::subF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)] - state.regs[static_cast<size_t> (inst.b)];
                    break;
                case YdspIrOp::mulF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)] * state.regs[static_cast<size_t> (inst.b)];
                    break;
                case YdspIrOp::divF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)] / state.regs[static_cast<size_t> (inst.b)];
                    break;
                case YdspIrOp::fmaF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)] * state.regs[static_cast<size_t> (inst.b)] + state.regs[static_cast<size_t> (inst.c)];
                    break;
                case YdspIrOp::movF:
                    state.regs[static_cast<size_t> (inst.result)] = state.regs[static_cast<size_t> (inst.a)];
                    break;
                default:
                    break;
            }
        }
        return state;
    }
};

//==============================================================================

TEST_F (YdspSchedulerTests, PreservesWarAndWawDependenciesInNonSsaBlock)
{
    YdspIrFunction fn;
    const int idx = addValue (fn, YdspValueType::int32Type);
    const int cScale = addValue (fn, YdspValueType::float32Type);
    const int v0 = addValue (fn, YdspValueType::float32Type);
    const int v1 = addValue (fn, YdspValueType::float32Type);
    const int v2 = addValue (fn, YdspValueType::float32Type);
    const int cond = addValue (fn, YdspValueType::boolType);
    const int zeroF = addValue (fn, YdspValueType::float32Type);

    YdspIrBlock block;
    block.insts.push_back ({ YdspIrOp::constI, idx, -1, -1, -1, -1, 0.0, 0 });
    block.insts.push_back ({ YdspIrOp::constF, cScale, -1, -1, -1, -1, 2.0 });
    block.insts.push_back ({ YdspIrOp::constF, zeroF, -1, -1, -1, -1, 0.0 });
    // First def of v0 -> read by v1 -> WAR into second def of v0 -> read by v2 -> WAR into third def of v0
    block.insts.push_back ({ YdspIrOp::loadInput, v0, idx, -1, -1, 0 });
    block.insts.push_back ({ YdspIrOp::addF, v1, v0, cScale });
    block.insts.push_back ({ YdspIrOp::constF, v0, -1, -1, -1, -1, 5.0 });
    block.insts.push_back ({ YdspIrOp::mulF, v2, v0, cScale });
    block.insts.push_back ({ YdspIrOp::constF, v0, -1, -1, -1, -1, 9.0 });
    block.insts.push_back ({ YdspIrOp::gtF, cond, v0, zeroF });
    block.insts.push_back ({ YdspIrOp::storeOutput, -1, idx, v1, -1, 0 });
    block.insts.push_back ({ YdspIrOp::storeOutput, -1, idx, v2, -1, 1 });
    block.insts.push_back ({ YdspIrOp::storeOutput, -1, idx, v0, -1, 2 });
    block.term = YdspIrTerm::branchIf;
    block.termCond = cond;
    block.termTarget = 0;
    block.termTarget2 = 0;

    fn.blocks.push_back (block);
    const auto originalBlock = fn.blocks[0];

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.scheduleInstructions (fn);

    const auto& scheduled = fn.blocks[0];

    const int def0 = findInstIndex (scheduled, YdspIrOp::loadInput, v0);
    const int read1 = findInstIndex (scheduled, YdspIrOp::addF, v1);
    const int def1 = findInstIndex (scheduled, YdspIrOp::constF, v0);
    const int read2 = findInstIndex (scheduled, YdspIrOp::mulF, v2);
    const int store0 = findInstIndex (scheduled, YdspIrOp::storeOutput, -1, 0);
    const int store1 = findInstIndex (scheduled, YdspIrOp::storeOutput, -1, 1);
    const int store2 = findInstIndex (scheduled, YdspIrOp::storeOutput, -1, 2);
    const int cmpIdx = findInstIndex (scheduled, YdspIrOp::gtF, cond);

    int def2 = -1;
    for (int i = def1 + 1; i < static_cast<int> (scheduled.insts.size()); ++i)
        if (scheduled.insts[static_cast<size_t> (i)].op == YdspIrOp::constF
            && scheduled.insts[static_cast<size_t> (i)].result == v0)
            def2 = i;

    ASSERT_GE (def0, 0);
    ASSERT_GE (read1, 0);
    ASSERT_GE (def1, 0);
    ASSERT_GE (read2, 0);
    ASSERT_GE (def2, 0);
    ASSERT_GE (cmpIdx, 0);

    EXPECT_LT (def0, read1);
    EXPECT_LT (read1, def1);
    EXPECT_LT (def1, read2);
    EXPECT_LT (read2, def2);
    EXPECT_LT (def2, cmpIdx);
    EXPECT_LT (def2, store2);
    EXPECT_LT (read1, store0);
    EXPECT_LT (read2, store1);

    SimState initial;
    initial.regs.assign (fn.valueTypes.size(), 0.0f);
    initial.intRegs.assign (fn.valueTypes.size(), 0);
    initial.inputStream.assign (64, 3.0f);
    initial.outputStream.assign (64, 0.0f);

    const auto expected = simulateBlock (originalBlock, initial);
    const auto actual = simulateBlock (scheduled, initial);

    EXPECT_FLOAT_EQ (expected.outputStream[0], actual.outputStream[0]);
    EXPECT_FLOAT_EQ (expected.outputStream[16], actual.outputStream[16]);
    EXPECT_FLOAT_EQ (expected.outputStream[32], actual.outputStream[32]);
}

//==============================================================================

TEST_F (YdspSchedulerTests, PreservesAliasingAndAllowsDisjointStateArrayReordering)
{
    YdspIrFunction fn;
    const int dynIdx = addValue (fn, YdspValueType::int32Type);
    const int c0 = addValue (fn, YdspValueType::int32Type);
    const int c1 = addValue (fn, YdspValueType::int32Type);
    const int valA = addValue (fn, YdspValueType::float32Type);
    const int valB = addValue (fn, YdspValueType::float32Type);
    const int loadedSame = addValue (fn, YdspValueType::float32Type);
    const int loadedDisjoint = addValue (fn, YdspValueType::float32Type);
    const int loadedOtherArr = addValue (fn, YdspValueType::float32Type);
    const int divTmp = addValue (fn, YdspValueType::float32Type);
    const int sumTmp = addValue (fn, YdspValueType::float32Type);

    YdspIrBlock block;
    block.insts.push_back ({ YdspIrOp::loadStateI, dynIdx, 0 });
    block.insts.push_back ({ YdspIrOp::constI, c0, -1, -1, -1, -1, 0.0, 0 });
    block.insts.push_back ({ YdspIrOp::constI, c1, -1, -1, -1, -1, 0.0, 1 });
    block.insts.push_back ({ YdspIrOp::constF, valA, -1, -1, -1, -1, 1.5 });
    block.insts.push_back ({ YdspIrOp::constF, valB, -1, -1, -1, -1, 2.5 });
    // Store then load on array 0 at constant index 0 (aliasing -> must stay ordered)
    block.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, c0, valA, -1, 0 });
    block.insts.push_back ({ YdspIrOp::loadStateArrayF, loadedSame, c0, -1, -1, 0 });
    // Dynamic store on array 0 (must follow earlier load on array 0, and precede later store)
    block.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, dynIdx, valB, -1, 0 });
    // High-latency disjoint load on array 0 at index 1 and distinct array 8 at dynIdx
    block.insts.push_back ({ YdspIrOp::loadStateArrayF, loadedDisjoint, c1, -1, -1, 0 });
    // Distinct array 8 load feeds a longer critical path (mul -> div) and is free
    // to schedule early (before storeArr0Dyn) while still staying before array 8's own store.
    const int mulTmp = addValue (fn, YdspValueType::float32Type);
    block.insts.push_back ({ YdspIrOp::loadStateArrayF, loadedOtherArr, dynIdx, -1, -1, 8 });
    block.insts.push_back ({ YdspIrOp::mulF, mulTmp, loadedOtherArr, valA });
    block.insts.push_back ({ YdspIrOp::divF, divTmp, loadedDisjoint, mulTmp });
    block.insts.push_back ({ YdspIrOp::addF, sumTmp, divTmp, loadedSame });
    block.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, c1, sumTmp, -1, 8 });

    fn.blocks.push_back (block);

    // Note: dynIdx could alias c0 or c1, so loadedDisjoint (index 1 on array 0)
    // must NOT move before dynIdx store on array 0; whereas loadedOtherArr
    // (distinct array memIndex=8) CAN move before array 0 stores!
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    const bool changed = optimizer.scheduleInstructions (fn);

    EXPECT_TRUE (changed);

    const auto& scheduled = fn.blocks[0];
    const int storeArr0C0 = findInstIndex (scheduled, YdspIrOp::storeStateArrayF, -1, 0);
    const int loadArr0C0 = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, loadedSame, 0);
    int storeArr0Dyn = -1;
    for (size_t i = 0; i < scheduled.insts.size(); ++i)
        if (scheduled.insts[i].op == YdspIrOp::storeStateArrayF && scheduled.insts[i].memIndex == 0
            && scheduled.insts[i].a == dynIdx)
            storeArr0Dyn = static_cast<int> (i);
    const int loadArr0C1 = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, loadedDisjoint, 0);
    const int loadArr8Dyn = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, loadedOtherArr, 8);
    const int storeArr8C1 = findInstIndex (scheduled, YdspIrOp::storeStateArrayF, -1, 8);

    ASSERT_GE (storeArr0C0, 0);
    ASSERT_GE (loadArr0C0, 0);
    ASSERT_GE (storeArr0Dyn, 0);
    ASSERT_GE (loadArr0C1, 0);
    ASSERT_GE (loadArr8Dyn, 0);
    ASSERT_GE (storeArr8C1, 0);

    // Same-index and potentially-aliasing dynamic index on array 0 stay ordered.
    EXPECT_LT (storeArr0C0, loadArr0C0);
    EXPECT_LT (loadArr0C0, storeArr0Dyn);
    EXPECT_LT (storeArr0Dyn, loadArr0C1);

    // Distinct array 8 load is free to schedule early (before storeArr0Dyn)
    // while still staying before array 8's own store.
    EXPECT_LT (loadArr8Dyn, storeArr0Dyn);
    EXPECT_LT (loadArr8Dyn, storeArr8C1);

    // Separate test: purely constant disjoint indices on the SAME array reorder freely.
    YdspIrFunction fnDisjoint;
    const int idx0 = addValue (fnDisjoint, YdspValueType::int32Type);
    const int idx1 = addValue (fnDisjoint, YdspValueType::int32Type);
    const int vWrite = addValue (fnDisjoint, YdspValueType::float32Type);
    const int vLoad = addValue (fnDisjoint, YdspValueType::float32Type);
    const int vDiv = addValue (fnDisjoint, YdspValueType::float32Type);

    YdspIrBlock disjointBlock;
    disjointBlock.insts.push_back ({ YdspIrOp::constI, idx0, -1, -1, -1, -1, 0.0, 0 });
    disjointBlock.insts.push_back ({ YdspIrOp::constI, idx1, -1, -1, -1, -1, 0.0, 1 });
    disjointBlock.insts.push_back ({ YdspIrOp::constF, vWrite, -1, -1, -1, -1, 3.0 });
    disjointBlock.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, idx0, vWrite, -1, 0 });
    disjointBlock.insts.push_back ({ YdspIrOp::loadStateArrayF, vLoad, idx1, -1, -1, 0 });
    disjointBlock.insts.push_back ({ YdspIrOp::divF, vDiv, vLoad, vWrite });
    disjointBlock.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, idx1, vDiv, -1, 0 });
    fnDisjoint.blocks.push_back (disjointBlock);

    EXPECT_TRUE (optimizer.scheduleInstructions (fnDisjoint));
    const int posStore0 = findInstIndex (fnDisjoint.blocks[0], YdspIrOp::storeStateArrayF, -1, 0);
    const int posLoad1 = findInstIndex (fnDisjoint.blocks[0], YdspIrOp::loadStateArrayF, vLoad, 0);
    EXPECT_LT (posLoad1, posStore0);
}

//==============================================================================

TEST_F (YdspSchedulerTests, PreservesMemorySlotsCallsAndLoopInductionTail)
{
    YdspIrFunction fn;
    const int iVal = addValue (fn, YdspValueType::int32Type);
    const int iNext = addValue (fn, YdspValueType::int32Type);
    const int oneI = addValue (fn, YdspValueType::int32Type);
    const int boundI = addValue (fn, YdspValueType::int32Type);
    const int cond = addValue (fn, YdspValueType::boolType);
    const int s0 = addValue (fn, YdspValueType::float32Type);
    const int s1 = addValue (fn, YdspValueType::float32Type);
    const int s2 = addValue (fn, YdspValueType::float32Type);
    const int p0 = addValue (fn, YdspValueType::float32Type);
    const int sinRes = addValue (fn, YdspValueType::float32Type);
    const int postConst = addValue (fn, YdspValueType::float32Type);
    const int postMul = addValue (fn, YdspValueType::float32Type);

    // Block 0: preheader
    YdspIrBlock preheader;
    preheader.insts.push_back ({ YdspIrOp::constI, oneI, -1, -1, -1, -1, 0.0, 1 });
    preheader.insts.push_back ({ YdspIrOp::constI, boundI, -1, -1, -1, -1, 0.0, 4 });
    preheader.insts.push_back ({ YdspIrOp::constI, iVal, -1, -1, -1, -1, 0.0, 0 });
    preheader.term = YdspIrTerm::fallthrough;

    // Block 1: loop header
    YdspIrBlock header;
    header.insts.push_back ({ YdspIrOp::ltI, cond, iVal, boundI });
    header.term = YdspIrTerm::branchIf;
    header.termCond = cond;
    header.termTarget = 2;
    header.termTarget2 = 3;

    // Block 2: loop body (non-unrolled) with state slot, param slot, call barrier, and induction tail
    YdspIrBlock body;
    body.insts.push_back ({ YdspIrOp::constF, s0, -1, -1, -1, -1, 1.0 });
    body.insts.push_back ({ YdspIrOp::storeStateF, -1, s0, -1, -1, 0 });
    body.insts.push_back ({ YdspIrOp::loadStateF, s1, 0 });
    body.insts.push_back ({ YdspIrOp::storeParam, -1, s1, -1, -1, 2 });
    body.insts.push_back ({ YdspIrOp::loadParam, p0, 2 });
    body.insts.push_back ({ YdspIrOp::sinF, sinRes, p0 });
    body.insts.push_back ({ YdspIrOp::constF, postConst, -1, -1, -1, -1, 0.5 });
    body.insts.push_back ({ YdspIrOp::mulF, postMul, sinRes, postConst });
    body.insts.push_back ({ YdspIrOp::storeStateF, -1, postMul, -1, -1, 0 });
    body.insts.push_back ({ YdspIrOp::loadStateF, s2, 0 });
    body.insts.push_back ({ YdspIrOp::storeOutput, -1, iVal, s2, -1, 0 });
    body.insts.push_back ({ YdspIrOp::addI, iNext, iVal, oneI });
    body.insts.push_back ({ YdspIrOp::movI, iVal, iNext });
    body.term = YdspIrTerm::branch;
    body.termTarget = 1;

    YdspIrBlock exitBlock;
    exitBlock.term = YdspIrTerm::fallthrough;

    fn.blocks = { preheader, header, body, exitBlock };
    YdspIrLoop loop;
    loop.id = 0;
    loop.headerBlock = 1;
    loop.exitBlock = 3;
    loop.induction = iVal;
    loop.bound.kind = YdspLoopBoundKind::constant;
    loop.bound.constant = 4;
    loop.unrolled = false;
    fn.loops.push_back (loop);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.scheduleInstructions (fn);

    const auto& scheduledBody = fn.blocks[2];
    const auto n = scheduledBody.insts.size();
    ASSERT_GE (n, 2u);

    // Trailing induction update must remain pinned at the end of the non-unrolled loop body.
    EXPECT_EQ (YdspIrOp::addI, scheduledBody.insts[n - 2].op);
    EXPECT_EQ (iNext, scheduledBody.insts[n - 2].result);
    EXPECT_EQ (YdspIrOp::movI, scheduledBody.insts[n - 1].op);
    EXPECT_EQ (iVal, scheduledBody.insts[n - 1].result);

    // Call barrier prevents post-call constF from hoisting before sinF.
    const int sinIdx = findInstIndex (scheduledBody, YdspIrOp::sinF);
    const int postConstIdx = findInstIndex (scheduledBody, YdspIrOp::constF, postConst);
    EXPECT_LT (sinIdx, postConstIdx);

    // State/param/output/event/trace ordering preserved.
    const int firstStoreState = findInstIndex (scheduledBody, YdspIrOp::storeStateF, -1, 0);
    const int firstLoadState = findInstIndex (scheduledBody, YdspIrOp::loadStateF, s1);
    const int storeParamIdx = findInstIndex (scheduledBody, YdspIrOp::storeParam, -1, 2);
    const int loadParamIdx = findInstIndex (scheduledBody, YdspIrOp::loadParam, p0);
    EXPECT_LT (firstStoreState, firstLoadState);
    EXPECT_LT (storeParamIdx, loadParamIdx);
    EXPECT_LT (loadParamIdx, sinIdx);

    // Output stream store->load and event/trace barriers stay ordered.
    YdspIrFunction fnSideEffects;
    const int streamIdx = addValue (fnSideEffects, YdspValueType::int32Type);
    const int val0 = addValue (fnSideEffects, YdspValueType::float32Type);
    const int val1 = addValue (fnSideEffects, YdspValueType::float32Type);
    const int outRead = addValue (fnSideEffects, YdspValueType::float32Type);
    const int evVal = addValue (fnSideEffects, YdspValueType::float32Type);
    const int evOff = addValue (fnSideEffects, YdspValueType::int32Type);

    YdspIrBlock sideBlock;
    sideBlock.insts.push_back ({ YdspIrOp::constI, streamIdx, -1, -1, -1, -1, 0.0, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::constI, evOff, -1, -1, -1, -1, 0.0, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::constF, val0, -1, -1, -1, -1, 1.0 });
    sideBlock.insts.push_back ({ YdspIrOp::storeOutput, -1, streamIdx, val0, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::loadOutput, outRead, streamIdx, -1, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::addF, val1, outRead, val0 });
    sideBlock.insts.push_back ({ YdspIrOp::storeOutput, -1, streamIdx, val1, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::constF, evVal, -1, -1, -1, -1, 0.5 });
    sideBlock.insts.push_back ({ YdspIrOp::storeEventFieldF, -1, evVal, -1, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::emitEvent, -1, evOff, -1, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::traceFloat, -1, evVal, -1, -1, 0 });
    sideBlock.insts.push_back ({ YdspIrOp::traceCommit, -1, -1, -1, -1, 0 });
    fnSideEffects.blocks.push_back (sideBlock);

    optimizer.scheduleInstructions (fnSideEffects);
    const auto& sb = fnSideEffects.blocks[0];

    const int outStore0 = findInstIndex (sb, YdspIrOp::storeOutput, -1, 0);
    const int outLoad0 = findInstIndex (sb, YdspIrOp::loadOutput, outRead, 0);
    const int storeEv = findInstIndex (sb, YdspIrOp::storeEventFieldF);
    const int emitEv = findInstIndex (sb, YdspIrOp::emitEvent);
    const int traceF = findInstIndex (sb, YdspIrOp::traceFloat);
    const int traceC = findInstIndex (sb, YdspIrOp::traceCommit);
    EXPECT_LT (outStore0, outLoad0);
    EXPECT_LT (storeEv, emitEv);
    EXPECT_LT (emitEv, traceF);
    EXPECT_LT (traceF, traceC);
}

//==============================================================================

TEST_F (YdspSchedulerTests, InterleavesParallelChainsByCriticalPath)
{
    // Reverb-like block with 4 independent delay taps laid out sequentially,
    // where tap 3 has the longest critical path (divF).
    YdspIrFunction fn;
    const int idx = addValue (fn, YdspValueType::int32Type);
    const int gain = addValue (fn, YdspValueType::float32Type);
    const int input = addValue (fn, YdspValueType::float32Type);

    YdspIrBlock block;
    block.insts.push_back ({ YdspIrOp::constI, idx, -1, -1, -1, -1, 0.0, 0 });
    block.insts.push_back ({ YdspIrOp::constF, gain, -1, -1, -1, -1, 0.75 });
    block.insts.push_back ({ YdspIrOp::loadInput, input, idx, -1, -1, 0 });

    std::vector<int> tapLoads;
    std::vector<int> tapOuts;

    for (int tap = 0; tap < 4; ++tap)
    {
        const int delayed = addValue (fn, YdspValueType::float32Type);
        const int scaled = addValue (fn, YdspValueType::float32Type);
        const int nextVal = addValue (fn, YdspValueType::float32Type);
        const int memBase = tap * 16;

        block.insts.push_back ({ YdspIrOp::loadStateArrayF, delayed, idx, -1, -1, memBase });
        if (tap == 3)
            block.insts.push_back ({ YdspIrOp::divF, scaled, delayed, gain });
        else
            block.insts.push_back ({ YdspIrOp::mulF, scaled, delayed, gain });
        block.insts.push_back ({ YdspIrOp::addF, nextVal, scaled, input });
        block.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, idx, nextVal, -1, memBase });

        tapLoads.push_back (delayed);
        tapOuts.push_back (nextVal);
    }

    const int mix0 = addValue (fn, YdspValueType::float32Type);
    const int mix1 = addValue (fn, YdspValueType::float32Type);
    const int mix2 = addValue (fn, YdspValueType::float32Type);
    block.insts.push_back ({ YdspIrOp::addF, mix0, tapOuts[0], tapOuts[1] });
    block.insts.push_back ({ YdspIrOp::addF, mix1, tapOuts[2], tapOuts[3] });
    block.insts.push_back ({ YdspIrOp::addF, mix2, mix0, mix1 });
    block.insts.push_back ({ YdspIrOp::storeOutput, -1, idx, mix2, -1, 0 });

    fn.blocks.push_back (block);
    const auto originalBlock = fn.blocks[0];

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    const bool changed = optimizer.scheduleInstructions (fn);

    EXPECT_TRUE (changed);

    const auto& scheduled = fn.blocks[0];

    int firstStorePos = static_cast<int> (scheduled.insts.size());
    int lastLoadPos = -1;

    for (int tap = 0; tap < 4; ++tap)
    {
        const int loadPos = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, tapLoads[static_cast<size_t> (tap)]);
        const int storePos = findInstIndex (scheduled, YdspIrOp::storeStateArrayF, -1, tap * 16);
        ASSERT_GE (loadPos, 0);
        ASSERT_GE (storePos, 0);
        EXPECT_LT (loadPos, storePos);

        lastLoadPos = std::max (lastLoadPos, loadPos);
        firstStorePos = std::min (firstStorePos, storePos);
    }

    // All 4 parallel tap loads are interleaved before the first tap store.
    EXPECT_LT (lastLoadPos, firstStorePos);

    // Tap 3 (divF, longest critical path) is issued before shorter taps.
    const int loadTap0 = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, tapLoads[0]);
    const int loadTap3 = findInstIndex (scheduled, YdspIrOp::loadStateArrayF, tapLoads[3]);
    EXPECT_LT (loadTap3, loadTap0);

    // Verify exact numerical equivalence.
    SimState initial;
    initial.regs.assign (fn.valueTypes.size(), 0.0f);
    initial.intRegs.assign (fn.valueTypes.size(), 0);
    initial.inputStream.assign (64, 0.25f);
    initial.outputStream.assign (64, 0.0f);
    initial.stateArrayF.assign (64, 0.0f);
    initial.stateArrayF[0] = 0.1f;
    initial.stateArrayF[16] = 0.2f;
    initial.stateArrayF[32] = 0.3f;
    initial.stateArrayF[48] = 0.4f;

    const auto expected = simulateBlock (originalBlock, initial);
    const auto actual = simulateBlock (scheduled, initial);

    EXPECT_FLOAT_EQ (expected.outputStream[0], actual.outputStream[0]);
    for (int tap = 0; tap < 4; ++tap)
        EXPECT_FLOAT_EQ (expected.stateArrayF[static_cast<size_t> (tap * 16)],
                         actual.stateArrayF[static_cast<size_t> (tap * 16)]);
}

//==============================================================================

TEST_F (YdspSchedulerTests, LimitsRegisterPressureOnWideIndependentChains)
{
    std::vector<int> peaks;

    for (const int vectorWidth : { 4, 8 })
    {
        YdspIrFunction fn;
        fn.vectorWidth = vectorWidth;

        const int idx = addValue (fn, YdspValueType::int32Type);

        constexpr int numPartials = 24;
        std::vector<int> loads;
        std::vector<int> coeffs;
        std::vector<int> products;

        for (int p = 0; p < numPartials; ++p)
        {
            loads.push_back (addValue (fn, YdspValueType::float32Type));
            coeffs.push_back (addValue (fn, YdspValueType::float32Type));
            products.push_back (addValue (fn, YdspValueType::float32Type));
        }

        // Equal-height independent chains laid out in worst-case high-pressure order:
        // all 24 loads, then all 24 consts, then all 24 multiplies, then all 24 stores.
        YdspIrBlock block;
        block.insts.push_back ({ YdspIrOp::constI, idx, -1, -1, -1, -1, 0.0, 0 });

        for (int p = 0; p < numPartials; ++p)
            block.insts.push_back ({ YdspIrOp::loadStateArrayF, loads[static_cast<size_t> (p)], idx, -1, -1, p * 4 });

        for (int p = 0; p < numPartials; ++p)
            block.insts.push_back ({ YdspIrOp::constF, coeffs[static_cast<size_t> (p)], -1, -1, -1, -1, 0.1 * (p + 1) });

        for (int p = 0; p < numPartials; ++p)
            block.insts.push_back ({ YdspIrOp::mulF, products[static_cast<size_t> (p)],
                                     loads[static_cast<size_t> (p)], coeffs[static_cast<size_t> (p)] });

        for (int p = 0; p < numPartials; ++p)
            block.insts.push_back ({ YdspIrOp::storeStateArrayF, -1, idx, products[static_cast<size_t> (p)], -1, p * 4 });

        fn.blocks.push_back (block);
        const auto originalBlock = fn.blocks[0];

        const int peakBefore = computePeakLiveFp (fn, originalBlock);
        EXPECT_GE (peakBefore, 48);

        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        const bool changed = optimizer.scheduleInstructions (fn);

        EXPECT_TRUE (changed);

        const int peakAfter = computePeakLiveFp (fn, fn.blocks[0]);
        const int softFpLimit = vectorWidth >= 8 ? 14 : 16;

        // Pressure relief keeps peak live FP registers bounded near softFpLimit.
        EXPECT_LT (peakAfter, peakBefore / 2);
        EXPECT_LE (peakAfter, softFpLimit + 1);
        peaks.push_back (peakAfter);

        SimState initial;
        initial.regs.assign (fn.valueTypes.size(), 0.0f);
        initial.intRegs.assign (fn.valueTypes.size(), 0);
        initial.stateArrayF.assign (static_cast<size_t> (numPartials * 4), 0.0f);
        for (int p = 0; p < numPartials; ++p)
            initial.stateArrayF[static_cast<size_t> (p * 4)] = 0.5f + 0.25f * static_cast<float> (p);

        const auto expected = simulateBlock (originalBlock, initial);
        const auto actual = simulateBlock (fn.blocks[0], initial);

        for (int p = 0; p < numPartials; ++p)
            EXPECT_FLOAT_EQ (expected.stateArrayF[static_cast<size_t> (p * 4)],
                             actual.stateArrayF[static_cast<size_t> (p * 4)]);
    }

    ASSERT_EQ (2u, peaks.size());
    EXPECT_LT (peaks[1], peaks[0]);
}
