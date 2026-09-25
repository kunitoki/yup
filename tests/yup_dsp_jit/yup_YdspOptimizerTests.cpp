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
#include <limits>
#include <memory>

using namespace yup;

namespace
{

std::unique_ptr<YdspIrProgram> buildIr (StringRef source, YdspDiagnostics& diagnostics, bool fastMath)
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
    optimizer.setFastMath (fastMath);

    return optimizer.build (*analyzed);
}

std::unique_ptr<YdspIrProgram> buildIr (StringRef source, YdspDiagnostics& diagnostics)
{
    return buildIr (source, diagnostics, false);
}

std::unique_ptr<YdspIrProgram> buildIrWithLoopTransforms (StringRef source, YdspDiagnostics& diagnostics)
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

    // Host-like tier: vectorisation at four lanes plus unrolling, which is the
    // configuration under which loop fusion is enabled.
    YdspOptimizer optimizer (diagnostics);
    optimizer.setVectorizationEnabled (true);
    optimizer.setVectorWidth (4);
    optimizer.setUnrollingEnabled (true);

    return optimizer.build (*analyzed);
}

int countInst (const YdspIrFunction& fn, YdspIrOp op)
{
    int count = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op)
                ++count;

    return count;
}

bool hasConstF (const YdspIrFunction& fn, double value)
{
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::constF && inst.fvalue == value)
                return true;

    return false;
}

bool hasConstI (const YdspIrFunction& fn, int64_t value)
{
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::constI && inst.ivalue == value)
                return true;

    return false;
}

} // namespace

//==============================================================================
// IR building (the optimizer's default pipeline)
//==============================================================================

TEST (YdspOptimizerTests, BuildsSampleLoopIR)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];
    EXPECT_TRUE (fn.isSampleMode);
    EXPECT_EQ (1, fn.numInputs);
    EXPECT_EQ (1, fn.numOutputs);
    ASSERT_EQ (1u, fn.loops.size());
    EXPECT_EQ (YdspLoopBoundKind::blockSize, fn.loops[0].bound.kind);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::storeOutput));
}

TEST (YdspOptimizerTests, BuildsBlockModeLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 2; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];
    EXPECT_FALSE (fn.isSampleMode);
    ASSERT_EQ (1u, fn.loops.size());
    EXPECT_EQ (YdspLoopBoundKind::blockSize, fn.loops[0].bound.kind);
}

//==============================================================================
// Constant folding
//==============================================================================

TEST (YdspOptimizerTests, ConstantFoldsArithmetic)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in + (2.0 + 3.0); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (1, countInst (fn, YdspIrOp::addF));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
    EXPECT_TRUE (hasConstF (fn, 5.0));
}

TEST (YdspOptimizerTests, ConstantFoldsArithmeticInsideSampleLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in + (2.0 * 3.0); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addF));
    EXPECT_TRUE (hasConstF (fn, 6.0));
}

TEST (YdspOptimizerTests, ConstantFoldingSaturatesInt64MinDividedByMinusOne)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int64 a = -9223372036854775808;
                int64 b = -1;
                out = in + float (a / b);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::divI));
    EXPECT_TRUE (hasConstF (fn, static_cast<float> (std::numeric_limits<int64_t>::max())));
}

TEST (YdspOptimizerTests, ConstantFoldingDoesNotShiftInt64ByAnOutOfRangeAmount)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int64 a = 1;
                int64 b = 100;
                out = in + float (a << b);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
}

TEST (YdspOptimizerTests, ConstantFoldingSaturatesSourceInt64Addition)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                int64 half = 0x4000000000000000;
                out = in + float (half + half);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    bool foundSaturated = hasConstF (fn, static_cast<double> (std::numeric_limits<int64_t>::max()));

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::constI && inst.ivalue == std::numeric_limits<int64_t>::max())
                foundSaturated = true;

    EXPECT_TRUE (foundSaturated)
        << "constI=" << countInst (fn, YdspIrOp::constI)
        << " constF=" << countInst (fn, YdspIrOp::constF)
        << " addI=" << countInst (fn, YdspIrOp::addI)
        << " itof=" << countInst (fn, YdspIrOp::itof);
}

TEST (YdspOptimizerTests, ConstantFoldingKeepsSampleLoopInductionLive)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // The induction add stays (constant folding must not turn the loop counter
    // into a constant), and its write-back move has folded into the add, which
    // now updates the loop-carried register in place.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::ltI));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addI));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movI));
}

TEST (YdspOptimizerTests, ConstantFoldingKeepsBoundedLoopInductionLive)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..16 { out[i] = in[i]; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // The loop condition and two stream bounds guards must remain.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::ltI));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::ltUI));
    EXPECT_TRUE (fn.hasGuardedControlFlow);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addI));
    // The constant-bound loop keeps its canonical seed and write-back moves
    // (fusion, unrolling and the vectoriser pattern-match on them), so both
    // movIs survive the write-back fold.
    EXPECT_EQ (2, countInst (fn, YdspIrOp::movI));
}

//==============================================================================
// if-conversion
//==============================================================================

TEST (YdspOptimizerTests, IfConversionSpeculatesASampleHoldInputLoad)
{
    YdspDiagnostics diagnostics;

    // The 12:1 sample-hold shape: `if (counter == 0) { held = in; }` reads the
    // input stream inside the branch. The load reads the sample loop's own
    // induction, so it is provably in-bounds and read-only - if-conversion may
    // load it unconditionally and select, turning the periodic branch (which
    // mispredicts every 12th sample) into straight-line code. The counter wrap
    // was already a select; after the change the hold must be one too.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float held;
            state int counter;
            process {
                if (counter == 0) { held = in; }
                counter = counter + 1;
                if (counter >= 12) { counter = 0; }
                out = held;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    // Only the sample loop header still branches: the hold and the wrap are
    // both straight-line selects now.
    int branchIfCount = 0;

    for (const auto& block : fn.blocks)
        if (block.term == YdspIrTerm::branchIf)
            ++branchIfCount;

    EXPECT_EQ (1, branchIfCount);
    EXPECT_GE (countInst (fn, YdspIrOp::selectB), 2);
}

TEST (YdspOptimizerTests, RematerializesConstantsBetweenCallsAndAfterTheLastCall)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            process { out = pow (10.0, log10 (in) * 0.05) + 0.25; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    bool sawLog = false;
    bool sawPow = false;
    int middleConstants = 0;
    int tailConstants = 0;
    for (const auto& block : ir->kernels[0]->blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::log10F)
                sawLog = true;
            if (inst.op == YdspIrOp::powF)
                sawPow = true;
            if (inst.op == YdspIrOp::constF && (inst.fvalue == 10.0 || inst.fvalue == 0.05))
            {
                EXPECT_TRUE (sawLog);
                EXPECT_FALSE (sawPow);
                ++middleConstants;
            }
            if (inst.op == YdspIrOp::constF && inst.fvalue == 0.25)
            {
                EXPECT_TRUE (sawPow);
                ++tailConstants;
            }
        }
    EXPECT_EQ (2, middleConstants);
    EXPECT_EQ (1, tailConstants);
}

TEST (YdspOptimizerTests, KeepsConstantsWhoseUsesSpanMultipleCallRegions)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            process { out = pow (10.0, log10 (in + 0.25) * 0.25) + 0.25; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());
    int constants = 0;
    for (size_t b = 0; b < ir->kernels[0]->blocks.size(); ++b)
        for (const auto& inst : ir->kernels[0]->blocks[b].insts)
            if (inst.op == YdspIrOp::constF && inst.fvalue == 0.25)
            {
                EXPECT_EQ (0u, b);
                ++constants;
            }
    EXPECT_EQ (1, constants);
}

TEST (YdspOptimizerTests, RematerializesDryCoefficientAfterCall)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float wet = 0.25;
            process { let shaped = sin (in); out = (1.0 - wet) * in + wet * shaped; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    bool sawCall = false;
    int differences = 0;
    for (const auto& block : ir->kernels[0]->blocks)
        for (const auto& inst : block.insts)
        {
            sawCall |= inst.op == YdspIrOp::sinF;
            if (inst.op == YdspIrOp::subF)
            {
                EXPECT_TRUE (sawCall);
                ++differences;
            }
        }
    EXPECT_EQ (1, differences);
}

TEST (YdspOptimizerTests, RematerializesPostCallParameterUse)
{
    YdspDiagnostics diagnostics;

    // The `g` parameter is loaded before the sample loop but only multiplied
    // after the tanh call, so it crosses the per-sample libm call and gets
    // parked around it. The remat pass re-defines it right after the call:
    // one per-sample reload, no parking, and the pre-loop load disappears.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float g = 2.0;
            process { out = tanh (in) * g; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    // Exactly one parameter load remains, and it sits in the sample loop body
    // after the tanh call instead of before the loop.
    int paramLoads = 0;
    bool loadAfterCall = false;

    for (const auto& block : fn.blocks)
    {
        int callIndex = -1;
        int loadIndex = -1;

        for (size_t i = 0; i < block.insts.size(); ++i)
        {
            if (block.insts[i].op == YdspIrOp::tanhF)
                callIndex = static_cast<int> (i);

            if (block.insts[i].op == YdspIrOp::loadParam)
            {
                ++paramLoads;
                loadIndex = static_cast<int> (i);
            }
        }

        if (callIndex >= 0 && loadIndex > callIndex)
            loadAfterCall = true;
    }

    EXPECT_EQ (1, paramLoads);
    EXPECT_TRUE (loadAfterCall);
}

//==============================================================================
// Loop fusion
//==============================================================================

TEST (YdspOptimizerTests, LoopFusionMergesAdjacentDisjointLoops)
{
    // Two consecutive 8-iteration loops over disjoint arrays fuse into one
    // loop (the host tier enables fusion via vectorization/unrolling).
    YdspDiagnostics diagnostics;

    auto ir = buildIrWithLoopTransforms (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process block {
                let sample = in[0];
                for i in 0..8 { z[i] = z[i] * 0.5 + sample; }
                for j in 0..8 { w[j] = w[j] * 0.5 + 0.02; }
                out[0] = z[0] + w[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (1u, fn.loops.size()) << "the two loops should have fused into one";
}

TEST (YdspOptimizerTests, LoopFusionSkipsLoopsSharingMemory)
{
    // Loop 2 reads the array loop 1 writes, so fusing would change which
    // values it sees; the pass must leave them apart.
    YdspDiagnostics diagnostics;

    auto ir = buildIrWithLoopTransforms (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process block {
                let sample = in[0];
                for i in 0..8 { z[i] = z[i] * 0.5 + sample; }
                for j in 0..8 { w[j] = w[j] * 0.5 + z[j]; }
                out[0] = z[0] + w[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (2u, fn.loops.size()) << "a read-after-write dependency must block fusion";
}

TEST (YdspOptimizerTests, LoopFusionExitsIntoItsRecordedExitBlock)
{
    // The fused loop's terminator must jump to the block the loop record names
    // as its exit. The old code retargeted the header to e2's *post-remap*
    // index and then remapped it a second time, aiming the fused loop at the
    // block before its own header.
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process block {
                let sample = in[0];
                for i in 0..8 { z[i] = z[i] * 0.5 + sample; }
                for j in 0..8 { w[j] = w[j] * 0.5 + 0.02; }
                out[0] = z[0] + w[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    // Run only the fusion pass (the default pipeline leaves loop fusion off),
    // exactly as the loop-transform tier would before vectorizing/unrolling.
    YdspIrFunction fn = *ir->kernels[0];
    YdspOptimizer optimizer (diagnostics);
    optimizer.loopFusion (fn);

    ASSERT_EQ (1u, fn.loops.size()) << "the two loops should have fused into one";

    const auto& fusedLoop = fn.loops[0];
    ASSERT_GE (fusedLoop.headerBlock, 0);
    ASSERT_LT (static_cast<size_t> (fusedLoop.headerBlock), fn.blocks.size());
    ASSERT_LT (static_cast<size_t> (fusedLoop.exitBlock), fn.blocks.size());

    const auto& header = fn.blocks[static_cast<size_t> (fusedLoop.headerBlock)];

    EXPECT_EQ (YdspIrTerm::branchIf, header.term);
    EXPECT_EQ (fusedLoop.exitBlock, header.termTarget2)
        << "the fused loop's terminator must target the block its loop record names as the exit";

    // Every remaining terminator target must name a block that still exists.
    for (int blockIndex = 0; blockIndex < static_cast<int> (fn.blocks.size()); ++blockIndex)
    {
        const auto& block = fn.blocks[static_cast<size_t> (blockIndex)];

        if (block.termTarget >= static_cast<int> (fn.blocks.size()))
            FAIL() << "dangling termTarget " << block.termTarget << " in block " << blockIndex;

        if (block.termTarget2 >= static_cast<int> (fn.blocks.size()))
            FAIL() << "dangling termTarget2 " << block.termTarget2 << " in block " << blockIndex;
    }
}

TEST (YdspOptimizerTests, LoopFusionSkipsALoopWhoseInductionDoesNotStartAtZero)
{
    // The second loop runs j over 3..8: its `j = 3` init must survive, so the
    // loops cannot fuse. The old code treated any movI into the second
    // induction as a droppable "init from literal 0" and silently restarted it
    // at the first loop's value.
    YdspDiagnostics diagnostics;

    auto ir = buildIrWithLoopTransforms (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process block {
                for i in 0..8 { z[i] = z[i] * 0.5 + in[i]; }
                for j in 3..8 { w[j] = w[j] * 0.5 + 0.02; }
                out[0] = z[0] + w[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (2u, fn.loops.size()) << "a non-zero second start must block fusion";
}

//==============================================================================
// Algebraic simplification
//==============================================================================

TEST (YdspOptimizerTests, AlgebraicSimplifiesMulByOne)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in * 1.0; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));
}

TEST (YdspOptimizerTests, AlgebraicSimplifiesMulByZero)
{
    YdspDiagnostics diagnostics;

    // The x * 0.0 -> 0.0 identity is fast-math only (it is wrong for signed
    // zero, NaN and infinity); strict-mode coverage lives in the graph tests.
    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in * 0; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics, true);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::loadInput));
}

TEST (YdspOptimizerTests, ReciprocalMultiplyReplacesDivisionByConstantOnlyUnderFastMath)
{
    // x / c -> x * (1 / c) removes the division but changes rounding, so it is
    // gated on fastMath and must leave the strict build untouched.
    constexpr auto source = R"YDSP(
        processor P { input stream in; output stream out; process { out = in / 0.25; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    YdspDiagnostics strictDiagnostics;
    auto strict = buildIr (source, strictDiagnostics, false);
    ASSERT_FALSE (strictDiagnostics.hasErrors()) << strictDiagnostics.toString();
    ASSERT_NE (nullptr, strict);
    EXPECT_EQ (1, countInst (*strict->kernels[0], YdspIrOp::divF));

    YdspDiagnostics fastDiagnostics;
    auto fast = buildIr (source, fastDiagnostics, true);
    ASSERT_FALSE (fastDiagnostics.hasErrors()) << fastDiagnostics.toString();
    ASSERT_NE (nullptr, fast);

    const auto& fn = *fast->kernels[0];
    EXPECT_EQ (0, countInst (fn, YdspIrOp::divF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::mulF));
    EXPECT_TRUE (hasConstF (fn, 4.0));
}

TEST (YdspOptimizerTests, PowSquaredBecomesAMultiplyOnlyUnderFastMath)
{
    // pow (x, 2.0) -> x * x is a rounding change (libm pow rounds once with
    // its own algorithm), so it is fast-math only; strict keeps the intrinsic.
    YdspDiagnostics diagnostics;

    auto strict = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = pow (in, 2.0); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                           diagnostics, false);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, strict);
    EXPECT_EQ (1, countInst (*strict->kernels[0], YdspIrOp::powF));
    EXPECT_EQ (0, countInst (*strict->kernels[0], YdspIrOp::mulF));

    YdspDiagnostics fastDiagnostics;

    auto fast = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = pow (in, 2.0); } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                         fastDiagnostics, true);

    ASSERT_FALSE (fastDiagnostics.hasErrors()) << fastDiagnostics.toString();
    ASSERT_NE (nullptr, fast);
    EXPECT_EQ (0, countInst (*fast->kernels[0], YdspIrOp::powF));
    EXPECT_EQ (1, countInst (*fast->kernels[0], YdspIrOp::mulF));
}

TEST (YdspOptimizerTests, Pow2ModuloOfMaskedValueBecomesAMask)
{
    // i is masked to 0..15 before the modulo, so `i % 8` is provably
    // non-negative and rewrites to `i & 7` without changing the result - in
    // every tier, not just fastMath.
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                let i = int32 (in * 127.0) & 15;
                out = float (i % 8) * 0.001;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::modI));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::andI)); // the source `& 15` plus the rewritten `& 7`
    EXPECT_TRUE (hasConstI (fn, 7));
}

