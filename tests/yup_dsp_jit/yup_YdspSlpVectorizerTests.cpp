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

  ==============================================================================
*/

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

#include <memory>
#include <vector>

using namespace yup;

//==============================================================================

class YdspSlpVectorizerTests : public ::testing::Test
{
protected:
    static int newValue (YdspIrFunction& fn, YdspValueType type, int lanes = 1)
    {
        const int id = static_cast<int> (fn.valueTypes.size());
        fn.valueTypes.push_back (type);

        if (! fn.valueLanes.empty())
            fn.valueLanes.push_back (lanes);

        return id;
    }

    static int emitInst (YdspIrFunction& fn,
                         int blockIdx,
                         YdspIrOp op,
                         int result = -1,
                         int a = -1,
                         int b = -1,
                         int c = -1,
                         int memIndex = -1,
                         double fvalue = 0.0,
                         int64_t ivalue = 0)
    {
        YdspIrInst inst;
        inst.op = op;
        inst.result = result;
        inst.a = a;
        inst.b = b;
        inst.c = c;
        inst.memIndex = memIndex;
        inst.fvalue = fvalue;
        inst.ivalue = ivalue;
        fn.blocks[static_cast<size_t> (blockIdx)].insts.push_back (inst);
        return result;
    }

    static int countOp (const YdspIrFunction& fn, YdspIrOp op)
    {
        int count = 0;

        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
                if (inst.op == op)
                    ++count;

        return count;
    }

    static int countWidenedOp (const YdspIrFunction& fn, YdspIrOp op, int expectedLanes)
    {
        int count = 0;

        for (const auto& block : fn.blocks)
        {
            for (const auto& inst : block.insts)
            {
                if (inst.op != op)
                    continue;

                const int val = inst.result >= 0 ? inst.result
                              : (op == YdspIrOp::storeStateF ? inst.a : inst.b);

                if (fn.laneCountOf (val) == expectedLanes)
                    ++count;
            }
        }

        return count;
    }

    static std::unique_ptr<YdspIrProgram> buildIrFromSource (StringRef source, YdspDiagnostics& diagnostics)
    {
        YdspLexer lexer (source, diagnostics);
        auto tokens = lexer.tokenize();

        YdspParser parser (std::move (tokens), diagnostics);
        auto program = parser.parseProgram();
        if (program == nullptr)
            return nullptr;

        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (program));
        if (analyzed == nullptr)
            return nullptr;

        YdspOptimizer optimizer (diagnostics);
        optimizer.setVectorizationEnabled (false);
        optimizer.setUnrollingEnabled (true);
        return optimizer.build (*analyzed);
    }
};

//==============================================================================