TEST (YdspOptimizerTests, DoesNotSimplifyAgainstAnOverwrittenLiteral)
{
    // `t` is bound to the literal 0 by its declaration and then reassigned, all
    // in one block. Treating the literal as `t`'s value would fold `t + 1.0` to
    // a constant and drop the input entirely - the IR is not SSA, so a constant
    // writing a register does not mean the register holds it at a later use.
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { float t = 0.0; t = in * 2.0; out = t + 1.0; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // The multiply and the add both have to survive, and the input still has to
    // be read: the output depends on it.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addF));
}

//==============================================================================
// Copy propagation
//==============================================================================

TEST (YdspOptimizerTests, ReusesIdenticalPureExpressions)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { let x = in; let k = 2.0; out = x * k + x * k; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (1, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addF));
}

TEST (YdspOptimizerTests, CopyPropagatesFunctionParameterCopies)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            func doubleIt(x: float) : float { return x * 2.0; }
            process { out = doubleIt(in); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::movF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));

    int loadResult = -1;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::loadInput)
                loadResult = inst.result;

    bool propagatesIntoMul = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::mulF)
                propagatesIntoMul = (inst.a == loadResult);

    EXPECT_TRUE (propagatesIntoMul);
}

//==============================================================================
// Dead code elimination
//==============================================================================

TEST (YdspOptimizerTests, DeadCodeEliminationRemovesUnusedComputation)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float32 dead = in * 2.0;
                out = in;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::storeOutput));
}

//==============================================================================
// Loop-invariant code motion
//==============================================================================

TEST (YdspOptimizerTests, HoistsInvariantParamComputation)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float drive = 0.5;
            process block {
                for i in 0..blockSize { out[i] = in[i] * (1 - drive); }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    // (1 - drive) is loop-invariant and must be hoisted into the entry block.
    bool sawHoistedSubF = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::subF)
            sawHoistedSubF = true;

    EXPECT_TRUE (sawHoistedSubF);
}

TEST (YdspOptimizerTests, GuardsPreserveConditionalWorkWithoutBlockingUnconditionalHoisting)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float attack = 0.005, release = 0.15;
            input parameter int index = 0;
            state float data[8];
            process {
                let attackCoeff = exp (-1.0 / (attack * sampleRate));
                let releaseCoeff = exp (-1.0 / (release * sampleRate));
                let selected = in > 0.0 ? exp (attack) : releaseCoeff;
                float polarity = 1.0;
                if (in < 0.0) { polarity = -1.0; }
                data[index] = in;
                out = polarity * selected * attackCoeff;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    ASSERT_TRUE (fn.hasGuardedControlFlow);
    EXPECT_EQ (3, countInst (fn, YdspIrOp::expF));
    EXPECT_EQ (2, std::count_if (fn.blocks[0].insts.begin(), fn.blocks[0].insts.end(), [] (const YdspIrInst& inst)
    {
        return inst.op == YdspIrOp::expF;
    }));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::selectB));
    EXPECT_EQ (0, std::count_if (fn.blocks[0].insts.begin(), fn.blocks[0].insts.end(), [] (const YdspIrInst& inst)
    {
        return inst.op == YdspIrOp::loadStateArrayF;
    }));
    EXPECT_EQ (2, std::count_if (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.preservesConditionalEvaluation && block.term == YdspIrTerm::branchIf;
    }));
}

TEST (YdspOptimizerTests, HoistsIterationLocalClampAndConversionChain)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float delay = 3.5;
            process {
                float d = delay;
                if (d > 8.0) d = 8.0;
                if (d < 1.0) d = 1.0;
                let whole = int32 (d);
                out = in * (d - float (whole));
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    int entrySelects = 0;
    int entryConversions = 0;
    for (const auto& inst : fn.blocks[0].insts)
    {
        entrySelects += inst.op == YdspIrOp::selectB;
        entryConversions += inst.op == YdspIrOp::ftoi;
    }
    EXPECT_EQ (2, entrySelects);
    EXPECT_EQ (1, entryConversions);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::ftoi));
}

TEST (YdspOptimizerTests, ProvesPersistentRingCounterAndWrappedClampedTaps)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            input parameter float delay = 3.5;
            state float data[8]; state int wp;
            process {
                float d = delay;
                if (d > 8.0) { d = 8.0; }
                if (d < 1.0) { d = 1.0; }
                int count = int (d);
                int a = wp - count;
                if (a < 0) { a = a + 8; }
                int b = a - 1;
                if (b < 0) { b = b + 8; }
                out = data[a] + data[b];
                data[wp] = in;
                wp = wp + 1;
                if (wp >= 8) { wp = 0; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    EXPECT_EQ (0, countInst (*ir->kernels[0], YdspIrOp::ltUI));
    EXPECT_EQ (2, countInst (*ir->kernels[0], YdspIrOp::loadStateArrayF));
    EXPECT_EQ (1, countInst (*ir->kernels[0], YdspIrOp::storeStateArrayF));
    for (const auto& block : ir->kernels[0]->blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::addI || inst.op == YdspIrOp::subI || inst.op == YdspIrOp::mulI)
                EXPECT_FALSE (inst.saturatingIntegerArithmetic);
}

TEST (YdspOptimizerTests, RingProofAuditsInitializationEventsAndEveryUpdate)
{
    for (const auto* change : { "init { wp = 3; }", "init { wp = 8; }",
                               "event midi (e: noteOn) { wp = 0; }", "event midi (e: noteOn) { wp = int (e.pitch); }" })
    {
        const bool event = String (change).startsWith ("event");
        YdspDiagnostics diagnostics;
        const auto source = String (R"YDSP(
            processor P {
                input stream in; output stream out; EVENT_INPUT
                state float data[8]; state int wp;
                CHANGE
                process {
                    out = data[wp]; data[wp] = in;
                    wp = wp + 1; if (wp >= 8) { wp = 0; }
                }
            }
            graph G { input stream x; output stream y; EVENT_INPUT
                node p = P; connection { x -> p.in; p.out -> y; EVENT_EDGE } }
        )YDSP").replace ("CHANGE", change)
                           .replace ("EVENT_INPUT", event ? "input event midi;" : "")
                           .replace ("EVENT_EDGE", event ? "midi -> p.midi;" : "");
        auto ir = buildIr (source, diagnostics);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        const bool safe = String (change).contains ("wp = 3") || String (change).contains ("wp = 0");
        EXPECT_EQ (safe, countInst (*ir->kernels[0], YdspIrOp::ltUI) == 0) << change;
    }
    for (const auto* extra : { "wp = wp + 1;", "wp = -1;", "wp = int (in);" })
    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (String (R"YDSP(
            processor P {
                input stream in; output stream out;
                state float data[8]; state int wp;
                process {
                    EXTRA
                    out = data[wp]; data[wp] = in;
                    wp = wp + 1; if (wp >= 8) { wp = 0; }
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP").replace ("EXTRA", extra), diagnostics);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        const auto& fn = *ir->kernels[0];
        if (String (extra) == "wp = -1;")
        {
            // Constant folding replaces the comparison, but does not require
            // removal of the safe fallback load or the guarded store block.
            bool hasRejectedAccess = false;
            for (const auto& block : fn.blocks)
                for (const auto& inst : block.insts)
                    hasRejectedAccess |= inst.op == YdspIrOp::constB && ! inst.bvalue;
            EXPECT_TRUE (hasRejectedAccess);
            EXPECT_EQ (1, countInst (fn, YdspIrOp::storeOutput));
            EXPECT_TRUE (hasConstF (fn, 0.0));
        }
        else
            EXPECT_GT (countInst (fn, YdspIrOp::ltUI), 0) << extra;
    }
}

TEST (YdspOptimizerTests, RingProofRetainsChecksForIncompleteTapBounds)
{
    for (const auto* tap : {
             "int a = wp - int (delay); if (a < 0) { a = a + 8; }",
             "float d = delay; if (d > 9.0) { d = 9.0; } if (d < 1.0) { d = 1.0; } int a = wp - int (d); if (a < 0) { a = a + 8; }",
             "int a = wp - 1;",
             "int a = wp + 1;" })
    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (String (R"YDSP(
            processor P {
                input stream in; output stream out;
                input parameter float delay = 3.5;
                state float data[8]; state int wp;
                process {
                    TAP
                    out = data[a]; data[wp] = in;
                    wp = wp + 1; if (wp >= 8) { wp = 0; }
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP").replace ("TAP", tap), diagnostics);
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        EXPECT_GT (countInst (*ir->kernels[0], YdspIrOp::ltUI), 0) << tap;
    }
}

TEST (YdspOptimizerTests, RemovesBoundsChecksForProvenIntegerRanges)
{
    for (const auto* expression : { "raw & 7", "clamp (raw, 0, 7)", "min (max (raw, 0), 7)",
                                   "(raw & 3) + 4", "(raw & 3) - 0", "select (raw < 0, int32 (0), int32 (7))" })
    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (String (R"YDSP(
            processor P {
                input stream in; output stream out;
                state float data[8];
                process {
                    let raw = int32 (in);
                    let index = EXPRESSION;
                    out = data[index];
                    data[index] = in;
                }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP").replace ("EXPRESSION", expression), diagnostics);
        ASSERT_FALSE (diagnostics.hasErrors()) << expression << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        EXPECT_EQ (0, countInst (*ir->kernels[0], YdspIrOp::ltUI)) << expression;
    }
}

TEST (YdspOptimizerTests, RetainsBoundsChecksForNegativeAndOversizedRanges)
{
    for (const auto* expression : { "raw & 15", "clamp (raw, -1, 7)", "(raw & 7) - 1", "raw + 1" })
    {
        YdspDiagnostics diagnostics;
        auto ir = buildIr (String (R"YDSP(
            processor P {
                input stream in; output stream out;
                state float data[8];
                process { let raw = int32 (in); out = data[EXPRESSION]; }
            }
            graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
        )YDSP").replace ("EXPRESSION", expression), diagnostics);
        ASSERT_FALSE (diagnostics.hasErrors()) << expression << diagnostics.toString();
        ASSERT_NE (nullptr, ir);
        EXPECT_GT (countInst (*ir->kernels[0], YdspIrOp::ltUI), 0) << expression;
    }
}

TEST (YdspOptimizerTests, CheckedArrayReadsSelectASafeIndexBeforeLoading)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            state float data[8];
            process { out = data[int32 (in)]; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (1, countInst (fn, YdspIrOp::ltUI));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::selectB));
    EXPECT_EQ (1, std::count_if (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.term == YdspIrTerm::branchIf;
    }));
    for (const auto& block : fn.blocks)
        for (size_t i = 0; i < block.insts.size(); ++i)
            if (block.insts[i].op == YdspIrOp::loadStateArrayF)
            {
                const auto safeIndex = block.insts[i].a;
                EXPECT_TRUE (std::any_of (block.insts.begin(), block.insts.begin() + static_cast<std::ptrdiff_t> (i), [safeIndex] (const YdspIrInst& inst)
                {
                    return inst.op == YdspIrOp::selectB && inst.result == safeIndex;
                }));
            }
}

TEST (YdspOptimizerTests, HoistsClampAndConversionBeforeAnArrayGuard)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float delay = 3.5;
            state float data[8];
            process {
                float d = delay;
                if (d > 8.0) d = 8.0;
                if (d < 1.0) d = 1.0;
                let whole = int32 (d);
                data[int32 (in)] = in;
                out = in * (d - float (whole));
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (1, countInst (fn, YdspIrOp::ltUI));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::andB));
    int entrySelects = 0;
    int entryConversions = 0;
    for (const auto& inst : fn.blocks[0].insts)
    {
        entrySelects += inst.op == YdspIrOp::selectB;
        entryConversions += inst.op == YdspIrOp::ftoi;
    }
    EXPECT_EQ (2, entrySelects);
    EXPECT_EQ (1, entryConversions);
    EXPECT_EQ (2, countInst (fn, YdspIrOp::ftoi));
}

TEST (YdspOptimizerTests, LoopInvariantCodeMotionKeepsInputLoadsInLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float drive = 0.5;
            process block {
                for i in 0..blockSize { out[i] = in[i] * (1 - drive); }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    // The input load depends on the sample index and must not leave the loop.
    bool sawLoadInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::loadInput)
            sawLoadInEntry = true;

    EXPECT_FALSE (sawLoadInEntry);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadInput));
}

TEST (YdspOptimizerTests, LoopInvariantCodeMotionKeepsInductionUpdateInLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..16 { out[i] = in[i]; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    bool sawIncrementInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::addI)
            sawIncrementInEntry = true;

    EXPECT_FALSE (sawIncrementInEntry);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addI));
}

TEST (YdspOptimizerTests, LoopInvariantCodeMotionKeepsSampleInductionInLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    bool sawIncrementInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::addI)
            sawIncrementInEntry = true;

    EXPECT_FALSE (sawIncrementInEntry);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::addI));
    // The sample-loop induction write-back folded into the increment during
    // the build; LICM must leave that in-place increment in the loop.
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movI));
}

TEST (YdspOptimizerTests, LoopInvariantCodeMotionKeepsPathDependentValuesInLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[64];
            process block {
                for i in 0..blockSize {
                    float32 v;
                    // A state-array load inside the arm keeps this diamond
                    // branchy (if-conversion only speculates pure values and
                    // in-bounds input-stream reads, never state-array loads);
                    // v's definitions stay path-dependent in the loop.
                    if (in[i] > 0.5) v = z[i]; else v = 3.0;
                    out[i] = v;
                }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    bool sawCopyInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::movF)
            sawCopyInEntry = true;

    EXPECT_FALSE (sawCopyInEntry);
    // The array arm retains its copy; the other arm becomes a constant.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::movF));
    EXPECT_EQ (0, std::count_if (fn.blocks[0].insts.begin(), fn.blocks[0].insts.end(), [] (const YdspIrInst& inst)
    {
        return inst.op == YdspIrOp::loadStateArrayF;
    }));
}

TEST (YdspOptimizerTests, LoopInvariantCodeMotionKeepsIfConvertedSelectInLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    float32 v;
                    // Both arms move pure constants, so if-conversion fuses
                    // the diamond into one selectB; that select depends on the
                    // per-sample input test and must stay in the loop.
                    if (in[i] > 0.5) v = 2.0; else v = 3.0;
                    out[i] = v;
                }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors());
    ASSERT_NE (nullptr, ir);

    auto& fn = *ir->kernels[0];

    YdspOptimizer optimizer (diagnostics);
    optimizer.loopInvariantCodeMotion (fn);

    bool sawSelectInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::selectB)
            sawSelectInEntry = true;

    EXPECT_FALSE (sawSelectInEntry);
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::selectB));
}

//==============================================================================
// State lowering and execution reports
//==============================================================================

TEST (YdspOptimizerTests, LowersPrevIntoHiddenState)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z;
            process { out = z'; z = in; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (2, fn.float32Scalars);
    EXPECT_EQ (0, fn.float64Scalars);
}

TEST (YdspOptimizerTests, LowersDelayIntoRingBuffer)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in @ 3; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (4, fn.float32ArrayElements);
    EXPECT_EQ (1, fn.int32Scalars);
}

TEST (YdspOptimizerTests, SharesUnconditionalInputDelayHistories)
{
    YdspDiagnostics diagnostics;
    const auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out;
            process { out = (in @ 127) + (in @ 509) + (in @ 251); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (512, fn.float32ArrayElements);
    EXPECT_EQ (1, fn.int32Scalars);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::storeStateArrayF));
    EXPECT_EQ (3, countInst (fn, YdspIrOp::loadStateArrayF));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::advanceWrapI));
}

TEST (YdspOptimizerTests, SharedInputDelaysKeepStreamsSeparateAndAcceptTheMaximumDelay)
{
    YdspDiagnostics diagnostics;
    const auto ir = buildIr (R"YDSP(
        processor P {
            input stream left; input stream right; output stream out;
            process {
                out = (left @ 1) + (left @ 65536) + (right @ 1) + (right @ 3);
            }
        }
        graph G {
            input stream x; input stream y; output stream z; node p = P;
            connection { x -> p.left; y -> p.right; p.out -> z; }
        }
    )YDSP", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (131072 + 4, fn.float32ArrayElements);
    EXPECT_EQ (2, fn.int32Scalars);
    EXPECT_EQ (2, countInst (fn, YdspIrOp::storeStateArrayF));
    EXPECT_EQ (4, countInst (fn, YdspIrOp::loadStateArrayF));
}

TEST (YdspOptimizerTests, KeepsConditionalAndMutableDelayHistoriesSeparate)
{
    YdspDiagnostics diagnostics;
    const auto ir = buildIr (R"YDSP(
        processor P {
            input stream in; output stream out; state float z;
            process {
                let a = z @ 3;
                z = in;
                let b = z @ 7;
                float c = 0.0;
                if (in > 0.0) { c = in @ 3; }
                out = a + b + c + (in @ 7);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (24, fn.float32ArrayElements);
    EXPECT_EQ (4, fn.int32Scalars);
    EXPECT_EQ (4, countInst (fn, YdspIrOp::storeStateArrayF));
}

TEST (YdspOptimizerTests, BuildsExecutionReport)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor Taps {
            input stream in;
            output stream out;
            process block {
                for i in 0..16 { out[i] = in[i]; }
            }
        }
        graph G { input stream x; output stream y; node t = Taps; connection { x -> t.in; t.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    YdspExecutionReport report;
    YdspOptimizer::buildReport (*ir, report);

    ASSERT_EQ (1u, report.getKernels().size());
    const auto& kernel = report.getKernels()[0];
    EXPECT_EQ ("Taps", kernel.name);
    EXPECT_GT (kernel.instructionCount, 0);
    EXPECT_EQ (16, kernel.boundedIterationCount);
    EXPECT_TRUE (kernel.provenRealtimeSafe);
    ASSERT_EQ (1u, kernel.loopBounds.size());
    EXPECT_EQ ("16", kernel.loopBounds[0]);
    EXPECT_TRUE (report.isProvenRealtimeSafe());
}

TEST (YdspOptimizerTests, RecordsEachNestedLoopsOwnBound)
{
    YdspDiagnostics diagnostics;

    // The outer loop's bound is resolved before its body is lowered, but only
    // recorded afterwards - so lowering the inner loop in between must not
    // clobber it. With the bound held in a builder member both loops reported
    // the inner bound, and the worst-case iteration count came out as 5 x 5.
    auto ir = buildIr (R"YDSP(
        processor Nested {
            input stream in;
            output stream out;
            state float acc;
            process block {
                for i in 0..7 {
                    for j in 0..5 { acc = acc + 1.0; }
                    out[i] = in[i] + acc;
                }
            }
        }
        graph G { input stream x; output stream y; node n = Nested; connection { x -> n.in; n.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    YdspExecutionReport report;
    YdspOptimizer::buildReport (*ir, report);

    ASSERT_EQ (1u, report.getKernels().size());
    const auto& kernel = report.getKernels()[0];

    ASSERT_EQ (2u, kernel.loopBounds.size());

    StringArray bounds (kernel.loopBounds);
    bounds.sort (false);

    EXPECT_EQ ("5", bounds[0]);
    EXPECT_EQ ("7", bounds[1]);
    EXPECT_EQ (35, kernel.boundedIterationCount);
}

TEST (YdspOptimizerTests, LowersDelayWrapWithoutAnIntegerDivision)
{
    YdspDiagnostics diagnostics;

    // The `@` ring wrap used to emit modI, which on x86-64 is a call into a
    // helper - once per delay tap per sample. The write pointer is provably
    // within [0, n], so the dedicated increment-and-wrap operation is exact.
    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in @ 8; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (1, countInst (fn, YdspIrOp::advanceWrapI));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::wrapI));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::modI));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::divI));
}

TEST (YdspOptimizerTests, ConvertsAShortElselessIfIntoSelects)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            process {
                float y = in * 3.0;

                if (y > 1.0) { y = 2.0 - y; }
                if (y < -1.0) { y = -2.0 - y; }

                out = y;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // Both folds become selects ...
    EXPECT_EQ (2, countInst (fn, YdspIrOp::selectB));

    const auto branches = std::count_if (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.term == YdspIrTerm::branchIf;
    });

    // ... leaving only the sample loop's own termination test, which is a real
    // loop back-edge rather than a diamond and is never a conversion candidate.
    EXPECT_EQ (1, branches);
}

TEST (YdspOptimizerTests, FusesATwoSidedIfOfMovesIntoASelect)
{
    YdspDiagnostics diagnostics;

    // if (cond) { y = a; } else { y = b; } - both arms a single move into the
    // same value - becomes one select, killing the branch (the wavefolder and
    // clipper shapes).
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float y = 0.0;
                if (in > 0.5) { y = -1.0; }
                else { y = 1.0; }
                out = y;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (1, countInst (fn, YdspIrOp::selectB)) << "both arms should fuse into a select";

    const auto branches = std::count_if (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.term == YdspIrTerm::branchIf;
    });

    // Only the sample loop's own termination test remains.
    EXPECT_EQ (1, branches);
}

TEST (YdspOptimizerTests, DoesNotIfConvertABodyThatTouchesMemory)
{
    YdspDiagnostics diagnostics;

    // The array read is guarded: running it unconditionally would index the
    // array with whatever `idx` holds when the guard is false.
    auto ir = buildIr (R"YDSP(
        let size = 4;

        processor P {
            input stream in;
            output stream out;

            input parameter float pick = 0.0;

            state float bank[size];

            process {
                let idx = int (pick);

                float y = in;

                if (idx < size) { y = bank[idx]; }

                out = y;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    const auto branches = std::count_if (fn.blocks.begin(), fn.blocks.end(), [] (const YdspIrBlock& block)
    {
        return block.term == YdspIrTerm::branchIf;
    });

    // The sample-loop header and source condition remain branchy.
    EXPECT_EQ (2, branches);
}

TEST (YdspOptimizerTests, HoistsInnerLoopInvariantWorkIntoItsOwnPreheader)
{
    YdspDiagnostics diagnostics;

    // `drive` does not depend on `i`, so it belongs in the inner loop's
    // preheader - but it *does* depend on `env`, which changes every sample, so
    // it must not climb any further than that. Hoisting it to the entry block
    // would freeze it at the value `env` had before the first sample.
    auto ir = buildIr (R"YDSP(
        let modes = 8;

        processor P {
            input stream in;
            output stream out;

            input parameter float damping = 0.5;

            state float z[modes];
            state float env;

            process {
                env = env * 0.999 + abs (in) * 0.001;

                float sum = 0.0;

                for i in 0..modes {
                    let drive = exp (-env * damping) * (1.0 - damping);

                    z[i] = z[i] * 0.9 + in * drive;
                    sum = sum + z[i];
                }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // The inner `for` is the constant-bound loop; the sample loop is bounded by
    // blockSize.
    const auto inner = std::find_if (fn.loops.begin(), fn.loops.end(), [] (const YdspIrLoop& loop)
    {
        return loop.bound.kind == YdspLoopBoundKind::constant;
    });

    ASSERT_NE (fn.loops.end(), inner);
    ASSERT_GT (inner->headerBlock, 0);

    const auto countIn = [&fn] (int block, YdspIrOp op)
    {
        const auto& insts = fn.blocks[static_cast<size_t> (block)].insts;

        return static_cast<int> (std::count_if (insts.begin(), insts.end(), [op] (const YdspIrInst& inst)
        {
            return inst.op == op;
        }));
    };

    // Evaluated once per sample in the preheader ...
    EXPECT_EQ (1, countIn (inner->headerBlock - 1, YdspIrOp::expF));

    // ... not once per mode inside the loop ...
    for (int block = inner->headerBlock; block < inner->exitBlock; ++block)
        EXPECT_EQ (0, countIn (block, YdspIrOp::expF)) << "block " << block;

    // ... and not lifted clear of the sample loop, where `env` would be stale.
    EXPECT_EQ (0, countIn (0, YdspIrOp::expF));
}

TEST (YdspOptimizerTests, ForwardsAStoredArrayElementToALaterLoad)
{
    YdspDiagnostics diagnostics;

    // `bank[i]` is written and read back in the same iteration, with a write to
    // a *different* array through the same index in between - which cannot
    // alias, so the read still forwards.
    auto ir = buildIr (R"YDSP(
        let size = 8;

        processor P {
            input stream in;
            output stream out;

            state float bank[size];
            state float other[size];

            process {
                float sum = 0.0;

                for i in 0..size {
                    bank[i] = bank[i] + in;
                    other[i] = in;
                    sum = sum + bank[i];
                }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // Two reads of bank[i] in the source, but the second is satisfied from the
    // value just stored, leaving one real load.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadStateArrayF));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::storeStateArrayF));
}

TEST (YdspOptimizerTests, DoesNotForwardAcrossAPossiblyAliasingStore)
{
    YdspDiagnostics diagnostics;

    // The intervening write uses a *different* index into the same array, so
    // `bank[j]` may be `bank[i]` at runtime and the read has to stay a load.
    auto ir = buildIr (R"YDSP(
        let size = 8;

        processor P {
            input stream in;
            output stream out;

            state float bank[size];

            process {
                float sum = 0.0;

                for i in 0..size {
                    let j = size - 1 - i;

                    bank[i] = in;
                    bank[j] = in * 2.0;
                    sum = sum + bank[i];
                }

                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    EXPECT_EQ (1, countInst (*ir->kernels[0], YdspIrOp::loadStateArrayF));
}

TEST (YdspOptimizerTests, ArrayCsePreservesLoadsAcrossDisjointStores)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type,
                             YdspValueType::int32Type, YdspValueType::int64Type })
        for (const int lanes : { 1, 4, 8 })
            for (const int offset : { -8, -1, 0, 1, 4, 8 })
                for (const int hazard : { 0, 1, 2, 3 })
                {
                    SCOPED_TRACE (::testing::Message() << "type=" << static_cast<int> (type)
                                                      << " lanes=" << lanes << " offset=" << offset << " hazard=" << hazard);
                    const bool integer = type == YdspValueType::int32Type || type == YdspValueType::int64Type;
                    const auto load = integer ? YdspIrOp::loadStateArrayI : YdspIrOp::loadStateArrayF;
                    const auto store = integer ? YdspIrOp::storeStateArrayI : YdspIrOp::storeStateArrayF;
                    YdspIrFunction fn;
                    fn.valueTypes = { YdspValueType::int32Type, type, type, type, YdspValueType::int32Type };
                    fn.valueLanes = { 1, lanes, lanes, lanes, 1 };
                    fn.blocks.resize (1);
                    auto& insts = fn.blocks[0].insts;
                    insts = {
                        { load, 1, 0, -1, -1, 16 },
                        { store, -1, hazard == 1 ? 4 : 0, 2, -1, 16 + offset }
                    };
                    if (hazard == 2)
                        insts.push_back ({ YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 1 });
                    if (hazard == 3)
                        fn.valueTypes[2] = integer ? YdspValueType::int64Type : YdspValueType::float64Type;
                    insts.push_back ({ load, 3, 0, -1, -1, 16 });
                    YdspDiagnostics diagnostics;
                    YdspOptimizer optimizer (diagnostics);
                    const bool sameType = fn.valueTypes[1] == fn.valueTypes[2];
                    const bool expected = hazard != 1 && hazard != 2 && sameType
                                       && (offset >= lanes || offset + lanes <= 0);
                    EXPECT_EQ (expected, optimizer.commonSubexpressionElimination (fn));
                    EXPECT_EQ (expected ? (integer ? YdspIrOp::movI : YdspIrOp::movF) : load, insts.back().op);
                    if (expected)
                        EXPECT_EQ (1, insts.back().a);
                }
}

TEST (YdspOptimizerTests, ArrayForwardingUsesConstantRangesAndExactLaneShapes)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type,
                             YdspValueType::int32Type, YdspValueType::int64Type })
        for (const int lanes : { 1, 4, 8 })
            for (const int offset : { 0, 1, 4, 8 })
            {
                SCOPED_TRACE (::testing::Message() << "type=" << static_cast<int> (type)
                                                  << ", lanes=" << lanes << ", offset=" << offset);
                const bool integer = type == YdspValueType::int32Type || type == YdspValueType::int64Type;
                const auto load = integer ? YdspIrOp::loadStateArrayI : YdspIrOp::loadStateArrayF;
                const auto store = integer ? YdspIrOp::storeStateArrayI : YdspIrOp::storeStateArrayF;
                YdspIrFunction fn;
                fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int32Type,
                                  type, type, type, YdspValueType::int32Type };
                fn.valueLanes = { 1, 1, lanes, lanes, lanes, 1 };
                fn.blocks.resize (1);
                fn.blocks[0].insts = {
                    { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
                    { YdspIrOp::constI, 1, -1, -1, -1, -1, 0.0, offset },
                    { YdspIrOp::constI, 5, -1, -1, -1, -1, 0.0, 0 },
                    { load, 2, 0, -1, -1, 32 },
                    { load, 3, 0, -1, -1, 64 },
                    { store, -1, 0, 2, -1, 0 },
                    { store, -1, 1, 3, -1, 0 },
                    { load, 4, 5, -1, -1, 0 }
                };
                YdspDiagnostics diagnostics;
                YdspOptimizer optimizer (diagnostics);
                const bool expected = offset == 0 || offset >= lanes;
                EXPECT_EQ (expected, optimizer.storeToLoadForwarding (fn));
                const auto& result = fn.blocks[0].insts.back();
                EXPECT_EQ (expected ? (integer ? YdspIrOp::movI : YdspIrOp::movF) : load, result.op);
                if (expected)
                    EXPECT_EQ (offset == 0 ? 3 : 2, result.a);
                EXPECT_EQ (lanes, fn.laneCountOf (result.result));
            }
}

TEST (YdspOptimizerTests, ArrayForwardingRejectsWidthTypeAndRedefinitionHazards)
{
    for (const int hazard : { 0, 1, 2, 3, 4, 5 })
    {
        SCOPED_TRACE (hazard);
        YdspIrFunction fn;
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float32Type, YdspValueType::float32Type };
        fn.valueLanes = { 1, 4, 4 };
        fn.blocks.resize (1);
        auto& insts = fn.blocks[0].insts;
        insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 16 },
            { YdspIrOp::storeStateArrayF, -1, 0, 1, -1, 0 }
        };
        if (hazard == 0)
            fn.valueLanes[2] = 1;
        else if (hazard == 1)
            fn.valueTypes[2] = YdspValueType::float64Type;
        else if (hazard == 2)
            insts.push_back ({ YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 1 });
        else if (hazard == 3)
            insts.push_back ({ YdspIrOp::loadStateArrayF, 1, 0, -1, -1, 32 });
        else if (hazard == 4)
            insts.push_back ({ YdspIrOp::storeStateArrayF, -1, 0, 1, -1, 1 });
        else
            insts.push_back ({ YdspIrOp::emitEvent });
        insts.push_back ({ YdspIrOp::loadStateArrayF, 2, 0, -1, -1, 0 });
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        EXPECT_FALSE (optimizer.storeToLoadForwarding (fn));
        EXPECT_EQ (YdspIrOp::loadStateArrayF, insts.back().op);
    }
}

TEST (YdspOptimizerTests, DoesNotForwardTheDelayRingRead)
{
    YdspDiagnostics diagnostics;

    // `@` writes the ring at the write pointer and reads it one slot later, so
    // the two indices differ and the read must not be forwarded - that would
    // turn the delay into a pass-through.
    auto ir = buildIr (R"YDSP(
        processor P { input stream in; output stream out; process { out = in @ 6; } }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadStateArrayF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::storeStateArrayF));
}

TEST (YdspOptimizerTests, PromotesScalarStateOutOfTheSampleLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor OnePole {
            input stream in;
            output stream out;
            state float z;
            process { z = z * 0.5 + in * 0.5; out = z; }
        }
        graph G { input stream x; output stream y; node p = OnePole; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];
    ASSERT_FALSE (fn.blocks.empty());

    const auto countIn = [] (const YdspIrBlock& block, YdspIrOp op)
    {
        return static_cast<int> (std::count_if (block.insts.begin(), block.insts.end(), [op] (const YdspIrInst& inst)
        {
            return inst.op == op;
        }));
    };

    // The load belongs to the prologue and the store to the epilogue (the last
    // block); the per-sample body in between must touch neither.
    EXPECT_EQ (1, countIn (fn.blocks.front(), YdspIrOp::loadStateF));
    EXPECT_EQ (1, countIn (fn.blocks.back(), YdspIrOp::storeStateF));

    for (size_t i = 1; i + 1 < fn.blocks.size(); ++i)
    {
        EXPECT_EQ (0, countIn (fn.blocks[i], YdspIrOp::loadStateF)) << "block " << i;
        EXPECT_EQ (0, countIn (fn.blocks[i], YdspIrOp::storeStateF)) << "block " << i;
    }
}

TEST (YdspOptimizerTests, LowersFloat64AndInt64Values)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float64 z;
            state int64 wp;
            process {
                float64 d = 0.5;
                int64 j = 2;
                float32 f = 0.25;
                z = z + d;
                wp = wp + j;
                out = in + f;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // Per-width state layout: one f64 scalar and one i64 scalar.
    EXPECT_EQ (1, fn.float64Scalars);
    EXPECT_EQ (1, fn.int64Scalars);
    EXPECT_EQ (0, fn.float32Scalars);
    EXPECT_EQ (0, fn.int32Scalars);

    bool sawF32 = false, sawF64 = false, sawI32 = false, sawI64 = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::constF)
            {
                if (fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float64Type)
                    sawF64 = true;
                if (fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float32Type)
                    sawF32 = true;
            }

            if (inst.op == YdspIrOp::constI)
            {
                if (fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int64Type)
                    sawI64 = true;
                if (fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int32Type)
                    sawI32 = true;
            }
        }

    EXPECT_TRUE (sawF64);
    EXPECT_TRUE (sawI64);
    EXPECT_TRUE (sawF32);
    EXPECT_TRUE (sawI32);

    // The f64 scalar state is loaded as a float64-typed value.
    bool sawF64Load = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::loadStateF && inst.result >= 0
                && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::float64Type)
                sawF64Load = true;

    EXPECT_TRUE (sawF64Load);
}