TEST_F (YdspSlpVectorizerTests, VectorizesContiguousStateArrayOperations)
{
    YdspIrFunction fn;
    fn.float32ArrayElements = 4;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });
    fn.blocks.resize (1);

    const int coeff = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int bias = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.25);

    for (int i = 0; i < 4; ++i)
    {
        const int idx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx, -1, -1, 0);
        const int mul = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), ld, coeff);
        const int add = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), mul, bias);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, idx, add, -1, 0);
    }

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (4, fn.vectorWidth);

    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::loadStateArrayF, 4));
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::storeStateArrayF, 4));
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::mulF, 4));
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::addF, 4));
    EXPECT_EQ (2, countOp (fn, YdspIrOp::vsplat));
    EXPECT_EQ (0, countOp (fn, YdspIrOp::vinsertF));
    EXPECT_EQ (0, countOp (fn, YdspIrOp::vextractF));
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, VectorizesContiguousArrayReduction)
{
    YdspIrFunction fn;
    fn.float32ArrayElements = 4;
    fn.numOutputs = 1;
    fn.outputTypes.push_back (YdspValueType::float32Type);
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });
    fn.blocks.resize (1);

    const int outIdx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, 0);
    int sum = -1;

    for (int i = 0; i < 4; ++i)
    {
        const int idx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx, -1, -1, 0);

        if (sum < 0)
            sum = ld;
        else
            sum = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), sum, ld);
    }

    emitInst (fn, 0, YdspIrOp::storeOutput, -1, outIdx, sum, -1, 0);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (4, fn.vectorWidth);
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::loadStateArrayF, 4));
    EXPECT_EQ (1, countOp (fn, YdspIrOp::vreduceAddF));
    EXPECT_EQ (0, countOp (fn, YdspIrOp::addF));
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, VectorizesReverbStyleEightParallelCombs)
{
    // Sample-loop structure: block 0 (preheader), block 1 (header), block 2 (body), block 3 (epilogue).
    YdspIrFunction fn;
    fn.numInputs = 1;
    fn.numOutputs = 1;
    fn.inputTypes.push_back (YdspValueType::float32Type);
    fn.outputTypes.push_back (YdspValueType::float32Type);
    fn.float32Scalars = 8;
    fn.int32Scalars = 8;
    fn.blocks.resize (4);

    constexpr int combLengths[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    int arrayBase[8] {};
    int totalElems = 0;

    for (int k = 0; k < 8; ++k)
    {
        arrayBase[k] = totalElems;
        fn.stateArrays.push_back ({ YdspValueType::float32Type, totalElems, combLengths[k], {} });
        totalElems += combLengths[k];
    }

    fn.float32ArrayElements = totalElems;

    // Block 0: preheader loads state & constants.
    const int sampleIdx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, 0);
    const int blockSize = emitInst (fn, 0, YdspIrOp::loadBlockSize, newValue (fn, YdspValueType::int32Type));
    const int oneI = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, 1);
    const int zeroI = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, 0);

    const int damping = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int oneMinusDamping = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int feedback = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.84);
    const int inputGain = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.015);

    std::vector<int> dampReg (8);
    std::vector<int> wReg (8);
    std::vector<int> lenConst (8);

    for (int k = 0; k < 8; ++k)
    {
        dampReg[static_cast<size_t> (k)] = emitInst (fn, 0, YdspIrOp::loadStateF, newValue (fn, YdspValueType::float32Type), k);
        wReg[static_cast<size_t> (k)] = emitInst (fn, 0, YdspIrOp::loadStateI, newValue (fn, YdspValueType::int32Type), k);
        lenConst[static_cast<size_t> (k)] = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, combLengths[k]);
    }

    // Block 1: header.
    const int loopCond = emitInst (fn, 1, YdspIrOp::ltI, newValue (fn, YdspValueType::boolType), sampleIdx, blockSize);
    fn.blocks[1].term = YdspIrTerm::branchIf;
    fn.blocks[1].termCond = loopCond;
    fn.blocks[1].termTarget = 2;
    fn.blocks[1].termTarget2 = 3;

    // Block 2: body with 8 parallel combs and horizontal sum.
    const int inSample = emitInst (fn, 2, YdspIrOp::loadInput, newValue (fn, YdspValueType::float32Type), sampleIdx, -1, -1, 0);
    const int scaledIn = emitInst (fn, 2, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), inSample, inputGain);

    std::vector<int> dVals (8);

    for (int k = 0; k < 8; ++k)
    {
        const auto ku = static_cast<size_t> (k);
        const int d = emitInst (fn, 2, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), wReg[ku], -1, -1, arrayBase[k]);
        dVals[ku] = d;

        const int t1 = emitInst (fn, 2, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), dampReg[ku], damping);
        const int t2 = emitInst (fn, 2, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), d, oneMinusDamping);
        const int t3 = emitInst (fn, 2, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), t1, t2);
        emitInst (fn, 2, YdspIrOp::movF, dampReg[ku], t3);

        const int t4 = emitInst (fn, 2, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), feedback, t3);
        const int t5 = emitInst (fn, 2, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), t4, scaledIn);
        emitInst (fn, 2, YdspIrOp::storeStateArrayF, -1, wReg[ku], t5, -1, arrayBase[k]);

        const int wNext = emitInst (fn, 2, YdspIrOp::addI, newValue (fn, YdspValueType::int32Type), wReg[ku], oneI);
        const int wWrapCond = emitInst (fn, 2, YdspIrOp::geI, newValue (fn, YdspValueType::boolType), wNext, lenConst[ku]);
        emitInst (fn, 2, YdspIrOp::selectB, wReg[ku], wWrapCond, zeroI, wNext);
    }

    int wet = dVals[0];

    for (int k = 1; k < 8; ++k)
        wet = emitInst (fn, 2, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), wet, dVals[static_cast<size_t> (k)]);

    emitInst (fn, 2, YdspIrOp::storeOutput, -1, sampleIdx, wet, -1, 0);

    const int nextSample = emitInst (fn, 2, YdspIrOp::addI, newValue (fn, YdspValueType::int32Type), sampleIdx, oneI);
    emitInst (fn, 2, YdspIrOp::movI, sampleIdx, nextSample);
    fn.blocks[2].term = YdspIrTerm::branch;
    fn.blocks[2].termTarget = 1;

    // Block 3: epilogue stores carried states.
    for (int k = 0; k < 8; ++k)
    {
        emitInst (fn, 3, YdspIrOp::storeStateF, -1, dampReg[static_cast<size_t> (k)], -1, -1, k);
        emitInst (fn, 3, YdspIrOp::storeStateI, -1, wReg[static_cast<size_t> (k)], -1, -1, k);
    }

    YdspIrLoop loop;
    loop.id = 0;
    loop.headerBlock = 1;
    loop.exitBlock = 3;
    loop.induction = sampleIdx;
    loop.bound = { YdspLoopBoundKind::blockSize, 0 };
    fn.loops.push_back (loop);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (8);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (8, fn.vectorWidth);

    // Expect 8-lane vector ops, gathers, extracts, splats, and vreduceAddF.
    EXPECT_EQ (16, countOp (fn, YdspIrOp::vinsertF));
    EXPECT_EQ (16, countOp (fn, YdspIrOp::vextractF));
    EXPECT_EQ (1, countOp (fn, YdspIrOp::vreduceAddF));
    EXPECT_EQ (3, countWidenedOp (fn, YdspIrOp::mulF, 8));
    EXPECT_EQ (2, countWidenedOp (fn, YdspIrOp::addF, 8));
    EXPECT_EQ (8, countOp (fn, YdspIrOp::loadStateArrayF));
    EXPECT_EQ (8, countOp (fn, YdspIrOp::storeStateArrayF));

    // Loop-invariant splats (damping, oneMinusDamping, feedback) should be hoisted into block 0.
    int preheaderSplats = 0;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::vsplat)
            ++preheaderSplats;

    EXPECT_EQ (3, preheaderSplats);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, VectorizesCompiledReverbPatchFromSource)
{
    constexpr auto reverbSource = R"YDSP(
        processor Reverb {
            input stream in;
            output stream out;

            input parameter float mix = 0.33;
            input parameter float damping = 0.5;
            input parameter float roomSize = 0.8;

            state float buf1[1116]; state int w1; state float damp1;
            state float buf2[1188]; state int w2; state float damp2;
            state float buf3[1277]; state int w3; state float damp3;
            state float buf4[1356]; state int w4; state float damp4;
            state float buf5[1422]; state int w5; state float damp5;
            state float buf6[1491]; state int w6; state float damp6;
            state float buf7[1557]; state int w7; state float damp7;
            state float buf8[1617]; state int w8; state float damp8;

            process {
                float feedback = roomSize * 0.28 + 0.7;

                let d1 = buf1[w1];
                damp1 = damp1 * damping + d1 * (1.0 - damping);
                buf1[w1] = feedback * damp1 + 0.015 * in;
                w1 = w1 + 1;
                if (w1 >= 1116) { w1 = 0; }

                let d2 = buf2[w2];
                damp2 = damp2 * damping + d2 * (1.0 - damping);
                buf2[w2] = feedback * damp2 + 0.015 * in;
                w2 = w2 + 1;
                if (w2 >= 1188) { w2 = 0; }

                let d3 = buf3[w3];
                damp3 = damp3 * damping + d3 * (1.0 - damping);
                buf3[w3] = feedback * damp3 + 0.015 * in;
                w3 = w3 + 1;
                if (w3 >= 1277) { w3 = 0; }

                let d4 = buf4[w4];
                damp4 = damp4 * damping + d4 * (1.0 - damping);
                buf4[w4] = feedback * damp4 + 0.015 * in;
                w4 = w4 + 1;
                if (w4 >= 1356) { w4 = 0; }

                let d5 = buf5[w5];
                damp5 = damp5 * damping + d5 * (1.0 - damping);
                buf5[w5] = feedback * damp5 + 0.015 * in;
                w5 = w5 + 1;
                if (w5 >= 1422) { w5 = 0; }

                let d6 = buf6[w6];
                damp6 = damp6 * damping + d6 * (1.0 - damping);
                buf6[w6] = feedback * damp6 + 0.015 * in;
                w6 = w6 + 1;
                if (w6 >= 1491) { w6 = 0; }

                let d7 = buf7[w7];
                damp7 = damp7 * damping + d7 * (1.0 - damping);
                buf7[w7] = feedback * damp7 + 0.015 * in;
                w7 = w7 + 1;
                if (w7 >= 1557) { w7 = 0; }

                let d8 = buf8[w8];
                damp8 = damp8 * damping + d8 * (1.0 - damping);
                buf8[w8] = feedback * damp8 + 0.015 * in;
                w8 = w8 + 1;
                if (w8 >= 1617) { w8 = 0; }

                let wet = d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8;
                out = (1.0 - mix) * in + mix * wet;
            }
        }

        graph G {
            input stream x;
            output stream y;
            node r = Reverb;
            connection { x -> r.in; r.out -> y; }
        }
    )YDSP";

    YdspDiagnostics diagnostics;
    auto ir = buildIrFromSource (reverbSource, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (8);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (8, fn.vectorWidth);
    EXPECT_GT (countOp (fn, YdspIrOp::vinsertF), 0);
    EXPECT_GT (countOp (fn, YdspIrOp::vextractF), 0);
    EXPECT_EQ (1, countOp (fn, YdspIrOp::vreduceAddF));
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, VectorizesFloatComparisonsAndSelect)
{
    YdspIrFunction fn;
    fn.float32ArrayElements = 8;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 8, {} });
    fn.blocks.resize (1);

    const int limit = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 1.0);

    for (int i = 0; i < 4; ++i)
    {
        const int idx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx, -1, -1, 0);
        const int cmp = emitInst (fn, 0, YdspIrOp::gtF, newValue (fn, YdspValueType::boolType), ld, limit);
        const int sel = emitInst (fn, 0, YdspIrOp::selectB, newValue (fn, YdspValueType::float32Type), cmp, limit, ld);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, idx, sel, -1, 0);
    }

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (4, fn.vectorWidth);
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::gtF, 4));
    EXPECT_EQ (1, countWidenedOp (fn, YdspIrOp::selectB, 4));
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, RejectsCrossLaneDependencyChain)
{
    YdspIrFunction fn;
    fn.float32ArrayElements = 4;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });
    fn.blocks.resize (1);

    const int gain = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    int prev = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 1.0);

    for (int i = 0; i < 4; ++i)
    {
        const int idx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx, -1, -1, 0);
        const int mul = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), ld, gain);
        const int add = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), mul, prev);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, idx, add, -1, 0);
        prev = add; // serial dependency from lane i to lane i + 1
    }

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_FALSE (optimizer.slpVectorize (fn));
    EXPECT_FALSE (fn.vectorized);
    EXPECT_EQ (1, fn.vectorWidth);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, RejectsAliasingMemoryDependence)
{
    // Four writes to dynamic indices into the SAME state array may alias previous reads.
    YdspIrFunction fn;
    fn.float32ArrayElements = 16;
    fn.int32Scalars = 4;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 16, {} });
    fn.blocks.resize (1);

    const int c1 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int c2 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.25);

    for (int i = 0; i < 4; ++i)
    {
        const int w = emitInst (fn, 0, YdspIrOp::loadStateI, newValue (fn, YdspValueType::int32Type), i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), w, -1, -1, 0);
        const int mul = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), ld, c1);
        const int add = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), mul, c2);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, w, add, -1, 0);
    }

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_FALSE (optimizer.slpVectorize (fn));
    EXPECT_FALSE (fn.vectorized);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, RejectsUnprofitableSingleLevelGatherReduction)
{
    // DelayTaps shape: 4 distinct inputs multiplied by 4 distinct weights and summed.
    YdspIrFunction fn;
    fn.numInputs = 1;
    fn.numOutputs = 1;
    fn.inputTypes.push_back (YdspValueType::float32Type);
    fn.outputTypes.push_back (YdspValueType::float32Type);
    fn.float32ArrayElements = 3;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 1, {} });
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 1, 1, {} });
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 2, 1, {} });
    fn.blocks.resize (1);

    const int idx0 = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, 0);
    const int in0 = emitInst (fn, 0, YdspIrOp::loadInput, newValue (fn, YdspValueType::float32Type), idx0, -1, -1, 0);
    const int d1 = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx0, -1, -1, 0);
    const int d2 = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx0, -1, -1, 1);
    const int d3 = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx0, -1, -1, 2);

    const int w0 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int w1 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.25);
    const int w2 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.15);
    const int w3 = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.1);

    const int m0 = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), in0, w0);
    const int m1 = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), d1, w1);
    const int m2 = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), d2, w2);
    const int m3 = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), d3, w3);

    const int s0 = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), m0, m1);
    const int s1 = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), s0, m2);
    const int s2 = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), s1, m3);
    emitInst (fn, 0, YdspIrOp::storeOutput, -1, idx0, s2, -1, 0);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_FALSE (optimizer.slpVectorize (fn));
    EXPECT_FALSE (fn.vectorized);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, RespectsExistingVectorWidthWhenAlreadyVectorized)
{
    YdspIrFunction fn;
    fn.vectorized = true;
    fn.vectorWidth = 4;
    fn.float32ArrayElements = 8;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 8, {} });
    fn.blocks.resize (1);

    const int coeff = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);

    for (int i = 0; i < 8; ++i)
    {
        const int idx = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);
        const int ld = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idx, -1, -1, 0);
        const int mul = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), ld, coeff);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, idx, mul, -1, 0);
    }

    fn.valueLanes.assign (fn.valueTypes.size(), 1);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (8);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_EQ (4, fn.vectorWidth);
    EXPECT_EQ (2, countWidenedOp (fn, YdspIrOp::loadStateArrayF, 4));
    EXPECT_EQ (2, countWidenedOp (fn, YdspIrOp::storeStateArrayF, 4));
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, RejectsNonSsaRedefinitionConflictAcrossLanes)
{
    // Shared non-SSA register `x` redefined between lane 0 and lane 1.
    YdspIrFunction fn;
    fn.float32ArrayElements = 4;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });
    fn.blocks.resize (1);

    const int coeff = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    const int sharedReg = newValue (fn, YdspValueType::float32Type);
    std::vector<int> indices (4);

    for (int i = 0; i < 4; ++i)
        indices[static_cast<size_t> (i)] = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, i);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_FALSE (optimizer.slpVectorize (fn));
    EXPECT_FALSE (fn.vectorized);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, HoistsLoopInvariantPreheaderGathersOutOfLoop)
{
    // Preheader (B0) computes 4 independent loop-invariant parameter factors
    // used in 4 parallel filter lanes inside the loop body (B2).
    YdspIrFunction fn;
    fn.isSampleMode = true;
    fn.numInputs = 1;
    fn.numOutputs = 1;
    fn.float32Scalars = 4;
    fn.float32ArrayElements = 4;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });

    const int blockSize = newValue (fn, YdspValueType::int32Type);
    const int idxZero = newValue (fn, YdspValueType::int32Type);
    const int idxOne = newValue (fn, YdspValueType::int32Type);
    const int sampleIdx = newValue (fn, YdspValueType::int32Type);
    const int cond = newValue (fn, YdspValueType::boolType);
    const int nextIdx = newValue (fn, YdspValueType::int32Type);

    std::vector<int> coeffs (4);
    for (int k = 0; k < 4; ++k)
        coeffs[static_cast<size_t> (k)] = newValue (fn, YdspValueType::float32Type);

    std::vector<int> arrIdx (4), loaded (4), mulRes (4), addRes (4);
    for (int k = 0; k < 4; ++k)
    {
        arrIdx[static_cast<size_t> (k)] = newValue (fn, YdspValueType::int32Type);
        loaded[static_cast<size_t> (k)] = newValue (fn, YdspValueType::float32Type);
        mulRes[static_cast<size_t> (k)] = newValue (fn, YdspValueType::float32Type);
        addRes[static_cast<size_t> (k)] = newValue (fn, YdspValueType::float32Type);
    }

    const int inputVal = newValue (fn, YdspValueType::float32Type);

    fn.blocks.resize (4);
    emitInst (fn, 0, YdspIrOp::loadBlockSize, blockSize);
    emitInst (fn, 0, YdspIrOp::constI, idxZero, -1, -1, -1, -1, 0.0, 0);
    emitInst (fn, 0, YdspIrOp::constI, idxOne, -1, -1, -1, -1, 0.0, 1);

    for (int k = 0; k < 4; ++k)
    {
        emitInst (fn, 0, YdspIrOp::loadParam, coeffs[static_cast<size_t> (k)], k);
        emitInst (fn, 0, YdspIrOp::constI, arrIdx[static_cast<size_t> (k)], -1, -1, -1, -1, 0.0, k);
    }

    emitInst (fn, 0, YdspIrOp::movI, sampleIdx, idxZero);
    fn.blocks[0].term = YdspIrTerm::fallthrough;

    emitInst (fn, 1, YdspIrOp::ltI, cond, sampleIdx, blockSize);
    fn.blocks[1].term = YdspIrTerm::branchIf;
    fn.blocks[1].termCond = cond;
    fn.blocks[1].termTarget = 2;
    fn.blocks[1].termTarget2 = 3;

    emitInst (fn, 2, YdspIrOp::loadInput, inputVal, sampleIdx, -1, -1, 0);

    for (int k = 0; k < 4; ++k)
    {
        const auto ku = static_cast<size_t> (k);
        emitInst (fn, 2, YdspIrOp::loadStateArrayF, loaded[ku], arrIdx[ku], -1, -1, 0);
        emitInst (fn, 2, YdspIrOp::mulF, mulRes[ku], loaded[ku], coeffs[ku]);
        emitInst (fn, 2, YdspIrOp::addF, addRes[ku], mulRes[ku], inputVal);
        emitInst (fn, 2, YdspIrOp::storeStateArrayF, -1, arrIdx[ku], addRes[ku], -1, 0);
    }

    emitInst (fn, 2, YdspIrOp::addI, nextIdx, sampleIdx, idxOne);
    emitInst (fn, 2, YdspIrOp::movI, sampleIdx, nextIdx);
    fn.blocks[2].term = YdspIrTerm::branch;
    fn.blocks[2].termTarget = 1;

    YdspIrLoop loop;
    loop.id = 0;
    loop.headerBlock = 1;
    loop.exitBlock = 3;
    loop.induction = sampleIdx;
    loop.bound = { YdspLoopBoundKind::blockSize, 0 };
    fn.loops.push_back (loop);

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_TRUE (optimizer.slpVectorize (fn));
    EXPECT_TRUE (fn.vectorized);

    int preheaderInserts = 0;
    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::vinsertF)
            ++preheaderInserts;

    int loopInserts = 0;
    for (const auto& inst : fn.blocks[2].insts)
        if (inst.op == YdspIrOp::vinsertF)
            ++loopInserts;

    EXPECT_EQ (4, preheaderInserts);
    EXPECT_EQ (0, loopInserts);
}