TEST (YdspOptimizerTests, KeepsFloat64StateArraysSeparate)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float64 mem[8];
            process block {
                for i in 0..blockSize { out[i] = float32(mem[i]); }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];
    EXPECT_EQ (8, fn.float64ArrayElements);
    EXPECT_EQ (0, fn.float32ArrayElements);
    EXPECT_EQ (0, fn.int64ArrayElements);
}

TEST (YdspOptimizerTests, CopyPropagationDoesNotClobberDelayWritePointerSlots)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor ReverbLike {
            input stream in;
            output stream out;

            input parameter float mix = 0.3;
            input parameter float feedback = 0.7;
            input parameter float damping = 0.5;

            state float c1; state float c2; state float c3; state float c4;
            state float c5; state float c6; state float c7; state float c8;
            state float l1; state float l2; state float l3; state float l4;
            state float l5; state float l6; state float l7; state float l8;

            process {
                let d1 = c1 @ 10;  l1 = l1 * damping + d1 * (1.0 - damping);  c1 = l1 * feedback + in;
                let d2 = c2 @ 11;  l2 = l2 * damping + d2 * (1.0 - damping);  c2 = l2 * feedback + in;
                let d3 = c3 @ 12;  l3 = l3 * damping + d3 * (1.0 - damping);  c3 = l3 * feedback + in;
                let d4 = c4 @ 13;  l4 = l4 * damping + d4 * (1.0 - damping);  c4 = l4 * feedback + in;
                let d5 = c5 @ 14;  l5 = l5 * damping + d5 * (1.0 - damping);  c5 = l5 * feedback + in;
                let d6 = c6 @ 15;  l6 = l6 * damping + d6 * (1.0 - damping);  c6 = l6 * feedback + in;
                let d7 = c7 @ 16;  l7 = l7 * damping + d7 * (1.0 - damping);  c7 = l7 * feedback + in;
                let d8 = c8 @ 17;  l8 = l8 * damping + d8 * (1.0 - damping);  c8 = l8 * feedback + in;
                let d9 = c1 @ 18;  c1 = l1 * feedback + in;
                let d10 = c2 @ 19; c2 = l2 * feedback + in;
                let d11 = c3 @ 20; c3 = l3 * feedback + in;
                let d12 = c4 @ 21; c4 = l4 * feedback + in;
                out = (d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9 + d10 + d11 + d12) * mix;
            }
        }
        graph G { input stream x; output stream y; node p = ReverbLike; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    std::vector<int> wpLoadSlots;
    std::vector<int> wpStoreSlots;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::loadStateI)
                wpLoadSlots.push_back (inst.a);

            if (inst.op == YdspIrOp::storeStateI)
                wpStoreSlots.push_back (inst.memIndex);
        }
    }

    EXPECT_EQ (12u, wpLoadSlots.size());
    EXPECT_EQ (12u, wpStoreSlots.size());

    // Every write-pointer load must stay within the int32 scalar segment...
    for (const auto slot : wpLoadSlots)
        EXPECT_LT (slot, fn.int32Scalars) << "write-pointer load addressed slot " << slot;

    // ... and must pair up with the matching store (same slot).
    for (const auto slot : wpLoadSlots)
        EXPECT_NE (wpStoreSlots.end(), std::find (wpStoreSlots.begin(), wpStoreSlots.end(), slot))
            << "no write-pointer store for load slot " << slot;
}

//==============================================================================
// Event-handler lowering
//==============================================================================

namespace
{

const YdspIrFunction* findEventHandler (const YdspIrProgram& ir, const char* name)
{
    for (const auto& fn : ir.eventHandlers)
        if (fn->name == name)
            return fn.get();

    return nullptr;
}

/** Collects the byte offsets carried by every event-field load of one opcode. */
std::vector<int> eventFieldOffsets (const YdspIrFunction& fn, YdspIrOp op)
{
    std::vector<int> offsets;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op)
                offsets.push_back (inst.memIndex);

    return offsets;
}

constexpr const char* eventShapeIrSource = R"YDSP(
    processor Voice {
        output stream out;
        input event midi;
        state float f;
        state int i;
        event midi (e: noteOn)        { f = e.pitch + e.velocity; if (e.isLegato) { f = 0.0; } }
        event midi (e: pitchBend)     { f = e.bendSemitones; }
        event midi (e: controlChange) { i = e.control; f = e.value; }
        process { out = f; }
    }
    graph G { input event midi; output stream y; node v = Voice; connection { midi -> v.midi; v.out -> y; } }
)YDSP";

} // namespace

TEST (YdspOptimizerTests, LowersEventFieldsToOffsetCarryingLoads)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (eventShapeIrSource, diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto* noteOn = findEventHandler (*ir, "Voice.noteOn");
    ASSERT_NE (nullptr, noteOn);

    // `.pitch` and `.velocity` are float loads at their ABI offsets, and
    // `.isLegato` is an int load of the flags word (no dedicated opcode).
    const auto noteOnFloats = eventFieldOffsets (*noteOn, YdspIrOp::loadEventFieldF);
    ASSERT_EQ (2u, noteOnFloats.size());
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, pitch)), noteOnFloats[0]);
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, velocity)), noteOnFloats[1]);

    const auto noteOnInts = eventFieldOffsets (*noteOn, YdspIrOp::loadEventFieldI);
    ASSERT_EQ (1u, noteOnInts.size());
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, flags)), noteOnInts[0]);

    // `.isLegato` masks bit 0 and compares against zero.
    EXPECT_EQ (1, countInst (*noteOn, YdspIrOp::andI));
    EXPECT_EQ (1, countInst (*noteOn, YdspIrOp::neI));

    const auto* pitchBend = findEventHandler (*ir, "Voice.pitchBend");
    ASSERT_NE (nullptr, pitchBend);

    const auto bendFloats = eventFieldOffsets (*pitchBend, YdspIrOp::loadEventFieldF);
    ASSERT_EQ (1u, bendFloats.size());
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, bend)), bendFloats[0]);

    const auto* controlChange = findEventHandler (*ir, "Voice.controlChange");
    ASSERT_NE (nullptr, controlChange);

    const auto controlInts = eventFieldOffsets (*controlChange, YdspIrOp::loadEventFieldI);
    ASSERT_EQ (1u, controlInts.size());
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, index)), controlInts[0]);

    const auto controlFloats = eventFieldOffsets (*controlChange, YdspIrOp::loadEventFieldF);
    ASSERT_EQ (1u, controlFloats.size());
    EXPECT_EQ (static_cast<int> (offsetof (YdspEventContext, value)), controlFloats[0]);
}

//==============================================================================
// Output-event ('emit') lowering
//==============================================================================

namespace
{

int storeEventFieldValueFor (const YdspIrFunction& fn, YdspIrOp storeOp, int byteOffset)
{
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == storeOp && inst.memIndex == byteOffset)
                return inst.a;

    return -1;
}

bool isZeroConst (const YdspIrFunction& fn, int valueId)
{
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result == valueId)
                return (inst.op == YdspIrOp::constF && inst.fvalue == 0.0)
                    || (inst.op == YdspIrOp::constI && inst.ivalue == 0)
                    || (inst.op == YdspIrOp::constB && ! inst.bvalue);

    return false;
}

bool isDefinedByOp (const YdspIrFunction& fn, int valueId, YdspIrOp op)
{
    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.result == valueId)
                return inst.op == op;

    return false;
}

} // namespace

TEST (YdspOptimizerTests, EmitZeroFillsFieldsOmittedFromTheEmitCall)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            output event noteOn;
            process {
                emit noteOn (pitch: in) -> noteOn;
                out = in;
            }
        }
        graph G { input stream x; output stream y; output event noteOn; node p = P; connection { x -> p.in; p.out -> y; p.noteOn -> noteOn; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    // noteOn declares 5 fields (pitch, velocity, bendSemitones, isLegato,
    // channel); only `pitch` is given, so all 5 must still be staged - the
    // other 4 zero-filled.
    EXPECT_EQ (3, countInst (fn, YdspIrOp::storeEventFieldF));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::storeEventFieldI));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::emitEvent));

    const auto pitchOffset = static_cast<int> (offsetof (YdspEventContext, pitch));
    const auto velocityOffset = static_cast<int> (offsetof (YdspEventContext, velocity));
    const auto bendOffset = static_cast<int> (offsetof (YdspEventContext, bend));
    const auto flagsOffset = static_cast<int> (offsetof (YdspEventContext, flags));
    const auto channelOffset = static_cast<int> (offsetof (YdspEventContext, channel));

    const auto pitchValue = storeEventFieldValueFor (fn, YdspIrOp::storeEventFieldF, pitchOffset);
    ASSERT_GE (pitchValue, 0);
    EXPECT_TRUE (isDefinedByOp (fn, pitchValue, YdspIrOp::loadInput)) << "pitch was given and must come from the real expression, not a zero-fill";

    const auto velocityValue = storeEventFieldValueFor (fn, YdspIrOp::storeEventFieldF, velocityOffset);
    ASSERT_GE (velocityValue, 0);
    EXPECT_TRUE (isZeroConst (fn, velocityValue)) << "velocity was omitted and must be zero-filled";

    const auto bendValue = storeEventFieldValueFor (fn, YdspIrOp::storeEventFieldF, bendOffset);
    ASSERT_GE (bendValue, 0);
    EXPECT_TRUE (isZeroConst (fn, bendValue)) << "bendSemitones was omitted and must be zero-filled";

    const auto flagsValue = storeEventFieldValueFor (fn, YdspIrOp::storeEventFieldI, flagsOffset);
    ASSERT_GE (flagsValue, 0);
    EXPECT_TRUE (isZeroConst (fn, flagsValue)) << "isLegato was omitted and must be zero-filled";

    const auto channelValue = storeEventFieldValueFor (fn, YdspIrOp::storeEventFieldI, channelOffset);
    ASSERT_GE (channelValue, 0);
    EXPECT_TRUE (isZeroConst (fn, channelValue)) << "channel was omitted and must be zero-filled";
}

//==============================================================================
// Parameter smoothing
//==============================================================================

TEST (YdspOptimizerTests, HoistsSmoothCoefficientOutOfSampleLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float gain = 0.5;
            process { out = in * smooth (gain, 0.02); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    // The whole coefficient chain (1 - exp (-samplePeriod / tau)) depends only
    // on sampleRate and a literal, so it must be hoisted into the entry block
    // and paid once per kernel invocation rather than once per sample. This is
    // the performance claim behind smooth() being one lerp per sample.
    int entryExpF = 0;

    for (const auto& inst : fn.blocks[0].insts)
        if (inst.op == YdspIrOp::expF)
            ++entryExpF;

    EXPECT_EQ (1, entryExpF);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::expF));

    // The per-sample work is the lerp plus the snap compare/select.
    EXPECT_EQ (1, countInst (fn, YdspIrOp::lerpF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::selectB));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::eqF));
}

TEST (YdspOptimizerTests, SmoothingAnnotationLowersIdenticallyToExplicitSmoothLocal)
{
    YdspDiagnostics sugarDiagnostics;

    auto sugar = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float gain = 0.5 [[ smoothing: 0.02 ]];
            process { out = in * gain; }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                          sugarDiagnostics);

    YdspDiagnostics explicitDiagnostics;

    auto explicitForm = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float gain = 0.5;
            process {
                float gainSmoothed = smooth (gain, 0.02);
                out = in * gainSmoothed;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                                 explicitDiagnostics);

    ASSERT_FALSE (sugarDiagnostics.hasErrors()) << sugarDiagnostics.toString();
    ASSERT_FALSE (explicitDiagnostics.hasErrors()) << explicitDiagnostics.toString();
    ASSERT_NE (nullptr, sugar);
    ASSERT_NE (nullptr, explicitForm);

    // `[[ smoothing: t ]]` emits literally `smooth (param, t)`, so the two
    // forms must produce the same opcodes against the same state slots. Value
    // ids are deliberately not compared: they are an emission-order artifact,
    // not part of what the sugar promises. YdspJitGraphTests'
    // SmoothIntrinsicMatchesSmoothingAnnotation pins the observable property
    // (identical output samples).
    ASSERT_EQ (sugar->kernels.size(), explicitForm->kernels.size());

    const auto& a = *sugar->kernels[0];
    const auto& b = *explicitForm->kernels[0];

    EXPECT_EQ (a.float32Scalars, b.float32Scalars);
    EXPECT_EQ (a.int32Scalars, b.int32Scalars);

    ASSERT_EQ (a.blocks.size(), b.blocks.size());

    for (size_t block = 0; block < a.blocks.size(); ++block)
    {
        const auto& lhs = a.blocks[block].insts;
        const auto& rhs = b.blocks[block].insts;

        ASSERT_EQ (lhs.size(), rhs.size()) << "block " << block;

        for (size_t i = 0; i < lhs.size(); ++i)
        {
            EXPECT_EQ (lhs[i].op, rhs[i].op) << "block " << block << " inst " << i;
            EXPECT_EQ (lhs[i].memIndex, rhs[i].memIndex) << "block " << block << " inst " << i;
        }
    }
}

TEST (YdspOptimizerTests, SmoothAllocatesOneHiddenSlotPairPerCallSite)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float gain = 0.5;
            input parameter float pan = 0.5;
            process { out = in * smooth (gain, 0.02) * smooth (pan, 0.05); }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    // The scalar layout counts only declared `state`, and this processor has
    // none, so every slot here is a smoother's: two float slots (the running
    // values) and two int slots (the primed flags).
    EXPECT_EQ (2, fn.float32Scalars);
    EXPECT_EQ (2, fn.int32Scalars);
    EXPECT_EQ (2, countInst (fn, YdspIrOp::lerpF));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::loadStateF));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::storeStateF));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::loadStateI));
    EXPECT_EQ (2, countInst (fn, YdspIrOp::storeStateI));
}

//==============================================================================
// Parameter hoisting
//==============================================================================