//==============================================================================

TEST_F (YdspSlpVectorizerTests, SkipsPureLoadStoreCopyWhenScalarUsesExistInSameBlock)
{
    YdspIrFunction fn;
    fn.float32ArrayElements = 8;
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 0, 4, {} });
    fn.stateArrays.push_back ({ YdspValueType::float32Type, 4, 4, {} });
    fn.blocks.resize (1);

    const int gain = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 0.5);
    std::vector<int> idxA (4), idxB (4);
    for (int k = 0; k < 4; ++k)
    {
        idxA[static_cast<size_t> (k)] = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, k);
        idxB[static_cast<size_t> (k)] = emitInst (fn, 0, YdspIrOp::constI, newValue (fn, YdspValueType::int32Type), -1, -1, -1, -1, 0.0, k);
    }

    // Scalar reads of array 0 (base 0) and array 1 (base 4) along a serial dependency chain.
    int acc = emitInst (fn, 0, YdspIrOp::constF, newValue (fn, YdspValueType::float32Type), -1, -1, -1, -1, 1.0);
    for (int k = 0; k < 4; ++k)
    {
        const auto ku = static_cast<size_t> (k);
        const int rA = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idxA[ku], -1, -1, 0);
        const int rB = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idxB[ku], -1, -1, 4);
        const int m1 = emitInst (fn, 0, YdspIrOp::mulF, newValue (fn, YdspValueType::float32Type), rA, gain);
        const int s1 = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), acc, m1);
        acc = emitInst (fn, 0, YdspIrOp::addF, newValue (fn, YdspValueType::float32Type), s1, rB);
    }

    // Pure contiguous copy: b[0..3] = a[0..3] (no arithmetic in the store tree).
    for (int k = 0; k < 4; ++k)
    {
        const auto ku = static_cast<size_t> (k);
        const int rA = emitInst (fn, 0, YdspIrOp::loadStateArrayF, newValue (fn, YdspValueType::float32Type), idxA[ku], -1, -1, 0);
        emitInst (fn, 0, YdspIrOp::storeStateArrayF, -1, idxB[ku], rA, -1, 4);
    }

    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorWidth (4);

    EXPECT_FALSE (optimizer.slpVectorize (fn));
    EXPECT_FALSE (fn.vectorized);
}