TEST (YdspOptimizerTests, ParameterDerivedCoefficientIsHoistedOutOfSampleLoop)
{
    YdspDiagnostics diagnostics;

    // Parameters are sampled once per kernel invocation - the builder loads
    // them in the prologue, and automation is applied *between* invocations
    // because the runtime splits the block at each event offset. So a
    // coefficient built only from parameters is loop-invariant and its exp is
    // paid once per (sub-)block, which is what lets a patch compute a filter
    // coefficient straight from a parameter without per-sample cost. Both the
    // smoothing docs and the Analog Saw / Pulse Bass patches rely on this.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            input parameter float cutoff = 400.0;
            state float z1;
            process {
                float k = 1.0 - exp (-6.283185307 * cutoff / sampleRate);
                z1 = z1 + k * (in - z1);
                out = z1;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];

    bool paramLoadInEntry = false;
    bool coefficientInEntry = false;

    for (const auto& inst : fn.blocks[0].insts)
    {
        if (inst.op == YdspIrOp::loadParam)
            paramLoadInEntry = true;

        if (inst.op == YdspIrOp::expF)
            coefficientInEntry = true;
    }

    EXPECT_TRUE (paramLoadInEntry);
    EXPECT_TRUE (coefficientInEntry);
    EXPECT_EQ (1, countInst (fn, YdspIrOp::loadParam));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::expF));

    // The recurrence itself must stay in the loop: z1 is updated per sample.
    int loopStores = 0;

    for (size_t block = 1; block < fn.blocks.size(); ++block)
        for (const auto& inst : fn.blocks[block].insts)
            if (inst.op == YdspIrOp::storeStateF)
                ++loopStores;

    EXPECT_GT (loopStores, 0);
}

//==============================================================================
// The [[ role: voiceActivity ]] flag's resolved byte offset.

TEST (YdspOptimizerTests, ResolvesActivityByteOffsetPastHiddenSlots)
{
    YdspDiagnostics diagnostics;

    // Two declared f32 scalars, then a ' and a smooth() - both of which
    // allocate hidden f32 slots *past* the declared layout and so push the i32
    // region base out. A naive `declaredFloat32Scalars * 4` would land on the
    // wrong slot; the offset must be computed off the kernel's final counts.
    auto ir = buildIr (R"YDSP(
        processor V {
            input stream in;
            output stream out;
            input event midi;
            state float env;
            state float z;
            state int other;
            state int active [[ role: voiceActivity ]];
            event midi (e: noteOn) { env = e.velocity; active = 1; }
            process {
                other = other + 1;
                env = env * 0.999;
                active = select (env < 0.000001, 0, 1);
                z = in;
                out = smooth (env, 0.003) * z';
            }
        }
        graph G { input stream x; input event midi; output stream y; node v = V[4]; connection { x -> v.in; midi -> v.midi; v.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    // The hidden slots really are there: more f32 scalars than the two declared.
    EXPECT_GT (fn.float32Scalars, 2);

    // Hidden i32 slots are appended past the declared ones too, so this is a
    // lower bound rather than an equality.
    EXPECT_GE (fn.int32Scalars, 2);

    // `active` is the second *declared* i32 scalar - declared slots come first
    // within the region - and the i32 region starts after *every* f32 scalar,
    // hidden ones included.
    EXPECT_EQ (fn.float32Scalars * 4 + 4, fn.activityByteOffset);

    // ... and it lands inside the scalar segment.
    EXPECT_LT (static_cast<size_t> (fn.activityByteOffset), fn.stateScalarSize());
}

TEST (YdspOptimizerTests, LeavesActivityByteOffsetUnsetWithoutTheAnnotation)
{
    YdspDiagnostics diagnostics;

    auto ir = buildIr (R"YDSP(
        processor V {
            output stream out;
            input event midi;
            state float env;
            state int active;
            event midi (e: noteOn) { env = e.velocity; active = 1; }
            process { out = env * float (active); }
        }
        graph G { input event midi; output stream y; node v = V[4]; connection { midi -> v.midi; v.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    EXPECT_EQ (-1, ir->kernels[0]->activityByteOffset);
}

//==============================================================================
// Full unroll of constant-trip-count loops
//==============================================================================

namespace
{

/** buildIr with the unroller switched on, as the compiler runs it natively. */
std::unique_ptr<YdspIrProgram> buildUnrolledIr (StringRef source, YdspDiagnostics& diagnostics)
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
    optimizer.setUnrollingEnabled (true);

    return optimizer.build (*analyzed);
}

/** buildIr with the native transforms on, optionally including the split. */
std::unique_ptr<YdspIrProgram> buildNativeIr (StringRef source, YdspDiagnostics& diagnostics, bool splitReductions)
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
    optimizer.setVectorizationEnabled (true);
    optimizer.setUnrollingEnabled (true);
    optimizer.setReductionSplittingEnabled (splitReductions);

    return optimizer.build (*analyzed);
}

/** A bank whose 8 modes widen to two lanes' worth of copies and accumulate. */
constexpr auto accumulatingBankSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        state float z[16];
        process {
            float sum = 0.0;
            for i in 0..16 {
                z[i] = z[i] * 0.9 + in;
                sum = sum + z[i];
            }
            out = sum;
        }
    }
    graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
)YDSP";

/** The `for` loop of a single-processor patch, i.e. not the sample loop. */
const YdspIrLoop* innerLoop (const YdspIrFunction& fn)
{
    for (const auto& loop : fn.loops)
        if (loop.bound.kind == YdspLoopBoundKind::constant)
            return &loop;

    return nullptr;
}

} // namespace

TEST (YdspOptimizerTests, FullyUnrollsAConstantTripCountLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[4];
            process {
                for i in 0..4 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto* loop = innerLoop (fn);

    ASSERT_NE (nullptr, loop);
    EXPECT_TRUE (loop->unrolled);

    // The header and the body are what the loop was; both are emptied and left
    // falling through, so no block index moved.
    EXPECT_TRUE (fn.blocks[static_cast<size_t> (loop->headerBlock)].insts.empty());
    EXPECT_TRUE (fn.blocks[static_cast<size_t> (loop->headerBlock + 1)].insts.empty());
    EXPECT_EQ (YdspIrTerm::fallthrough, fn.blocks[static_cast<size_t> (loop->headerBlock)].term);
    EXPECT_EQ (YdspIrTerm::fallthrough, fn.blocks[static_cast<size_t> (loop->headerBlock + 1)].term);

    // Four copies of the body: four element loads rather than one.
    EXPECT_EQ (4, countInst (fn, YdspIrOp::storeStateArrayF));

    // ... and the worst-case iteration count the report reads is untouched, so
    // "how many iterations could this run" still answers 4 rather than 1.
    EXPECT_EQ (4, loop->bound.constant);
}

TEST (YdspOptimizerTests, HalvesAWidenedAccumulator)
{
    YdspDiagnostics rolledUpDiagnostics, splitDiagnostics;

    auto single = buildNativeIr (accumulatingBankSource, rolledUpDiagnostics, false);
    auto split = buildNativeIr (accumulatingBankSource, splitDiagnostics, true);

    ASSERT_FALSE (rolledUpDiagnostics.hasErrors()) << rolledUpDiagnostics.toString();
    ASSERT_FALSE (splitDiagnostics.hasErrors()) << splitDiagnostics.toString();
    ASSERT_NE (nullptr, single);
    ASSERT_NE (nullptr, split);

    const auto& singleFn = *single->kernels[0];
    const auto& splitFn = *split->kernels[0];

    ASSERT_TRUE (singleFn.vectorized) << "the split only ever applies to a widened accumulator";
    ASSERT_TRUE (splitFn.vectorized);

    // The balanced tree over a 4-link chain adds nine vector values: the
    // pre-chain accumulator snapshot, four per-link addend snapshots (loop
    // splitting conservatively snapshots each addend at its original link)
    // and four internal pair sums (leaves = 5, depth 2).
    ASSERT_EQ (singleFn.valueTypes.size() + 9, splitFn.valueTypes.size());
    EXPECT_EQ (YdspVectorizer::vectorWidth, splitFn.laneCountOf (static_cast<int> (splitFn.valueTypes.size()) - 1));

    // The add count is unchanged: the four serial chain adds become the four
    // tree adds. The serial form's per-link accumulator moves are dropped, but
    // the per-link addend snapshots the tree needs replace them, so the move
    // count is no longer a meaningful comparison between the two forms.
    EXPECT_EQ (countInst (singleFn, YdspIrOp::addF), countInst (splitFn, YdspIrOp::addF));

    // Still one horizontal fold: the tree is built before it, not beside it.
    EXPECT_EQ (countInst (singleFn, YdspIrOp::vreduceAddF), countInst (splitFn, YdspIrOp::vreduceAddF));
    EXPECT_EQ (1, countInst (splitFn, YdspIrOp::vreduceAddF));
}

TEST (YdspOptimizerTests, HalvesALongAccumulatorRepeatedly)
{
    YdspDiagnostics singleDiagnostics, splitDiagnostics;

    // 32 modes over four lanes is eight links. The balanced tree adds
    // seventeen vector values for that chain: the pre-chain accumulator
    // snapshot, eight per-link addend snapshots (one per unrolled copy, see
    // HalvesAWidenedAccumulator) and eight internal pair sums (leaves = 9).
    constexpr auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[32];
            process {
                float sum = 0.0;
                for i in 0..32 {
                    z[i] = z[i] * 0.5 + in;
                    sum = sum + z[i];
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    auto single = buildNativeIr (source, singleDiagnostics, false);
    auto split = buildNativeIr (source, splitDiagnostics, true);

    ASSERT_FALSE (singleDiagnostics.hasErrors()) << singleDiagnostics.toString();
    ASSERT_FALSE (splitDiagnostics.hasErrors()) << splitDiagnostics.toString();
    ASSERT_NE (nullptr, single);
    ASSERT_NE (nullptr, split);

    ASSERT_TRUE (single->kernels[0]->vectorized);

    EXPECT_EQ (single->kernels[0]->valueTypes.size() + 17, split->kernels[0]->valueTypes.size());

    EXPECT_EQ (1, countInst (*split->kernels[0], YdspIrOp::vreduceAddF));
}

TEST (YdspOptimizerTests, LeavesAScalarAccumulatorAlone)
{
    YdspDiagnostics plainDiagnostics, splitDiagnostics;

    // No vectoriser, so the accumulator never becomes a widened one - and a
    // scalar sum carries no licence to be re-associated.
    const auto build = [] (YdspDiagnostics& diagnostics, bool splitReductions)
    {
        YdspLexer lexer (accumulatingBankSource, diagnostics);
        auto tokens = lexer.tokenize();

        YdspParser parser (std::move (tokens), diagnostics);
        auto program = parser.parseProgram();

        YdspSemanticAnalyzer analyzer (diagnostics);
        auto analyzed = analyzer.analyze (std::move (program));

        YdspOptimizer optimizer (diagnostics);
        optimizer.setUnrollingEnabled (true);
        optimizer.setReductionSplittingEnabled (splitReductions);

        return optimizer.build (*analyzed);
    };

    auto plain = build (plainDiagnostics, false);
    auto split = build (splitDiagnostics, true);

    ASSERT_FALSE (plainDiagnostics.hasErrors()) << plainDiagnostics.toString();
    ASSERT_FALSE (splitDiagnostics.hasErrors()) << splitDiagnostics.toString();
    ASSERT_NE (nullptr, plain);
    ASSERT_NE (nullptr, split);

    ASSERT_FALSE (plain->kernels[0]->vectorized);

    EXPECT_EQ (plain->kernels[0]->valueTypes.size(), split->kernels[0]->valueTypes.size())
        << "a scalar accumulator was re-associated";
    EXPECT_EQ (countInst (*plain->kernels[0], YdspIrOp::addF),
               countInst (*split->kernels[0], YdspIrOp::addF));
}

TEST (YdspOptimizerTests, ReportsWhetherAKernelWasUnrolled)
{
    YdspDiagnostics rolledDiagnostics, unrolledDiagnostics;

    constexpr auto source = R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[4];
            process {
                for i in 0..4 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP";

    auto rolled = buildIr (source, rolledDiagnostics);
    auto unrolled = buildUnrolledIr (source, unrolledDiagnostics);

    ASSERT_FALSE (rolledDiagnostics.hasErrors()) << rolledDiagnostics.toString();
    ASSERT_FALSE (unrolledDiagnostics.hasErrors()) << unrolledDiagnostics.toString();
    ASSERT_NE (nullptr, rolled);
    ASSERT_NE (nullptr, unrolled);

    YdspExecutionReport rolledReport, unrolledReport;
    YdspOptimizer::buildReport (*rolled, rolledReport);
    YdspOptimizer::buildReport (*unrolled, unrolledReport);

    ASSERT_FALSE (rolledReport.getKernels().empty());
    ASSERT_FALSE (unrolledReport.getKernels().empty());

    EXPECT_FALSE (rolledReport.getKernels()[0].unrolled);
    EXPECT_TRUE (unrolledReport.getKernels()[0].unrolled);

    EXPECT_EQ (rolledReport.getKernels()[0].boundedIterationCount,
               unrolledReport.getKernels()[0].boundedIterationCount);
}

TEST (YdspOptimizerTests, LeavesARuntimeBoundLoopRolled)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5; }
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    ASSERT_FALSE (fn.loops.empty());

    for (const auto& loop : fn.loops)
        EXPECT_FALSE (loop.unrolled) << "a loop with no constant trip count was unrolled";
}

TEST (YdspOptimizerTests, LeavesALoopTooLargeToUnrollRolled)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[32];
            process {
                float sum = 0.0;
                for i in 0..32 {
                    z[i] = z[i] * 0.9 + in * 0.1 + z[i] * z[i] * 0.01 - in * in * 0.02;
                    sum = sum + z[i] * 0.5 + z[i] * 0.25;
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto* loop = innerLoop (fn);

    ASSERT_NE (nullptr, loop);
    EXPECT_FALSE (loop->unrolled);
    EXPECT_EQ (YdspIrTerm::branchIf, fn.blocks[static_cast<size_t> (loop->headerBlock)].term);
}

TEST (YdspOptimizerTests, LeavesAHighPressureScalarLoopRolled)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[16];
            process {
                float sum = 0.0;
                for i in 0..16 {
                    z[i] = z[i] * 0.9 + in * 0.1 + z[i] * z[i] * 0.01 - in * in * 0.02;
                    sum = sum + z[i] * 0.5 + z[i] * 0.25;
                }
                out = sum;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto* loop = innerLoop (fn);

    ASSERT_NE (nullptr, loop);
    EXPECT_FALSE (loop->unrolled);
    EXPECT_EQ (YdspIrTerm::branchIf, fn.blocks[static_cast<size_t> (loop->headerBlock)].term);
}

TEST (YdspOptimizerTests, LeavesAScalarLoopWithNativeCallsRolled)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[4];
            process {
                for i in 0..4 { z[i] = pow (abs (z[i]) + 1.0, 0.5) + in; }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto* loop = innerLoop (fn);

    ASSERT_NE (nullptr, loop);
    EXPECT_FALSE (loop->unrolled);
    EXPECT_EQ (YdspIrTerm::branchIf, fn.blocks[static_cast<size_t> (loop->headerBlock)].term);
}

TEST (YdspOptimizerTests, LeavesANestedLoopRolled)
{
    YdspDiagnostics diagnostics;

    auto ir = buildUnrolledIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[4];
            process {
                for i in 0..4 {
                    for j in 0..2 { z[i] = z[i] * 0.5 + in; }
                }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                               diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    int unrolledCount = 0;

    for (const auto& loop : fn.loops)
        if (loop.unrolled)
            ++unrolledCount;

    EXPECT_LE (unrolledCount, 1);

    for (const auto& loop : fn.loops)
        if (loop.unrolled)
            EXPECT_EQ (2, loop.bound.constant) << "the outer loop was unrolled";
}

//==============================================================================

TEST (YdspOptimizerTests, FoldsStateWriteBackMovesIntoTheirProducers)
{
    YdspDiagnostics diagnostics;

    // Three chained per-sample scalar state updates. Each becomes a fresh
    // arithmetic result plus a `movF` write-back into the loop-carried state
    // register; the fold removes the moves so the state chain stays in
    // registers without a per-sample hop through a second register.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float y0;
            state float y1;
            state float y2;
            process {
                y0 = y0 + in;
                y1 = y1 * 0.5 + y0;
                y2 = y2 * 0.25 + y1;
                out = y2;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    // The integer induction update folds exactly like the float chains now: no
    // write-back move survives - each increment writes the loop-carried
    // register directly.
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movF));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movI));
}

//==============================================================================
// foldStateWriteBacks validity: every IR read must resolve to a definition.
//==============================================================================

namespace
{

// Mirrors the module's isValueIdOperand: which operands hold a value id at
// all. A scalar load's first operand is a state/param slot, not a value.
bool ydspIsValueIdOperandForTest (YdspIrOp op, int operand)
{
    switch (op)
    {
        case YdspIrOp::loadParam:
        case YdspIrOp::loadParamOut:
        case YdspIrOp::loadStateF:
        case YdspIrOp::loadStateI:
            return operand != 0;

        default:
            return true;
    }
}

// True when every value-id operand and every branchIf terminator condition has
// at least one instruction anywhere in the function writing that value. A read
// with no definition is the <Reg-0>?255 asmjit renders on AArch64.
bool ydspEveryUseIsDefined (const YdspIrFunction& fn)
{
    std::vector<char> defined (fn.valueTypes.size(), 0);

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.result >= 0 && static_cast<size_t> (inst.result) < defined.size())
                defined[static_cast<size_t> (inst.result)] = 1;
        }
    }

    const auto isDefined = [&defined, &fn] (int value)
    {
        if (value < 0)
            return true;

        const auto index = static_cast<size_t> (value);

        return index < fn.valueTypes.size() && index < defined.size() && defined[index] != 0;
    };

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (ydspIsValueIdOperandForTest (inst.op, 0) && ! isDefined (inst.a))
                return false;

            if (ydspIsValueIdOperandForTest (inst.op, 1) && ! isDefined (inst.b))
                return false;

            if (ydspIsValueIdOperandForTest (inst.op, 2) && ! isDefined (inst.c))
                return false;
        }

        if (block.term == YdspIrTerm::branchIf && ! isDefined (block.termCond))
            return false;
    }

    return true;
}

YdspIrInst ydspInst (YdspIrOp op, int result, int a = -1, int b = -1, int c = -1)
{
    YdspIrInst inst;
    inst.op = op;
    inst.result = result;
    inst.a = a;
    inst.b = b;
    inst.c = c;
    return inst;
}

} // namespace

TEST (YdspOptimizerTests, ConstantFoldingUnsignedBoundsRejectsNegativeIndices)
{
    for (const auto type : { YdspValueType::int32Type, YdspValueType::int64Type })
        for (const int64_t index : { int64_t { -2147483648 }, int64_t { -1 }, int64_t { 0 }, int64_t { 7 }, int64_t { 8 }, int64_t { 2147483647 } })
        {
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes = { type, type, YdspValueType::boolType };
            YdspIrBlock block;
            block.insts = { ydspInst (YdspIrOp::constI, 0), ydspInst (YdspIrOp::constI, 1), ydspInst (YdspIrOp::ltUI, 2, 0, 1) };
            block.insts[0].ivalue = index;
            block.insts[1].ivalue = 8;
            fn.blocks.push_back (std::move (block));
            EXPECT_TRUE (optimizer.constantFolding (fn));
            EXPECT_EQ (YdspIrOp::constB, fn.blocks[0].insts[2].op);
            EXPECT_EQ (index >= 0 && index < 8, fn.blocks[0].insts[2].bvalue);
        }
}

TEST (YdspOptimizerTests, BoundsProofRejectsRedefinedValuesAndOverflow)
{
    for (const bool redefine : { false, true })
    {
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        YdspIrFunction fn;
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int32Type,
                          YdspValueType::int32Type, YdspValueType::int32Type,
                          YdspValueType::boolType };
        fn.blocks.resize (1);
        auto& insts = fn.blocks[0].insts;
        insts = { ydspInst (YdspIrOp::constI, 0), ydspInst (YdspIrOp::constI, 1),
                  ydspInst (YdspIrOp::addI, 2, 0, 1), ydspInst (YdspIrOp::constI, 3),
                  ydspInst (YdspIrOp::ltUI, 4, 2, 3) };
        insts[0].ivalue = redefine ? 0 : std::numeric_limits<int32_t>::max();
        insts[1].ivalue = 1;
        insts[3].ivalue = 8;
        if (redefine)
        {
            auto overwrite = ydspInst (YdspIrOp::constI, 2);
            overwrite.ivalue = -1;
            insts.insert (insts.begin() + 3, overwrite);
        }
        EXPECT_FALSE (optimizer.eliminateProvenBoundsChecks (fn));
        EXPECT_EQ (YdspIrOp::ltUI, insts.back().op);
    }
}

TEST (YdspOptimizerTests, BoundsProofHandlesSignedProductsAndRetainsOverflowChecks)
{
    struct Case
    {
        int64_t lowerA, upperA, lowerB, upperB, offset, bound;
        bool proven;
    };
    const Case cases[] {
        { 0, 3, 4, 4, 3, 16, true },
        { -3, 0, -4, -2, 0, 13, true },
        { -3, 2, -4, 5, 15, 31, true },
        { -3, 2, -4, 5, 15, 25, false },
        { -3, 2, -4, 5, 14, 31, false },
        { 0, 4, 4, 4, 0, 16, false },
        { -2147483648, 2147483647, 0, 0, 0, 1, true },
        { 0, 65536, 0, 65536, 0, 2147483647, false },
        { -2147483648, -2147483648, -1, -1, 0, 2147483647, false },
        { -2147483648, -2147483648, -2147483648, -2147483648, 0, 1, false },
    };
    for (const auto& item : cases)
        for (const bool redefine : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "a=[" << item.lowerA << "," << item.upperA
                                              << "], b=[" << item.lowerB << "," << item.upperB
                                              << "], offset=" << item.offset << ", bound=" << item.bound
                                              << ", redefine=" << redefine);
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes.assign (13, YdspValueType::int32Type);
            fn.valueTypes[12] = YdspValueType::boolType;
            fn.blocks.resize (1);
            auto& insts = fn.blocks[0].insts;
            insts = {
                ydspInst (YdspIrOp::loadParam, 0), ydspInst (YdspIrOp::loadParam, 1),
                ydspInst (YdspIrOp::constI, 2), ydspInst (YdspIrOp::constI, 3),
                ydspInst (YdspIrOp::constI, 4), ydspInst (YdspIrOp::constI, 5),
                ydspInst (YdspIrOp::clampI, 6, 0, 2, 3),
                ydspInst (YdspIrOp::clampI, 7, 1, 4, 5),
                ydspInst (YdspIrOp::mulI, 8, 6, 7),
                ydspInst (YdspIrOp::constI, 9),
                ydspInst (YdspIrOp::addI, 10, 8, 9),
                ydspInst (YdspIrOp::constI, 11),
                ydspInst (YdspIrOp::ltUI, 12, 10, 11),
            };
            insts[2].ivalue = item.lowerA;
            insts[3].ivalue = item.upperA;
            insts[4].ivalue = item.lowerB;
            insts[5].ivalue = item.upperB;
            insts[9].ivalue = item.offset;
            insts[11].ivalue = item.bound;
            if (redefine)
                insts.insert (insts.begin() + 9, ydspInst (YdspIrOp::loadParam, 8));
            const bool expected = item.proven && ! redefine;
            EXPECT_EQ (expected, optimizer.eliminateProvenBoundsChecks (fn));
            EXPECT_EQ (expected ? YdspIrOp::constB : YdspIrOp::ltUI, insts.back().op);
            if (expected)
                EXPECT_TRUE (insts.back().bvalue);
        }
}

TEST (YdspOptimizerTests, RemovesBoundsChecksForMaskedStridedArrayAccess)
{
    YdspDiagnostics diagnostics;
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float table[16];
            process {
                int32 index = (int32 (in) & 3) * 4 + 3;
                out = table[index];
                table[index] = in;
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP", diagnostics);
    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    EXPECT_EQ (0, countInst (*ir->kernels[0], YdspIrOp::ltUI));
    EXPECT_EQ (1, countInst (*ir->kernels[0], YdspIrOp::loadStateArrayF));
    EXPECT_EQ (1, countInst (*ir->kernels[0], YdspIrOp::storeStateArrayF));
}

TEST (YdspOptimizerTests, ConstantFoldingSignExtendsInt32Constants)
{
    for (const int64_t bits : { int64_t { 0 }, int64_t { 7 }, int64_t { -1 }, int64_t { 0x7fffffff }, int64_t { 0x80000000 }, int64_t { 0xffffffff } })
    {
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        YdspIrFunction fn;
        fn.valueTypes = { YdspValueType::int32Type, YdspValueType::int64Type };
        YdspIrBlock block;
        block.insts = { ydspInst (YdspIrOp::constI, 0), ydspInst (YdspIrOp::extI, 1, 0) };
        block.insts[0].ivalue = bits;
        fn.blocks.push_back (std::move (block));

        EXPECT_TRUE (optimizer.constantFolding (fn));
        const auto& result = fn.blocks[0].insts[1];
        EXPECT_EQ (YdspIrOp::constI, result.op);
        EXPECT_EQ (static_cast<int64_t> (static_cast<int32_t> (bits)), result.ivalue) << bits;
        EXPECT_EQ (YdspValueType::int64Type, fn.valueTypes[1]);
        EXPECT_FALSE (optimizer.constantFolding (fn));
    }
}

TEST (YdspOptimizerTests, BoundsUnrollingAroundFourLaneMathCalls)
{
    for (const int modes : { 8, 16 })
        for (const int calls : { 1, 2 })
        {
            SCOPED_TRACE (::testing::Message() << "modes=" << modes << ", calls=" << calls);
            YdspDiagnostics diagnostics;
            const auto source = String (R"YDSP(
                processor P {
                    input stream in;
                    output stream out;
                    state float phase[MODES];
                    process {
                        float sum = 0.0;
                        for i in 0..MODES {
                            phase[i] = phase[i] + in;
                            sum = sum + (SHAPE);
                        }
                        out = sum;
                    }
                }
                graph G { input stream x; output stream y; node p = P;
                    connection { x -> p.in; p.out -> y; } }
            )YDSP")
                                    .replace ("MODES", String (modes))
                                    .replace ("SHAPE", calls == 1 ? "sin (phase[i])" : "sin (phase[i]) + cos (phase[i])");
            YdspLexer lexer (source, diagnostics);
            YdspParser parser (lexer.tokenize(), diagnostics);
            auto parsed = parser.parseProgram();
            ASSERT_NE (nullptr, parsed) << diagnostics.toString();
            YdspSemanticAnalyzer analyzer (diagnostics);
            auto analyzed = analyzer.analyze (std::move (parsed));
            ASSERT_NE (nullptr, analyzed) << diagnostics.toString();
            YdspOptimizer optimizer (diagnostics);
            optimizer.setFastMath (true);
            optimizer.setVectorizationEnabled (true);
            optimizer.setVectorWidth (4);
            optimizer.setTargetHasVectorMath (true);
            auto ir = optimizer.build (*analyzed);
            ASSERT_NE (nullptr, ir) << diagnostics.toString();
            ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
            ASSERT_FALSE (ir->kernels.empty());
            auto& fn = *ir->kernels[0];
            StringArray vectorizationDecisions;
            for (const auto& result : fn.vectorizationResults)
                vectorizationDecisions.add (result.describe());
            ASSERT_TRUE (fn.vectorized) << vectorizationDecisions.joinIntoString ("\n");
            if (modes == 8 && calls == 1)
            {
                auto unsupported = fn;
                unsupported.vectorMathEnabled = false;
                optimizer.fullyUnrollBoundedLoops (unsupported);
                ASSERT_NE (nullptr, innerLoop (unsupported));
                EXPECT_FALSE (innerLoop (unsupported)->unrolled);

                auto pressured = fn;
                ASSERT_NE (nullptr, innerLoop (pressured));
                const auto& loop = *innerLoop (pressured);
                auto& preheader = pressured.blocks[static_cast<size_t> (loop.headerBlock - 1)].insts;
                auto& body = pressured.blocks[static_cast<size_t> (loop.headerBlock + 1)].insts;
                const auto scalar = static_cast<int> (pressured.valueTypes.size());
                pressured.valueTypes.push_back (YdspValueType::float32Type);
                pressured.valueLanes.push_back (1);
                preheader.push_back ({ YdspIrOp::constF, scalar, -1, -1, -1, -1, 0.25 });
                for (int index = 0; index < 3; ++index)
                {
                    const auto value = static_cast<int> (pressured.valueTypes.size());
                    pressured.valueTypes.push_back (YdspValueType::float32Type);
                    pressured.valueLanes.push_back (4);
                    preheader.push_back ({ YdspIrOp::vsplat, value, scalar });
                    body.insert (body.end() - 2, { YdspIrOp::storeStateArrayF, -1, loop.induction, value, -1, pressured.float32ArrayElements });
                    pressured.float32ArrayElements += modes;
                }
                ASSERT_LE (body.size() * 2, 64u);
                optimizer.fullyUnrollBoundedLoops (pressured);
                EXPECT_FALSE (innerLoop (pressured)->unrolled);
            }
            optimizer.fullyUnrollBoundedLoops (fn);
            ASSERT_NE (nullptr, innerLoop (fn));
            const bool expected = modes == 8 && calls == 1;
            EXPECT_EQ (expected, innerLoop (fn)->unrolled);
            EXPECT_EQ (expected ? 2 : 1, countInst (fn, YdspIrOp::sinF));
            EXPECT_TRUE (ydspEveryUseIsDefined (fn));
            for (const auto& block : fn.blocks)
                for (const auto& inst : block.insts)
                    if (inst.op == YdspIrOp::sinF)
                        EXPECT_EQ (4, fn.laneCountOf (inst.result));
        }
}

TEST (YdspOptimizerTests, UnrolledBanksExposeUniqueProductsAndConstantIndices)
{
    for (const bool vector : { false, true })
    {
        YdspDiagnostics diagnostics;
        const auto source = vector ? String (accumulatingBankSource) : String (accumulatingBankSource).replace ("16", "4");
        auto ir = vector ? buildNativeIr (source, diagnostics, false)
                         : buildUnrolledIr (source, diagnostics);
        ASSERT_NE (nullptr, ir) << diagnostics.toString();
        ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
        auto& fn = *ir->kernels[0];
        ASSERT_NE (nullptr, innerLoop (fn));
        ASSERT_TRUE (innerLoop (fn)->unrolled);
        const int lanes = vector ? 4 : 1;
        const int copies = 4;
        EXPECT_EQ (copies, countInst (fn, YdspIrOp::mulF));
        std::vector<int> definitions (fn.valueTypes.size(), 0);
        std::vector<int64_t> literals (fn.valueTypes.size(), -1);
        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
            {
                if (inst.result >= 0)
                    ++definitions[static_cast<size_t> (inst.result)];
                if (inst.op == YdspIrOp::constI)
                    literals[static_cast<size_t> (inst.result)] = inst.ivalue;
            }
        int stores = 0;
        for (const auto& block : fn.blocks)
            for (const auto& inst : block.insts)
            {
                if (inst.op == YdspIrOp::mulF)
                {
                    EXPECT_EQ (1, definitions[static_cast<size_t> (inst.result)]);
                    EXPECT_EQ (lanes, fn.laneCountOf (inst.result));
                }
                if (inst.op == YdspIrOp::storeStateArrayF)
                {
                    EXPECT_EQ (1, definitions[static_cast<size_t> (inst.a)]);
                    EXPECT_EQ (stores++ * lanes, literals[static_cast<size_t> (inst.a)]);
                }
            }
        EXPECT_EQ (copies, stores);
        YdspOptimizer optimizer (diagnostics);
        optimizer.setTargetHasPackedFusedMultiplyAdd (true);
        optimizer.contractMultiplyAdd (fn);
        optimizer.deadCodeElimination (fn);
        EXPECT_EQ (copies, countInst (fn, YdspIrOp::fmaF));
        EXPECT_EQ (0, countInst (fn, YdspIrOp::mulF));
        EXPECT_TRUE (ydspEveryUseIsDefined (fn));
    }
}

TEST (YdspOptimizerTests, ArrayLoadSharingStopsAtStoresAndIndexChanges)
{
    for (const bool integer : { false, true })
    {
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        YdspIrFunction fn;
        const auto type = integer ? YdspValueType::int32Type : YdspValueType::float32Type;
        const auto load = integer ? YdspIrOp::loadStateArrayI : YdspIrOp::loadStateArrayF;
        const auto store = integer ? YdspIrOp::storeStateArrayI : YdspIrOp::storeStateArrayF;
        fn.valueTypes = { YdspValueType::int32Type, type, type, type, type };
        fn.blocks.resize (1);
        fn.blocks[0].insts = {
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 0 },
            { load, 1, 0, -1, -1, 0 },
            { load, 2, 0, -1, -1, 0 },
            { store, -1, 0, 2, -1, 0 },
            { load, 3, 0, -1, -1, 0 },
            { YdspIrOp::constI, 0, -1, -1, -1, -1, 0.0, 1 },
            { load, 4, 0, -1, -1, 0 }
        };
        EXPECT_TRUE (optimizer.commonSubexpressionElimination (fn));
        EXPECT_EQ (integer ? YdspIrOp::movI : YdspIrOp::movF, fn.blocks[0].insts[2].op);
        EXPECT_EQ (load, fn.blocks[0].insts[4].op);
        EXPECT_EQ (load, fn.blocks[0].insts[6].op);
    }
}

TEST (YdspOptimizerTests, CombinesConstantProductsOnlyUnderFastMath)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type })
        for (const bool fast : { false, true })
            for (const bool modified : { false, true })
            {
                YdspDiagnostics diagnostics;
                YdspOptimizer optimizer (diagnostics);
                YdspIrFunction fn;
                fn.fastMath = fast;
                fn.valueTypes.assign (5, type);
                fn.blocks.resize (1);
                fn.blocks[0].insts = {
                    { YdspIrOp::loadParam, 0, 0 },
                    { YdspIrOp::constF, 1, -1, -1, -1, -1, 0.05 },
                    { YdspIrOp::constF, 2, -1, -1, -1, -1, 2.302585092994046 },
                    { YdspIrOp::mulF, 3, 0, 1 }
                };
                if (modified)
                    fn.blocks[0].insts.push_back ({ YdspIrOp::loadParam, 0, 1 });
                fn.blocks[0].insts.push_back ({ YdspIrOp::mulF, 4, 3, 2 });
                fn.blocks[0].insts.push_back ({ YdspIrOp::storeParamOut, -1, 4, -1, -1, 0 });
                optimizer.algebraicSimplification (fn);
                optimizer.deadCodeElimination (fn);
                EXPECT_EQ (fast && ! modified ? 1 : 2, countInst (fn, YdspIrOp::mulF));
                EXPECT_TRUE (ydspEveryUseIsDefined (fn));
            }
}

TEST (YdspOptimizerTests, SharedConstantDivisorsKeepTheirOtherUses)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type })
        for (const bool fastMath : { false, true })
        {
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.fastMath = fastMath;
            fn.valueTypes.assign (5, type);
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                ydspInst (YdspIrOp::loadParam, 0, 0),
                ydspInst (YdspIrOp::constF, 1),
                ydspInst (YdspIrOp::divF, 2, 0, 1),
                ydspInst (YdspIrOp::divF, 3, 0, 1),
                ydspInst (YdspIrOp::addF, 4, 0, 1)
            };
            fn.blocks[0].insts[1].fvalue = 4.0;
            EXPECT_EQ (fastMath, optimizer.algebraicSimplification (fn));
            EXPECT_EQ (fastMath ? 0 : 2, countInst (fn, YdspIrOp::divF));
            EXPECT_EQ (fastMath ? 2 : 0, countInst (fn, YdspIrOp::mulF));
            EXPECT_EQ (4.0, fn.blocks[0].insts[1].fvalue);
            EXPECT_EQ (1, fn.blocks[0].insts.back().b);
        }
}

TEST (YdspOptimizerTests, ConstantBasePowerSpecializationRespectsMathPolicyAndDomain)
{
    for (const bool fastMath : { false, true })
        for (const double base : { -2.0, -0.0, 0.0, 0.5, 1.0, 10.0, std::numeric_limits<double>::infinity() })
        {
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.fastMath = fastMath;
            fn.valueTypes.assign (3, YdspValueType::float32Type);
            fn.blocks.resize (1);
            fn.blocks[0].insts = {
                ydspInst (YdspIrOp::loadParam, 0, 0),
                ydspInst (YdspIrOp::constF, 1),
                ydspInst (YdspIrOp::powF, 2, 1, 0)
            };
            fn.blocks[0].insts[1].fvalue = base;
            const bool specialize = fastMath && base > 0.0 && base != 1.0 && std::isfinite (base);
            EXPECT_EQ (specialize, optimizer.algebraicSimplification (fn));
            EXPECT_EQ (specialize ? 0 : 1, countInst (fn, YdspIrOp::powF));
            EXPECT_EQ (specialize ? 1 : 0, countInst (fn, YdspIrOp::expF));
            EXPECT_EQ (specialize ? 1 : 0, countInst (fn, YdspIrOp::mulF));
        }
}

TEST (YdspOptimizerTests, FoldStateWriteBacksKeepsAMoveWhoseSourceIsReadInAnotherBlock)
{
    YdspDiagnostics diagnostics;

    // `f` reassigns its parameter inside an `if`, so the read of `t` in the
    // continuation after the (inlined) call lives in a different block than
    // the parameter-copy `movF` the builder emitted. Folding that copy into
    // its producer would rename the producer's result and leave the later read
    // with no definition at all - the <Reg-0>?255 ten YdspExamplePatchTests
    // fail with on AArch64.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            func f (t: float) : float {
                float r = 0.0;
                if (t < 0.5) {
                    t = t * 2.0;
                    r = t;
                }
                return r;
            }

            process {
                float x = in * 0.5;
                out = x - f (x);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (ydspEveryUseIsDefined (fn));
}

TEST (YdspOptimizerTests, FoldsIntegerStateWriteBackMovesIntoTheirProducers)
{
    YdspDiagnostics diagnostics;

    // A sample-mode integer state chain, the ring-buffer shape: two carried
    // counters updated every sample. Each update is a fresh add plus a `movI`
    // write-back; folding removes the moves so the counters stay in registers
    // without a per-sample hop through a second register.
    auto ir = buildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int reads;
            state int writes;
            process {
                reads = reads + 1;
                writes = writes + 2;
                out = in + float (reads + writes);
            }
        }
        graph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }
    )YDSP",
                       diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_EQ (1u, ir->kernels.size());

    const auto& fn = *ir->kernels[0];

    EXPECT_EQ (0, countInst (fn, YdspIrOp::movI));

    // The two per-sample increments and the `reads + writes` sum still exist
    // (plus the sample-loop induction increment), each state update now
    // writing its carried register in place.
    EXPECT_EQ (4, countInst (fn, YdspIrOp::addI));
    EXPECT_TRUE (ydspEveryUseIsDefined (fn));
}

TEST (YdspOptimizerTests, FoldStateWriteBacksCollapsesCopiesWithoutRereadingTheirSource)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type,
                             YdspValueType::int32Type, YdspValueType::int64Type })
    {
        const bool floating = type == YdspValueType::float32Type || type == YdspValueType::float64Type;
        const auto move = floating ? YdspIrOp::movF : YdspIrOp::movI;
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        YdspIrFunction fn;
        fn.valueTypes.assign (4, type);
        fn.blocks.resize (1);
        auto& insts = fn.blocks.front().insts;
        insts = {
            ydspInst (floating ? YdspIrOp::loadStateF : YdspIrOp::loadStateI, 0, 0),
            ydspInst (YdspIrOp::loadParam, 1, 0),
            ydspInst (move, 2, 1),
            ydspInst (floating ? YdspIrOp::constF : YdspIrOp::constI, 1),
            ydspInst (move, 0, 2),
            ydspInst (floating ? YdspIrOp::addF : YdspIrOp::addI, 3, 2, 0)
        };

        optimizer.foldStateWriteBacks (fn);

        ASSERT_EQ (5u, insts.size());
        EXPECT_EQ (move, insts[2].op);
        EXPECT_EQ (0, insts[2].result);
        EXPECT_EQ (1, insts[2].a);
        EXPECT_EQ (1, insts[3].result); // The source changes after the snapshot.
        EXPECT_EQ (0, insts.back().a);
        EXPECT_EQ (0, insts.back().b);
        EXPECT_TRUE (ydspEveryUseIsDefined (fn));
    }
}

TEST (YdspOptimizerTests, FoldStateWriteBackCopiesPreserveOldStateAndLaterSnapshotReads)
{
    for (const bool floating : { false, true })
    {
        for (const bool readOldState : { false, true })
        {
            const auto move = floating ? YdspIrOp::movF : YdspIrOp::movI;
            const auto add = floating ? YdspIrOp::addF : YdspIrOp::addI;
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes.assign (5, floating ? YdspValueType::float32Type : YdspValueType::int32Type);
            fn.blocks.resize (1);
            auto& insts = fn.blocks.front().insts;
            insts = {
                ydspInst (floating ? YdspIrOp::loadStateF : YdspIrOp::loadStateI, 0, 0),
                ydspInst (YdspIrOp::loadParam, 1, 0),
                ydspInst (move, 2, 1)
            };
            if (readOldState)
                insts.push_back (ydspInst (add, 3, 0, 1));
            insts.push_back (ydspInst (move, 0, 2));
            if (! readOldState)
                insts.push_back (ydspInst (floating ? YdspIrOp::constF : YdspIrOp::constI, 0));
            insts.push_back (ydspInst (add, 4, 2, 0));

            optimizer.foldStateWriteBacks (fn);

            ASSERT_EQ (6u, insts.size());
            EXPECT_EQ (2, insts[2].result);
            EXPECT_EQ (1, insts[2].a);
            EXPECT_EQ (2, insts.back().a);
            EXPECT_TRUE (ydspEveryUseIsDefined (fn));
        }
    }
}

TEST (YdspOptimizerTests, FoldFusedSubtractWriteBacksWithEachCarriedOperand)
{
    for (const auto type : { YdspValueType::float32Type, YdspValueType::float64Type })
    {
        for (int carriedOperand = 0; carriedOperand < 3; ++carriedOperand)
        {
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes.assign (5, type);
            fn.blocks.resize (1);
            int operands[] { 1, 2, 0 };
            std::swap (operands[carriedOperand], operands[2]);
            auto& insts = fn.blocks.front().insts;
            insts = {
                ydspInst (YdspIrOp::loadStateF, 0, 0),
                ydspInst (YdspIrOp::loadParam, 1, 0),
                ydspInst (YdspIrOp::loadParam, 2, 1),
                ydspInst (YdspIrOp::fmsubF, 3, operands[0], operands[1], operands[2]),
                ydspInst (YdspIrOp::movF, 0, 3),
                ydspInst (YdspIrOp::addF, 4, 3, 1)
            };

            optimizer.foldStateWriteBacks (fn);

            ASSERT_EQ (5u, insts.size());
            EXPECT_EQ (YdspIrOp::fmsubF, insts[3].op);
            EXPECT_EQ (0, insts[3].result);
            EXPECT_EQ (operands[0], insts[3].a);
            EXPECT_EQ (operands[1], insts[3].b);
            EXPECT_EQ (operands[2], insts[3].c);
            EXPECT_EQ (0, insts.back().a);
        }
    }
}

TEST (YdspOptimizerTests, FoldStateWriteBacksDoesNotTouchSlotIndexOperands)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    // Hand-built block where the `produced` value id (2) numerically equals a
    // state slot carried in loadStateF's first operand. The fold rewrites uses
    // of `produced` - the move and the add below - never the slot field.
    constexpr int constZero = 0;
    constexpr int constHalf = 1;
    constexpr int produced = 2;
    constexpr int carried = 3;
    constexpr int loaded = 4;
    constexpr int used = 5;

    YdspIrFunction fn;
    fn.valueTypes.assign (6, YdspValueType::float32Type);

    YdspIrBlock block;
    block.insts =
    {
        ydspInst (YdspIrOp::constF, constZero, -1, -1, -1),
        ydspInst (YdspIrOp::constF, constHalf, -1, -1, -1),
        ydspInst (YdspIrOp::mulF, produced, constZero, constHalf),
        ydspInst (YdspIrOp::movF, carried, produced),
        ydspInst (YdspIrOp::loadStateF, loaded, produced), // slot index == produced!
        ydspInst (YdspIrOp::addF, used, produced, carried),
    };
    fn.blocks.push_back (std::move (block));

    optimizer.foldStateWriteBacks (fn);

    const auto& insts = fn.blocks[0].insts;

    // The fold applied (no movF left) and redirected the real uses of the
    // produced value, but the loadStateF slot index must be untouched.
    bool sawLoad = false;
    bool sawAdd = false;
    int moveCount = 0;

    for (const auto& inst : insts)
    {
        if (inst.op == YdspIrOp::movF)
            ++moveCount;

        if (inst.op == YdspIrOp::loadStateF)
        {
            sawLoad = true;
            EXPECT_EQ (produced, inst.a);
        }

        if (inst.op == YdspIrOp::addF)
        {
            sawAdd = true;
            EXPECT_EQ (carried, inst.a);
        }
    }

    EXPECT_EQ (0, moveCount);
    EXPECT_TRUE (sawLoad);
    EXPECT_TRUE (sawAdd);
    EXPECT_TRUE (ydspEveryUseIsDefined (fn));
}

TEST (YdspOptimizerTests, FoldStateWriteBacksAppliesOverlappingFoldsOneAtATime)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    // Two candidate write-backs into the same carried register, with a read of
    // the first producer's value between the second producer and its move:
    //
    //   v1 = a * b;        carried = v1;    (candidate fold 1)
    //   v2 = c + d;                          (candidate fold 2's producer)
    //   use = v1 + e;                        (reads v1, after v2's producer)
    //   carried = v2;                        (candidate fold 2's move)
    //
    // Folding fold 1 first redirects `use` to `carried`, which makes fold 2
    // unsafe: its producer would redefine `carried` before that redirected
    // read. A batch rewrite validated against the pre-fold snapshot cannot see
    // that - both folds look valid - so folds must be applied one at a time.
    constexpr int constA = 0;
    constexpr int constB = 1;
    constexpr int constC = 2;
    constexpr int produced1 = 3;
    constexpr int produced2 = 4;
    constexpr int carried = 5;
    constexpr int used = 6;

    YdspIrFunction fn;
    fn.valueTypes.assign (7, YdspValueType::float32Type);

    YdspIrBlock block;
    block.insts =
    {
        ydspInst (YdspIrOp::constF, constA, -1, -1, -1),
        ydspInst (YdspIrOp::constF, constB, -1, -1, -1),
        ydspInst (YdspIrOp::constF, constC, -1, -1, -1),
        ydspInst (YdspIrOp::mulF, produced1, constA, constB),
        ydspInst (YdspIrOp::movF, carried, produced1),
        ydspInst (YdspIrOp::addF, produced2, constA, constB),
        ydspInst (YdspIrOp::addF, used, produced1, constC),
        ydspInst (YdspIrOp::movF, carried, produced2),
    };
    fn.blocks.push_back (std::move (block));

    optimizer.foldStateWriteBacks (fn);

    const auto& insts = fn.blocks[0].insts;

    // Fold 1 applies (its producer now writes the carried register and its
    // move is gone); fold 2 is refused, so its move and its producer's result
    // id survive.
    bool sawFirstProducer = false;
    bool sawSecondProducer = false;
    bool sawUse = false;
    int moveCount = 0;

    for (const auto& inst : insts)
    {
        if (inst.op == YdspIrOp::movF)
            ++moveCount;

        if (inst.op == YdspIrOp::mulF)
        {
            sawFirstProducer = true;
            EXPECT_EQ (carried, inst.result);
        }

        if (inst.op == YdspIrOp::addF && inst.result == produced2)
            sawSecondProducer = true;

        if (inst.op == YdspIrOp::addF && inst.result == used)
        {
            sawUse = true;
            EXPECT_EQ (carried, inst.a);
        }
    }

    EXPECT_EQ (1, moveCount);
    EXPECT_TRUE (sawFirstProducer);
    EXPECT_TRUE (sawSecondProducer);
    EXPECT_TRUE (sawUse);
    EXPECT_TRUE (ydspEveryUseIsDefined (fn));
}

TEST (YdspOptimizerTests, ConstantFoldingShiftsRespectWidthAndMaskedCounts)
{
    struct Case
    {
        int64_t value, count, left32, right32, left64, right64;
    };
    const Case cases[] {
        { 1, 32, 1, 1, 4294967296, 0 },
        { 3, 33, 6, 1, 25769803776, 0 },
        { 3, 64, 3, 3, 3, 3 },
        { -8, 1, -16, -4, -16, -4 },
        { -1, -1, std::numeric_limits<int32_t>::min(), -1, std::numeric_limits<int64_t>::min(), -1 },
        { 2147483648, 0, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), 2147483648, 2147483648 },
        { 4294967295, 1, -2, -1, 8589934590, 2147483647 },
    };
    for (const auto& item : cases)
        for (const bool wide : { false, true })
            for (const bool left : { false, true })
            {
                SCOPED_TRACE (::testing::Message() << "value=" << item.value << ", count=" << item.count << ", wide=" << wide << ", left=" << left);
                YdspDiagnostics diagnostics;
                YdspOptimizer optimizer (diagnostics);
                YdspIrFunction fn;
                fn.valueTypes.assign (3, wide ? YdspValueType::int64Type : YdspValueType::int32Type);
                YdspIrBlock block;
                block.insts = {
                    ydspInst (YdspIrOp::constI, 0),
                    ydspInst (YdspIrOp::constI, 1),
                    ydspInst (left ? YdspIrOp::shlI : YdspIrOp::shrI, 2, 0, 1),
                };
                block.insts[0].ivalue = item.value;
                block.insts[1].ivalue = item.count;
                fn.blocks.push_back (std::move (block));
                ASSERT_TRUE (optimizer.constantFolding (fn));
                const auto& result = fn.blocks[0].insts[2];
                EXPECT_EQ (YdspIrOp::constI, result.op);
                EXPECT_EQ (wide ? (left ? item.left64 : item.right64) : (left ? item.left32 : item.right32), result.ivalue);
            }
}

TEST (YdspOptimizerTests, ConstantIntegerArithmeticPreservesWidthBeforeConsumers)
{
    struct Case
    {
        YdspIrOp op;
        int64_t a, b, narrow, wide;
    };
    const Case cases[] {
        { YdspIrOp::addI, 2147483647, 1, -2147483648, 2147483648 },
        { YdspIrOp::subI, -2147483648, 1, 2147483647, -2147483649 },
        { YdspIrOp::mulI, 65536, 65536, 0, 4294967296 },
        { YdspIrOp::mulI, 2147483647, 2, -2, 4294967294 },
        { YdspIrOp::minI, 4294967295, 1, -1, 1 },
        { YdspIrOp::maxI, 4294967295, 1, 1, 4294967295 },
        { YdspIrOp::andI, 4294967295, 2147483648, -2147483648, 2147483648 },
        { YdspIrOp::orI, 2147483648, 1, -2147483647, 2147483649 },
        { YdspIrOp::xorI, 4294967295, 1, -2, 4294967294 },
    };
    for (const auto& item : cases)
        for (const bool wide : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "op=" << static_cast<int> (item.op) << ", wide=" << wide);
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes.assign (6, wide ? YdspValueType::int64Type : YdspValueType::int32Type);
            fn.valueTypes[4] = YdspValueType::boolType;
            fn.valueTypes[5] = YdspValueType::float64Type;
            YdspIrBlock block;
            block.insts = {
                ydspInst (YdspIrOp::constI, 0),
                ydspInst (YdspIrOp::constI, 1),
                ydspInst (item.op, 2, 0, 1),
                ydspInst (YdspIrOp::constI, 3),
                ydspInst (YdspIrOp::ltI, 4, 2, 3),
                ydspInst (YdspIrOp::itof, 5, 2),
            };
            block.insts[0].ivalue = item.a;
            block.insts[1].ivalue = item.b;
            fn.blocks.push_back (std::move (block));
            ASSERT_TRUE (optimizer.constantFolding (fn));
            const auto expected = wide ? item.wide : item.narrow;
            const auto& insts = fn.blocks[0].insts;
            EXPECT_EQ (YdspIrOp::constI, insts[2].op);
            EXPECT_EQ (expected, insts[2].ivalue);
            EXPECT_EQ (YdspIrOp::constB, insts[4].op);
            EXPECT_EQ (expected < 0, insts[4].bvalue);
            EXPECT_EQ (YdspIrOp::constF, insts[5].op);
            EXPECT_EQ (static_cast<double> (expected), insts[5].fvalue);
        }
}

TEST (YdspOptimizerTests, ConstantFloatToIntFoldingRespectsBothWidths)
{
    struct Case
    {
        double value;
        int64_t narrow, wide;
    };
    const Case cases[] {
        { 7.9, 7, 7 }, { -7.9, -7, -7 },
        { 2147483647.75, 2147483647, 2147483647 },
        { 2147483648.0, 2147483647, 2147483648 },
        { -2147483648.75, -2147483648, -2147483648 },
        { -2147483649.0, -2147483648, -2147483649 },
        { 9223372036854775808.0, 2147483647, std::numeric_limits<int64_t>::max() },
        { -9223372036854775808.0, -2147483648, std::numeric_limits<int64_t>::min() },
        { std::numeric_limits<double>::infinity(), 2147483647, std::numeric_limits<int64_t>::max() },
        { -std::numeric_limits<double>::infinity(), -2147483648, std::numeric_limits<int64_t>::min() },
        { std::numeric_limits<double>::quiet_NaN(), 0, 0 },
    };
    for (const auto& item : cases)
        for (const bool wide : { false, true })
        {
            SCOPED_TRACE (::testing::Message() << "value=" << item.value << ", wide=" << wide);
            YdspDiagnostics diagnostics;
            YdspOptimizer optimizer (diagnostics);
            YdspIrFunction fn;
            fn.valueTypes = { YdspValueType::float64Type, wide ? YdspValueType::int64Type : YdspValueType::int32Type };
            YdspIrBlock block;
            block.insts = { ydspInst (YdspIrOp::constF, 0), ydspInst (YdspIrOp::ftoi, 1, 0) };
            block.insts[0].fvalue = item.value;
            fn.blocks.push_back (std::move (block));
            EXPECT_TRUE (optimizer.constantFolding (fn));
            EXPECT_EQ (YdspIrOp::constI, fn.blocks[0].insts[1].op);
            EXPECT_EQ (wide ? item.wide : item.narrow, fn.blocks[0].insts[1].ivalue);
        }
}

TEST (YdspOptimizerTests, ConstantConversionsRespectSourceStorageWidth)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes = { YdspValueType::int32Type, YdspValueType::float64Type,
                      YdspValueType::float32Type, YdspValueType::int32Type, YdspValueType::int64Type };
    YdspIrBlock block;
    block.insts = {
        ydspInst (YdspIrOp::constI, 0), ydspInst (YdspIrOp::itof, 1, 0),
        ydspInst (YdspIrOp::constF, 2), ydspInst (YdspIrOp::ftoi, 3, 2), ydspInst (YdspIrOp::ftoi, 4, 2),
    };
    block.insts[0].ivalue = 4294967295;
    block.insts[2].fvalue = 2147483647.0; // Rounds to 2^31 in float32 storage.
    fn.blocks.push_back (std::move (block));
    ASSERT_TRUE (optimizer.constantFolding (fn));
    EXPECT_EQ (YdspIrOp::constF, fn.blocks[0].insts[1].op);
    EXPECT_EQ (-1.0, fn.blocks[0].insts[1].fvalue);
    EXPECT_EQ (YdspIrOp::constI, fn.blocks[0].insts[3].op);
    EXPECT_EQ (2147483647, fn.blocks[0].insts[3].ivalue);
    EXPECT_EQ (YdspIrOp::constI, fn.blocks[0].insts[4].op);
    EXPECT_EQ (2147483648, fn.blocks[0].insts[4].ivalue);
}

TEST (YdspOptimizerTests, ConstantFoldingDoesNotMaterialiseAnUnknownConversion)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes = { YdspValueType::float64Type, YdspValueType::int64Type };
    YdspIrBlock block;
    block.insts = { ydspInst (YdspIrOp::loadInput, 0), ydspInst (YdspIrOp::ftoi, 1, 0) };
    fn.blocks.push_back (std::move (block));
    EXPECT_FALSE (optimizer.constantFolding (fn));
    EXPECT_EQ (YdspIrOp::ftoi, fn.blocks[0].insts[1].op);
    EXPECT_EQ (0, fn.blocks[0].insts[1].a);
}

TEST (YdspOptimizerTests, CopyPropagationDoesNotRetargetATerminatorPastARedefinition)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    // c = a; c = p && q; branchIf c: the terminator reads the *second* c (a
    // non-move, so nothing later can legitimately retarget it). Copy
    // propagation over the first move must leave the terminator alone; the
    // old code stopped at the redefinition but still retargeted it to `a`.
    YdspIrFunction fn;
    fn.valueTypes.assign (4, YdspValueType::boolType);

    YdspIrBlock block;
    block.insts =
    {
        ydspInst (YdspIrOp::constB, 0, -1, -1, -1), // a = true
        ydspInst (YdspIrOp::movB, 1, 0),            // c = a
        ydspInst (YdspIrOp::constB, 2, -1, -1, -1), // p = false
        ydspInst (YdspIrOp::constB, 3, -1, -1, -1), // q = false
        ydspInst (YdspIrOp::andB, 1, 2, 3),         // c = p && q (redefinition)
    };

    block.insts[0].bvalue = true;
    block.insts[2].bvalue = false;
    block.insts[3].bvalue = true;

    block.term = YdspIrTerm::branchIf;
    block.termCond = 1;
    block.termTarget = 0;
    block.termTarget2 = 0;
    fn.blocks.push_back (std::move (block));

    optimizer.copyPropagation (fn);

    EXPECT_EQ (1, fn.blocks[0].termCond) << "the terminator must keep reading the redefined c";
}

TEST (YdspOptimizerTests, DeadCodeWorklistRemovesLongDependencyChains)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes.assign (4096, YdspValueType::float32Type);
    fn.blocks.resize (1);
    auto& instructions = fn.blocks.front().insts;
    instructions.push_back (ydspInst (YdspIrOp::constF, 0));

    for (int id = 1; id < 4096; ++id)
        instructions.push_back (ydspInst (YdspIrOp::movF, id, id - 1));

    EXPECT_TRUE (optimizer.deadCodeElimination (fn));
    EXPECT_TRUE (instructions.empty());
    EXPECT_FALSE (optimizer.deadCodeElimination (fn));
}

TEST (YdspOptimizerTests, DeadCodeWorklistPreservesAllDefinitionsOfATerminatorCondition)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes.assign (3, YdspValueType::boolType);
    fn.blocks.resize (1);
    auto& block = fn.blocks.front();
    block.insts = {
        ydspInst (YdspIrOp::constB, 0),
        ydspInst (YdspIrOp::movB, 1, 0),
        ydspInst (YdspIrOp::constB, 1),
        ydspInst (YdspIrOp::movB, 2, 0),
        ydspInst (YdspIrOp::constB, 2)
    };
    block.term = YdspIrTerm::branchIf;
    block.termCond = 1;
    block.termTarget = block.termTarget2 = 0;

    EXPECT_TRUE (optimizer.deadCodeElimination (fn));
    ASSERT_EQ (3u, block.insts.size());
    EXPECT_EQ (0, block.insts[0].result);
    EXPECT_EQ (1, block.insts[1].result);
    EXPECT_EQ (1, block.insts[2].result);
    EXPECT_FALSE (optimizer.deadCodeElimination (fn));
}

TEST (YdspOptimizerTests, SubtractZeroIdentityRespectsItsSignInStrictMode)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);

    for (const bool fastMath : { false, true })
    {
        for (const double zero : { 0.0, -0.0 })
        {
            YdspIrFunction fn;
            fn.fastMath = fastMath;
            fn.valueTypes.assign (3, YdspValueType::float64Type);
            fn.blocks.resize (1);
            auto& instructions = fn.blocks.front().insts;
            instructions = {
                ydspInst (YdspIrOp::loadParam, 0, 0),
                ydspInst (YdspIrOp::constF, 1),
                ydspInst (YdspIrOp::subF, 2, 0, 1)
            };
            instructions[1].fvalue = zero;
            const bool shouldSimplify = fastMath || ! std::signbit (zero);

            EXPECT_EQ (shouldSimplify, optimizer.algebraicSimplification (fn));
            EXPECT_EQ (shouldSimplify ? YdspIrOp::movF : YdspIrOp::subF, instructions.back().op);
            EXPECT_FALSE (optimizer.algebraicSimplification (fn));
        }
    }
}

TEST (YdspOptimizerTests, SharesImmutableEntryCoefficientsAcrossBlocks)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes.assign (6, YdspValueType::float32Type);
    fn.valueTypes[5] = YdspValueType::int32Type;
    fn.blocks.resize (3);
    fn.blocks[0].insts = {
        ydspInst (YdspIrOp::loadParam, 0, 0),
        ydspInst (YdspIrOp::constF, 1),
        ydspInst (YdspIrOp::subF, 2, 1, 0),
        ydspInst (YdspIrOp::constF, 3),
        ydspInst (YdspIrOp::subF, 4, 3, 0),
        ydspInst (YdspIrOp::constI, 5)
    };
    fn.blocks[0].insts[1].fvalue = fn.blocks[0].insts[3].fvalue = 1.0;
    fn.blocks[1].insts = { ydspInst (YdspIrOp::storeOutput, -1, 5, 2) };
    fn.blocks[2].insts = { ydspInst (YdspIrOp::storeOutput, -1, 5, 4) };

    EXPECT_TRUE (optimizer.commonSubexpressionElimination (fn));
    EXPECT_TRUE (optimizer.deadCodeElimination (fn));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::constF));
    EXPECT_EQ (1, countInst (fn, YdspIrOp::subF));
    EXPECT_EQ (0, countInst (fn, YdspIrOp::movF));
    EXPECT_EQ (2, fn.blocks[2].insts[0].b);
    EXPECT_FALSE (optimizer.commonSubexpressionElimination (fn));
}

TEST (YdspOptimizerTests, EntrySharingPreservesMutableValuesAndReenteredBlocks)
{
    for (const bool reentered : { false, true })
    {
        YdspDiagnostics diagnostics;
        YdspOptimizer optimizer (diagnostics);
        YdspIrFunction fn;
        fn.valueTypes.assign (4, YdspValueType::float32Type);
        fn.blocks.resize (2);
        fn.blocks[0].insts = {
            ydspInst (YdspIrOp::constF, 0),
            ydspInst (YdspIrOp::constF, 1)
        };
        fn.blocks[1].insts = { ydspInst (YdspIrOp::addF, 2, 0, 1) };
        if (reentered)
        {
            fn.blocks[1].term = YdspIrTerm::branch;
            fn.blocks[1].termTarget = 0;
        }
        else
        {
            fn.blocks[1].insts.push_back (ydspInst (YdspIrOp::constF, 1));
            fn.blocks[1].insts.back().fvalue = 2.0;
            fn.blocks[1].insts.push_back (ydspInst (YdspIrOp::addF, 3, 0, 1));
        }

        optimizer.commonSubexpressionElimination (fn);
        EXPECT_EQ (1, fn.blocks[1].insts[0].b);
        if (! reentered)
            EXPECT_EQ (1, fn.blocks[1].insts.back().b);
    }
}

TEST (YdspOptimizerTests, EntryConstantSharingPreservesSignedZeroAndTypes)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes = { YdspValueType::float32Type, YdspValueType::float32Type,
                      YdspValueType::float64Type, YdspValueType::int32Type };
    fn.blocks.resize (2);
    fn.blocks[0].insts = {
        ydspInst (YdspIrOp::constF, 0),
        ydspInst (YdspIrOp::constF, 1),
        ydspInst (YdspIrOp::constF, 2),
        ydspInst (YdspIrOp::constI, 3)
    };
    fn.blocks[0].insts[1].fvalue = -0.0;
    for (int value = 0; value < 3; ++value)
        fn.blocks[1].insts.push_back (ydspInst (YdspIrOp::storeOutput, -1, 3, value));

    EXPECT_FALSE (optimizer.commonSubexpressionElimination (fn));
    for (int value = 0; value < 3; ++value)
        EXPECT_EQ (value, fn.blocks[1].insts[static_cast<size_t> (value)].b);
}

TEST (YdspOptimizerTests, CleanupPassesReportAnAlreadyStableFunction)
{
    YdspDiagnostics diagnostics;
    YdspOptimizer optimizer (diagnostics);
    YdspIrFunction fn;
    fn.valueTypes.assign (1, YdspValueType::float32Type);
    fn.blocks.resize (1);
    fn.blocks.front().insts = {
        ydspInst (YdspIrOp::constF, 0),
        ydspInst (YdspIrOp::storeStateF, -1, 0)
    };

    EXPECT_FALSE (optimizer.constantFolding (fn));
    EXPECT_FALSE (optimizer.algebraicSimplification (fn));
    EXPECT_FALSE (optimizer.ifConversion (fn));
    EXPECT_FALSE (optimizer.storeToLoadForwarding (fn));
    EXPECT_FALSE (optimizer.copyPropagation (fn));
    EXPECT_FALSE (optimizer.commonSubexpressionElimination (fn));
    EXPECT_FALSE (optimizer.deadCodeElimination (fn));
}
