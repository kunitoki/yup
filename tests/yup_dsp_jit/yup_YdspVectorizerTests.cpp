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

#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>

using namespace yup;

namespace
{

//==============================================================================

std::unique_ptr<YdspIrProgram> vectorizerBuildIr (StringRef source, YdspDiagnostics& diagnostics)
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

    return optimizer.build (*analyzed);
}

String vectorizerPatch (StringRef processorBody)
{
    return String (processorBody)
         + "\ngraph G { input stream x; output stream y; node p = P; connection { x -> p.in; p.out -> y; } }";
}

int vectorizerCountInst (const YdspIrFunction& fn, YdspIrOp op)
{
    int count = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == op)
                ++count;

    return count;
}

bool vectorizerWidens (StringRef processorBody)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (processorBody), diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    EXPECT_NE (nullptr, ir);

    if (ir == nullptr || ir->kernels.empty())
        return false;

    return ir->kernels[0]->vectorized;
}

YdspAudioGraph vectorizerCompileGraph (StringRef source, YdspCompiler& compiler)
{
    auto result = compiler.compile (source);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

YdspAudioGraph vectorizerCompileGraph (StringRef source, YdspCompiler& compiler, const YdspCompileOptions& options)
{
    auto result = compiler.compile (source, options);
    EXPECT_TRUE (result.wasOk()) << compiler.getDiagnostics().toString();

    if (! result.wasOk())
        return {};

    return std::move (result).getValue();
}

void vectorizerRunBlock (YdspAudioGraph& graph, const float* input, float* output, int numSamples)
{
    std::vector<YdspInputBuffer> inputs;
    inputs.emplace_back (Span<const float> (input, static_cast<size_t> (numSamples)));

    std::vector<YdspOutputBuffer> outputs;
    outputs.emplace_back (Span<float> (output, static_cast<size_t> (numSamples)));

    graph.process (yup::YdspProcessRequest { inputs, outputs, numSamples });
}

constexpr auto vectorizerBankSource = R"YDSP(
    let modes = 8;

    processor P {
        input stream in;
        output stream out;

        state float z[modes];

        process {
            float sum = 0.0;

            for i in 0..modes {
                z[i] = z[i] * 0.5 + in;
                sum = sum + z[i];
            }

            out = sum;
        }
    }
)YDSP";

constexpr auto vectorizerElementWiseSource = R"YDSP(
    let taps = 4;

    processor P {
        input stream in;
        output stream out;

        state float a[taps];

        process {
            for i in 0..taps {
                a[i] = a[i] * 0.5 + in * 0.25;
            }

            out = a[0] + a[3];
        }
    }
)YDSP";

// Three loops: loop 0 widens (constant span, bounded state-array accesses),
// loop 1 trips on a rounding op consuming a widened value (transcendentals are only admitted to
// the packed set when the target reports vector math, so this stays a
// rejection on both native and wasm), loop 2 is shorter than one vector.
// Loop 1 reads z[] which loop 0 writes, so loop fusion leaves them apart.
// (The state arrays avoid the built-in constant names `pi`, `e` and `inf`.)
constexpr auto vectorizerMixedReasonsSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;

        state float z[8];
        state float w[8];
        state float r[2];

        process block {
            let sample = in[0];
            for i in 0..8 { z[i] = z[i] * 0.5 + sample; }
            for j in 0..8 { w[j] = round (z[j]); }
            for k in 0..2 { r[k] = r[k] * 0.5; }

            out[0] = z[0] + w[0] + r[0];
        }
    }
)YDSP";

} // namespace

//==============================================================================

TEST (YdspVectorizerTests, WidensAConstantBoundBankLoop)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerBankSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (YdspVectorizer::vectorWidth, fn.vectorWidth);

    EXPECT_GT (vectorizerCountInst (fn, YdspIrOp::vsplat), 0);
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));

    bool foundWidenedLoad = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
            if (inst.op == YdspIrOp::loadStateArrayF && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedLoad = true;

    EXPECT_TRUE (foundWidenedLoad);
}

TEST (YdspVectorizerTests, WidensAnElementWiseLoopWithNoReduction)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerElementWiseSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (0, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));
}

TEST (YdspVectorizerTests, IsOffUnlessEnabled)
{
    YdspDiagnostics diagnostics;

    YdspLexer lexer (vectorizerPatch (vectorizerBankSource), diagnostics);
    auto tokens = lexer.tokenize();

    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    ASSERT_NE (nullptr, program);

    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    ASSERT_NE (nullptr, analyzed);

    YdspOptimizer optimizer (diagnostics); // no setVectorizationEnabled
    auto ir = optimizer.build (*analyzed);

    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    EXPECT_FALSE (ir->kernels[0]->vectorized);
    EXPECT_EQ (1, ir->kernels[0]->vectorWidth);
    EXPECT_TRUE (ir->kernels[0]->valueLanes.empty());
}

//==============================================================================

TEST (YdspVectorizerTests, WidensATripCountWithAScalarTail)
{
    // 6 modes with 4 lanes: two scalar iterations peeled, one vector trip.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // The peel is straight-line code in the preheader: one constI index per
    // leading scalar iteration, plus the re-started induction at start + tail.
    const auto& preheader = fn.blocks[static_cast<size_t> (ir->kernels[0]->loops[0].headerBlock - 1)];
    bool foundPeelIndexZero = false;
    bool foundPeelIndexOne = false;
    bool foundVectorStart = false;

    for (const auto& inst : preheader.insts)
    {
        if (inst.op != YdspIrOp::constI)
            continue;

        foundPeelIndexZero = foundPeelIndexZero || inst.ivalue == 0;
        foundPeelIndexOne = foundPeelIndexOne || inst.ivalue == 1;
        foundVectorStart = foundVectorStart || inst.ivalue == 2;
    }

    EXPECT_TRUE (foundPeelIndexZero) << "the first scalar iteration must be peeled at index 0";
    EXPECT_TRUE (foundPeelIndexOne) << "the second scalar iteration must be peeled at index 1";
    EXPECT_TRUE (foundVectorStart) << "the vector loop must start at start + tail = 2";
}

TEST (YdspVectorizerTests, LeavesATripCountShorterThanOneVectorScalar)
{
    // 2 modes never fill a single 4-lane vector: no widening to gain from.
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[2];
            process {
                for i in 0..2 { z[i] = z[i] * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAConstantBoundLoopWithANonZeroStart)
{
    // span = 12 - 2 = 10: two leading scalar iterations (i = 2, 3) and two
    // vector trips over [4, 12). The old divisibility-only rule would have
    // stepped from i = 2 by 4 and overrun the bound.
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[12];
            process {
                for i in 2..12 { z[i] = z[i] * 0.5 + in; }
                out = z[2] + z[11];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsARuntimeLoopBound)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[64];
            process block {
                for i in 0..blockSize { z[i] = z[i] * 0.5; }
                out[0] = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAnIndirectIndex)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    let j = 7 - i;
                    z[j] = z[j] * 0.5 + in;
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsTheInductionVariableUsedAsAValue)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = z[i] * 0.5 + float (i); }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsATranscendentalOnAWidenedValueWithoutVectorMath)
{
    // The IR-level helper builds with vector math unset, which is what a wasm
    // or SLEEF-less target reports: the transcendental stays outside the
    // packed set and the loop is refused. The compiler path on native admits
    // it - see WidensATranscendentalLoopWithVectorMath below.
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = sin (z[i]) * 0.5 + in; }
                out = z[0];
            }
        }
    )YDSP"));
}

#if ! YUP_WASM
TEST (YdspVectorizerTests, WidensATranscendentalLoopWithVectorMath)
{
    // The full compiler path reports vector math on native (sleef_library is
    // linked), so the same shape now widens and is lowered to 4-lane SLEEF
    // calls rather than being refused. The block-mode body keeps the loop
    // count at exactly the one explicit `for`, like the mixed-reasons source.
    YdspCompileOptions options;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                let sample = in[0];
                for i in 0..8 { z[i] = sin (z[i]) * 0.5 + sample; }
                out[0] = z[0];
            }
        }
    )YDSP"), compiler, options);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    const auto& report = graph.getExecutionReport().getKernels()[0].loopVectorization;

    ASSERT_EQ (1u, report.loops.size());
    EXPECT_TRUE (report.loops[0].widened());
    EXPECT_EQ (1, report.countWidened());
}
#endif

TEST (YdspVectorizerTests, WidensAFloatSelectOnWidenedValues)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float w[8];
            process {
                for i in 0..8 { z[i] = select (z[i] > w[i], z[i], w[i]); }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidenedSelectUsesAVectorPredicateAndSelect)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = select (z[i] > 0.0, z[i], in); }
                out = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    EXPECT_TRUE (fn.vectorized);

    bool foundVectorPredicate = false;
    bool foundVectorSelect = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::gtF && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundVectorPredicate = true;

            if (inst.op == YdspIrOp::selectB && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundVectorSelect = true;
        }

    EXPECT_TRUE (foundVectorPredicate);
    EXPECT_TRUE (foundVectorSelect);
}

TEST (YdspVectorizerTests, RejectsAScalarStateWriteInsideTheLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float env;
            process {
                for i in 0..8 {
                    z[i] = z[i] * 0.5 + in;
                    env = env * 0.5 + z[i];
                }
                out = env;
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensASampleLoopOverStreams)
{
    // The canonical per-sample gain shape: stream accesses at the induction
    // variable of the blockSize-bound sample loop.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.5 + 0.25; }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);
    EXPECT_EQ (YdspVectorizer::vectorWidth, fn.vectorWidth);

    // One widened stream load and one widened stream store in the loop body.
    bool foundWidenedLoad = false;
    bool foundWidenedStore = false;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::loadInput && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedLoad = true;

            if (inst.op == YdspIrOp::storeOutput && fn.laneCountOf (inst.b) == fn.vectorWidth)
                foundWidenedStore = true;
        }

    EXPECT_TRUE (foundWidenedLoad);
    EXPECT_TRUE (foundWidenedStore);

    // The runtime remainder is a second blockSize-bound loop appended after
    // the widened sample loop.
    ASSERT_EQ (2u, fn.loops.size());
    EXPECT_EQ (YdspLoopBoundKind::blockSize, fn.loops[1].bound.kind);
}

TEST (YdspVectorizerTests, WidensABlockSizeBoundStreamLoop)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensABlockSizeMinusOneBoundStreamLoop)
{
    // The builder records `0..blockSize - 1` as a blockSizeMinusConst bound.
    // It is a runtime bound like plain blockSize, so it needs a stream access
    // for the remainder loop, and the whole-vector trip count is computed from
    // the runtime `blockSize - 1` value by the same epilogue machinery.
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize - 1 { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, KeepsConstantBoundStreamLoopScalarWithoutABlockLengthProof)
{
    // A valid process call may supply fewer than ten samples.
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..10 { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensPositiveStartsWithRuntimeBounds)
{
    for (const int start : { 1, 3, 4, 7, 16 })
        EXPECT_TRUE (vectorizerWidens (String (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for i in )YDSP") + String (start) + R"YDSP(..blockSize - 1 { out[i] = in[i] * 0.5; }
                }
            }
        )YDSP"));
}

TEST (YdspVectorizerTests, WidensIfConvertedStreamLoops)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    float v;
                    if (in[i] > 0.0) v = in[i] * 0.5; else v = in[i] * 0.25;
                    out[i] = v;
                }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsFixedIndexReadsFromWrittenArrays)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = z[0] + in; }
                out = z[7];
            }
        }
    )YDSP"));

    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = out[0] + in[i]; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensALoopContainingRoundingIntrinsics)
{
    // floor/ceil/rint lower to one native packed instruction on both SIMD
    // targets (frintm/frintp/frintn, roundps), so a quantiser loop may widen.
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    out[i] = floor (in[i] * 4.0) * 0.25 + ceil (in[i]) * 0.5 + rint (in[i]) * 0.125;
                }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, KeepsStreamLoopScalarWhenItMayExceedTheStateArray)
{
    // blockSize may exceed the eight-element state allocation.
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                for i in 0..blockSize { out[i] = z[i] + in[i]; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensAStreamLoopWithAReduction)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                float s = 0.0;
                for i in 0..blockSize { s = s + in[i]; }
                out[0] = s;
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // The vector accumulator folds into the scalar one in the tail loop's
    // header, and the scalar tail accumulates on top of it.
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::vreduceAddF));
}

TEST (YdspVectorizerTests, RejectsAStreamStoreAtANonInductionIndex)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process block {
                for i in 0..8 { out[0] = z[i] + in[i]; }
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensEveryStreamLoopInAKernel)
{
    // Two blockSize-bound stream loops: widening the first appends its scalar
    // tail loop, which must not shift the second loop's index or be widened
    // itself.
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5; }
                for j in 0..blockSize { out[j] = out[j] + 1.0; }
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    EXPECT_TRUE (fn.vectorized);

    // Two widened stream loads (one per loop) and two widened stream stores.
    int widenedLoads = 0;
    int widenedStores = 0;

    for (const auto& block : fn.blocks)
        for (const auto& inst : block.insts)
        {
            if ((inst.op == YdspIrOp::loadInput || inst.op == YdspIrOp::loadOutput)
                && fn.laneCountOf (inst.result) == fn.vectorWidth)
                ++widenedLoads;

            if (inst.op == YdspIrOp::storeOutput && fn.laneCountOf (inst.b) == fn.vectorWidth)
                ++widenedStores;
        }

    EXPECT_EQ (2, widenedLoads);
    EXPECT_EQ (2, widenedStores);

    // Each widened loop appended exactly one scalar tail.
    EXPECT_EQ (4u, fn.loops.size());
}

TEST (YdspVectorizerTests, RejectsControlFlowInsideTheLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    if (in > 0.5) { z[i] = z[i] * 0.5; } else { z[i] = z[i] * 0.25; }
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsANestedLoop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 {
                    for j in 0..4 { z[i] = z[i] * 0.5 + in; }
                }
                out = z[0];
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsALoopWithNoArrayAccessToWiden)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process {
                float sum = 0.0;
                for i in 0..8 { sum = sum + in; }
                out = sum;
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, WidensALoopUsingIntegerMin)
{
    EXPECT_TRUE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int buf[16];
            process {
                for i in 0..16 { buf[i] = min (buf[i], 10); }
                out = in + float32 (buf[0]);
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAnInt64Loop)
{
    EXPECT_FALSE (vectorizerWidens (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int64 buf[16];
            process {
                for i in 0..16 { buf[i] = buf[i] + 1; }
                out = in + float32 (buf[0]);
            }
        }
    )YDSP"));
}

TEST (YdspVectorizerTests, RejectsAnEmitInsideTheLoop)
{
    YdspDiagnostics diagnostics;

    auto ir = vectorizerBuildIr (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            output event noteOn;
            state float z[8];
            process {
                for i in 0..8 {
                    z[i] = z[i] * 0.5 + in;
                    emit noteOn (pitch: 60, velocity: 0.8) -> noteOn;
                }
                out = z[0];
            }
        }
        graph G { input stream x; output stream y; output event noteOn; node p = P; connection { x -> p.in; p.out -> y; p.noteOn -> noteOn; } }
    )YDSP",
                                  diagnostics);

    EXPECT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());
    EXPECT_FALSE (ir->kernels[0]->vectorized);
}

//==============================================================================

TEST (YdspVectorizerTests, WidenedBankProducesTheExpectedOutput)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    float expected = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        expected = expected * 0.5f + 1.0f;
        EXPECT_FLOAT_EQ (8.0f * expected, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, SplitAccumulatorBankProducesTheExpectedOutput)
{
    for (const int modes : { 16, 32 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (String ("let modes = ") + String (modes) + R"YDSP(;

            processor P {
                input stream in;
                output stream out;

                state float z[modes];

                process {
                    float sum = 0.0;

                    for i in 0..modes {
                        z[i] = z[i] * 0.5 + in;
                        sum = sum + z[i];
                    }

                    out = sum;
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, 8);

        const std::vector<float> input (8, 1.0f);
        std::vector<float> output (8, 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), 8);

        float expected = 0.0f;

        for (int sample = 0; sample < 8; ++sample)
        {
            expected = expected * 0.5f + 1.0f;

            EXPECT_FLOAT_EQ (static_cast<float> (modes) * expected, output[static_cast<size_t> (sample)])
                << modes << " modes, sample " << sample;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, WidenedElementWiseLoopIsBitExact)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerElementWiseSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    float tap = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        tap = tap * 0.5f + 0.25f;
        EXPECT_FLOAT_EQ (2.0f * tap, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, ReportsTheLaneCountPerKernel)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);

    const auto& kernels = graph.getExecutionReport().getKernels();
    ASSERT_FALSE (kernels.empty());

    bool foundVectorized = false;

    for (const auto& kernel : kernels)
    {
        if (! kernel.vectorized)
            continue;

        foundVectorized = true;
        EXPECT_TRUE (kernel.vectorWidth == 4 || kernel.vectorWidth == 8);

        EXPECT_EQ (8, kernel.boundedIterationCount);
    }

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (foundVectorized);
#else
    EXPECT_FALSE (foundVectorized); // a scalar wasm build (no -msimd128) stays scalar
#endif
}

#if ! YUP_WASM

TEST (YdspVectorizerTests, EmitsPackedInstructions)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerBankSource), compiler);

    ASSERT_TRUE (graph.isValid());

    const auto listing = graph.getDiagnostics().toString();
    ASSERT_FALSE (listing.isEmpty());

#if ASMJIT_ARCH_ARM
    EXPECT_TRUE (listing.contains ("v0.4s") || listing.contains (".4s"))
        << "expected an ASIMD vector form in the listing";
    EXPECT_TRUE (listing.contains ("faddp"));
#elif ASMJIT_ARCH_X86
    EXPECT_TRUE (listing.contains ("mulps") || listing.contains ("addps"));
    EXPECT_TRUE (listing.contains ("shufps"));
#endif

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << listing << std::endl;
}

#endif

//==============================================================================

TEST (YdspVectorizerTests, PeeledBankProducesTheExpectedOutput)
{
    for (const int modes : { 6, 10, 14 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (String ("let modes = ") + String (modes) + R"YDSP(;

            processor P {
                input stream in;
                output stream out;

                state float z[modes];

                process {
                    float sum = 0.0;

                    for i in 0..modes {
                        z[i] = z[i] * 0.5 + in;
                        sum = sum + z[i];
                    }

                    out = sum;
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, 8);

        const std::vector<float> input (8, 1.0f);
        std::vector<float> output (8, 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), 8);

        float expected = 0.0f;

        for (int sample = 0; sample < 8; ++sample)
        {
            expected = expected * 0.5f + 1.0f;

            EXPECT_FLOAT_EQ (static_cast<float> (modes) * expected, output[static_cast<size_t> (sample)])
                << modes << " modes, sample " << sample;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, PeeledNonZeroStartLoopProducesTheExpectedOutput)
{
    YdspCompiler compiler;

    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;

            state float z[12];

            process {
                for i in 2..12 { z[i] = z[i] * 0.5 + in; }
                out = z[2] + z[11];
            }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    graph.prepare (44100.0, 8);

    const std::vector<float> input (8, 1.0f);
    std::vector<float> output (8, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 8);

    // The state starts zeroed: z[2] and z[11] decay identically per sample.
    float z = 0.0f;

    for (int sample = 0; sample < 8; ++sample)
    {
        z = z * 0.5f + 1.0f;

        EXPECT_FLOAT_EQ (2.0f * z, output[static_cast<size_t> (sample)]) << "sample " << sample;
    }

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, BlockSizeMinusOneStreamLoopProducesTheExpectedOutput)
{
    // The widened loop plus its scalar remainder must cover exactly the
    // `blockSize - 1` samples - the last sample of the block is never touched,
    // at block sizes that are and are not whole multiples of the vector width.
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for i in 0..blockSize - 1 { out[i] = in[i] * 0.5 + in[i] * in[i]; }
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        std::vector<float> input (static_cast<size_t> (blockSize));
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        for (int s = 0; s < blockSize; ++s)
            input[static_cast<size_t> (s)] = 0.25f * static_cast<float> (s);

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        for (int s = 0; s < blockSize - 1; ++s)
        {
            const auto in = input[static_cast<size_t> (s)];

            EXPECT_FLOAT_EQ (in * 0.5f + in * in, output[static_cast<size_t> (s)])
                << blockSize << " samples, sample " << s;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, PositiveStartStreamLoopsPreservePrefixesAndTails)
{
    for (const int start : { 1, 3, 4, 7, 16 })
    {
        YdspCompiler compiler;
        auto graph = vectorizerCompileGraph (vectorizerPatch (String (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for j in 0..blockSize { out[j] = -2.0; }
                    for i in )YDSP") + String (start) + R"YDSP(..blockSize - 1 { out[i] = in[i] * 0.5; }
                }
            }
        )YDSP"), compiler);
        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, 65);

        for (int size = 1; size <= 65; ++size)
        {
            std::vector<float> input (static_cast<size_t> (size), 3.0f);
            std::vector<float> output (static_cast<size_t> (size), 99.0f);
            vectorizerRunBlock (graph, input.data(), output.data(), size);
            for (int i = 0; i < size; ++i)
                EXPECT_FLOAT_EQ (i >= start && i < size - 1 ? 1.5f : -2.0f, output[static_cast<size_t> (i)])
                    << "start " << start << ", size " << size << ", sample " << i;
        }
    }
}

TEST (YdspVectorizerTests, IfConvertedStreamLoopsMatchScalarAcrossBlockSizes)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    float v;
                    if (in[i] > 0.0) v = in[i] * 0.5; else v = in[i] * 0.25;
                    out[i] = v;
                }
                for j in 0..blockSize { out[j] = out[j] + 1.0; }
            }
        }
    )YDSP"), compiler);
    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    graph.prepare (44100.0, 65);
    for (int size = 1; size <= 65; ++size)
    {
        std::vector<float> input (static_cast<size_t> (size));
        std::vector<float> output (static_cast<size_t> (size));
        for (int i = 0; i < size; ++i)
            input[static_cast<size_t> (i)] = static_cast<float> (i % 7 - 3);
        vectorizerRunBlock (graph, input.data(), output.data(), size);
        for (int i = 0; i < size; ++i)
        {
            const float x = input[static_cast<size_t> (i)];
            EXPECT_FLOAT_EQ (x * (x > 0.0f ? 0.5f : 0.25f) + 1.0f, output[static_cast<size_t> (i)])
                << "size " << size << ", sample " << i;
        }
    }
}

TEST (YdspVectorizerTests, FixedIndexArrayReadsObserveEarlierIterations)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            process {
                for i in 0..8 { z[i] = z[0] + in; }
                out = z[7];
            }
        }
    )YDSP"), compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (44100.0, 8);
    std::vector<float> input (8, 1.0f), output (8);
    for (int block = 0; block < 2; ++block)
    {
        vectorizerRunBlock (graph, input.data(), output.data(), 8);
        for (int i = 0; i < 8; ++i)
            EXPECT_FLOAT_EQ (static_cast<float> (block * 8 + i + 2), output[static_cast<size_t> (i)]);
    }
}

TEST (YdspVectorizerTests, WidenedRoundingLoopProducesTheExpectedOutput)
{
    // Rounding widens element-wise and each op rounds once, so the widened
    // kernel must be bit-identical to the scalar formula - including negative
    // inputs and the rint ties-to-even cases (-2.5 -> -2, -0.5 -> -0, 1.5 -> 2).
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for i in 0..blockSize {
                        out[i] = floor (in[i] * 4.0) * 0.25 + ceil (in[i]) * 0.5 + rint (in[i]) * 0.125;
                    }
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        std::vector<float> input (static_cast<size_t> (blockSize));
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        for (int s = 0; s < blockSize; ++s)
        {
            const auto base = static_cast<float> ((s % 7) - 3);
            input[static_cast<size_t> (s)] = base + ((s % 2) == 0 ? 0.0f : 0.5f);
        }

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        for (int s = 0; s < blockSize; ++s)
        {
            const auto in = input[static_cast<size_t> (s)];

            const auto expected = std::floor (in * 4.0f) * 0.25f
                                + std::ceil (in) * 0.5f
                                + std::rint (in) * 0.125f;

            EXPECT_FLOAT_EQ (expected, output[static_cast<size_t> (s)])
                << blockSize << " samples, sample " << s << " (in = " << in << ")";
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, StreamLoopProducesTheExpectedOutput)
{
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    for i in 0..blockSize { out[i] = in[i] * 0.5 + in[i] * in[i]; }
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        std::vector<float> input (static_cast<size_t> (blockSize));
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        for (int s = 0; s < blockSize; ++s)
            input[static_cast<size_t> (s)] = 0.25f * static_cast<float> (s);

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        for (int s = 0; s < blockSize; ++s)
        {
            const auto in = input[static_cast<size_t> (s)];

            EXPECT_FLOAT_EQ (in * 0.5f + in * in, output[static_cast<size_t> (s)])
                << blockSize << " samples, sample " << s;
        }

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

TEST (YdspVectorizerTests, SampleModeGainProducesTheExpectedOutput)
{
    YdspCompiler compiler;

    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.5 + 0.25; }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
    graph.prepare (44100.0, 10);

    const std::vector<float> input (10, 1.0f);
    std::vector<float> output (10, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), 10);

    for (int s = 0; s < 10; ++s)
        EXPECT_FLOAT_EQ (0.75f, output[static_cast<size_t> (s)]) << "sample " << s;

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, StreamLoopReductionProducesTheExpectedOutput)
{
    for (const int blockSize : { 10, 63 })
    {
        YdspCompiler compiler;

        auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
            processor P {
                input stream in;
                output stream out;
                process block {
                    float s = 0.0;
                    for i in 0..blockSize { s = s + in[i]; }
                    out[0] = s;
                }
            }
        )YDSP"),
                                             compiler);

        ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();
        graph.prepare (44100.0, blockSize);

        // 0.25 is a power of two, so every reassociated partial sum is exact:
        // the folded vector result must equal the scalar-ordered sum bit for bit.
        std::vector<float> input (static_cast<size_t> (blockSize), 0.25f);
        std::vector<float> output (static_cast<size_t> (blockSize), 0.0f);

        vectorizerRunBlock (graph, input.data(), output.data(), blockSize);

        EXPECT_FLOAT_EQ (0.25f * static_cast<float> (blockSize), output[0])
            << blockSize << " samples";

        if (::testing::Test::HasFailure())
            std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
    }
}

#if ! YUP_WASM

TEST (YdspVectorizerTests, EmitsPackedStreamInstructions)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5 + 0.25; }
            }
        }
    )YDSP"),
                                         compiler);

    ASSERT_TRUE (graph.isValid());

    const auto listing = graph.getDiagnostics().toString();
    ASSERT_FALSE (listing.isEmpty());

#if ASMJIT_ARCH_ARM
    EXPECT_TRUE (listing.contains (".4s"))
        << "expected an ASIMD vector form for the stream loop in the listing";
#elif ASMJIT_ARCH_X86
    EXPECT_TRUE (listing.contains ("movups"))
        << "expected packed stream loads/stores in the listing";
    EXPECT_TRUE (listing.contains ("mulps") || listing.contains ("addps")
                 || listing.contains ("vfmadd") || listing.contains ("vfmsub"))
        << "expected packed arithmetic (or a packed fused multiply-add) in the listing";
#endif

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << listing << std::endl;
}

#endif

//==============================================================================

TEST (YdspVectorizerTests, ReportsWhyEachLoopWasNotVectorized)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (vectorizerMixedReasonsSource), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    const auto& results = fn.vectorizationResults;

    ASSERT_EQ (3u, results.size());

    EXPECT_TRUE (results[0].widened());
    EXPECT_EQ (0, results[0].loopId);
    EXPECT_EQ (YdspVectorizer::vectorWidth, results[0].laneCount);

    EXPECT_FALSE (results[1].widened());
    EXPECT_EQ (YdspVectorizationReason::unsupportedWidenedOp, results[1].reason);
    EXPECT_TRUE (results[1].describe().contains ("was not vectorized"));
    EXPECT_TRUE (results[1].describe().contains ("unsupported operation"));

    EXPECT_FALSE (results[2].widened());
    EXPECT_EQ (YdspVectorizationReason::shortTripCount, results[2].reason);

    // The report aggregates the rejection reasons, deduplicated.
    YdspVectorizationReport report;
    report.loops = results;

    EXPECT_EQ (1, report.countWidened());
    EXPECT_EQ (2, report.rejectionReasons().size());
}

TEST (YdspVectorizerTests, ReportsARuntimeStartAsTheReason)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[8];
            state float offset;
            process block {
                let start = int32 (offset);
                let sample = in[0];
                for i in start..8 { z[0] = z[0] * 0.5 + sample; }
                out[0] = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& results = ir->kernels[0]->vectorizationResults;

    ASSERT_EQ (1u, results.size());
    EXPECT_EQ (YdspVectorizationReason::nonConstantStart, results[0].reason);
}

TEST (YdspVectorizerTests, ExecutionReportCarriesPerLoopReasons)
{
#if ! YUP_WASM || defined (__wasm_simd128__)
    YdspCompileOptions options;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler, options);

    ASSERT_TRUE (graph.isValid()) << compiler.getDiagnostics().toString();

    const auto& kernels = graph.getExecutionReport().getKernels();
    ASSERT_FALSE (kernels.empty());

    const auto& report = kernels[0].loopVectorization;

    ASSERT_EQ (3u, report.loops.size());
    EXPECT_EQ (1, report.countWidened());
    EXPECT_TRUE (report.loops[0].widened());
    EXPECT_EQ (YdspVectorizationReason::unsupportedWidenedOp, report.loops[1].reason);
    EXPECT_EQ (YdspVectorizationReason::shortTripCount, report.loops[2].reason);

    EXPECT_TRUE (report.rejectionReasons().contains ("an unsupported operation consumes a widened value"));
    EXPECT_TRUE (report.rejectionReasons().contains ("the loop span is shorter than one vector"));
#endif
}

TEST (YdspVectorizerTests, MissedVectorizationAppearsAsInfoDiagnostics)
{
#if ! YUP_WASM || defined (__wasm_simd128__)
    YdspCompileOptions options;
    options.emitOptimizationReport = true;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler, options);

    ASSERT_TRUE (graph.isValid());

    // A successful compile moves the diagnostics into the graph.
    const auto text = graph.getDiagnostics().toString();

    EXPECT_TRUE (text.contains ("kernel 'P': loop 1 was not vectorized"))
        << "expected the select rejection as an info diagnostic";
    EXPECT_TRUE (text.contains ("kernel 'P': loop 2 was not vectorized"))
        << "expected the short-trip-count rejection as an info diagnostic";
    EXPECT_FALSE (text.contains ("loop 0 was not vectorized"))
        << "the widened loop must not be reported as a miss";
#endif
}

TEST (YdspVectorizerTests, MissedVectorizationIsSilentUnlessRequested)
{
    // Without emitOptimizationReport no info diagnostics are produced.
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerMixedReasonsSource), compiler);

    ASSERT_TRUE (graph.isValid());

    EXPECT_FALSE (graph.getDiagnostics().toString().contains ("was not vectorized"));
}

//==============================================================================
// Widened comparison values.
//
// Each relational operator contributes a distinct power of two to the sum, so
// the per-sample result is a bitmask and a single wrong predicate names itself.
// The values are whole numbers, so the comparison below is exact.
//
// Worth having as a *value* test rather than a "did it assemble" one: the
// legacy SSE2 CMPPS is two-operand and only decodes a three-bit predicate, so
// the four-lane x86 path has to seed its destination and swap the operands of
// the greater-than forms, none of which an assembler would reject.

namespace
{

constexpr auto vectorizerCompareSource = R"YDSP(
    processor P {
        input stream in;
        output stream out;
        process block {
            for i in 0..blockSize {
                let x = in[i];
                out[i] = select (x >  0.0,  1.0, 0.0)
                       + select (x >= 0.0,  2.0, 0.0)
                       + select (x <  0.0,  4.0, 0.0)
                       + select (x <= 0.0,  8.0, 0.0)
                       + select (x == 0.0, 16.0, 0.0)
                       + select (x != 0.0, 32.0, 0.0);
            }
        }
    }
)YDSP";

/** The mask the six operators produce for `x`, computed the way the patch
    spells them. */
float vectorizerCompareExpected (float x)
{
    return (x > 0.0f ? 1.0f : 0.0f)
         + (x >= 0.0f ? 2.0f : 0.0f)
         + (x < 0.0f ? 4.0f : 0.0f)
         + (x <= 0.0f ? 8.0f : 0.0f)
         + (x == 0.0f ? 16.0f : 0.0f)
         + (x != 0.0f ? 32.0f : 0.0f);
}

void vectorizerCheckCompares (YdspAudioGraph& graph, const char* what)
{
    ASSERT_TRUE (graph.isValid());

    // Not a multiple of four, so the widened body and the scalar remainder both
    // run and both have to agree.
    constexpr int numSamples = 11;

    graph.prepare (44100.0, numSamples);

    const std::vector<float> input { 1.0f, -1.0f, 0.0f, 2.5f, -2.5f, 0.0f, 0.25f, -0.25f, 7.0f, -7.0f, 0.0f };
    std::vector<float> output (numSamples, 0.0f);

    vectorizerRunBlock (graph, input.data(), output.data(), numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto x = input[static_cast<size_t> (i)];

        EXPECT_EQ (vectorizerCompareExpected (x), output[static_cast<size_t> (i)])
            << what << ": sample " << i << " (x = " << x << ")";
    }
}

} // namespace

TEST (YdspVectorizerTests, WidenedComparesProduceTheExpectedValues)
{
    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerCompareSource), compiler);

    vectorizerCheckCompares (graph, "host target");

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, WidenedComparesAgreeOnTheFourLaneBaselineTarget)
{
    // Requesting the SSE2 baseline is what reaches the legacy four-lane CMPPS
    // path on x86-64; on a host without SSE2 the request is declined and the
    // kernel compiles scalar, which still has to produce the same values.
    YdspCompileOptions options;
    options.targetPolicy = YdspTargetPolicy::baseline;
    options.baselineTarget = YdspNativeTarget::sse2;

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (vectorizerPatch (vectorizerCompareSource), compiler, options);

    vectorizerCheckCompares (graph, "sse2 baseline target");

    if (::testing::Test::HasFailure())
        std::cout << "\n[AsmJit] " << graph.getDiagnostics().toString() << std::endl;
}

TEST (YdspVectorizerTests, MatchAndIfRetainEquivalentOptimizedLoops)
{
    const struct { const char* selector; const char* ifBody; const char* matchBody; bool mustVectorize; } cases[] {
        { "v > 0.0", "if (choice) { y = v * 0.5; } else { y = v + dry; }",
          "match (choice) { true => { y = v * 0.5; }, _ => { y = v + dry; } }", true },
        { "v > 0.0", "if (!choice) { y = v + dry; } else { y = v * 0.5; }",
          "match (choice) { false => { y = v + dry; }, _ => { y = v * 0.5; } }", false },
        { "1 + 1", "if (choice == 1) { y = v - dry; } else if (choice == 2) { y = v * 0.5; } else { y = dry; }",
          "match (choice) { 1 => { y = v - dry; }, 2 => { y = v * 0.5; }, _ => { y = dry; } }", true },
        { "int (dry)", "if (choice == 1) { y = v - dry; } else if (choice == 2) { y = v * 0.5; } else { y = dry; }",
          "match (choice) { 1 => { y = v - dry; }, 2 => { y = v * 0.5; }, _ => { y = dry; } }", false },
    };
    for (const auto& item : cases)
    {
        SCOPED_TRACE (item.matchBody);
        const auto source = [&] (const char* body)
        {
            const String statement (body);
            const auto selection = statement.startsWith ("match")
                                     ? statement.replace ("match (choice)", String ("match (") + item.selector + ")")
                                     : String ("let choice = ") + item.selector + "; " + statement;
            return vectorizerPatch (String ("processor P { input stream in; output stream out; state float z[32]; process { let dry = in; for i in 0..32 { let v = z[i]; float y = 0.0; ")
                                    + selection + " z[i] = y; } out = z[0]; } }");
        };
        YdspDiagnostics ifDiagnostics, matchDiagnostics;
        auto ifIr = vectorizerBuildIr (source (item.ifBody), ifDiagnostics);
        auto matchIr = vectorizerBuildIr (source (item.matchBody), matchDiagnostics);
        ASSERT_FALSE (ifDiagnostics.hasErrors()) << ifDiagnostics.toString();
        ASSERT_FALSE (matchDiagnostics.hasErrors()) << matchDiagnostics.toString();
        ASSERT_NE (nullptr, ifIr);
        ASSERT_NE (nullptr, matchIr);
        ASSERT_EQ (1u, ifIr->kernels.size());
        ASSERT_EQ (1u, matchIr->kernels.size());
        const auto& reference = *ifIr->kernels[0];
        const auto& actual = *matchIr->kernels[0];
        EXPECT_EQ (reference.vectorized, actual.vectorized);
        EXPECT_EQ (reference.vectorWidth, actual.vectorWidth);
        if (item.mustVectorize)
            EXPECT_TRUE (actual.vectorized);
        ASSERT_EQ (reference.vectorizationResults.size(), actual.vectorizationResults.size());
        for (size_t i = 0; i < reference.vectorizationResults.size(); ++i)
        {
            EXPECT_EQ (reference.vectorizationResults[i].reason, actual.vectorizationResults[i].reason);
            EXPECT_EQ (reference.vectorizationResults[i].laneCount, actual.vectorizationResults[i].laneCount);
        }
        const auto shape = [] (const YdspIrFunction& fn)
        {
            std::map<std::pair<int, int>, int> counts;
            for (const auto& block : fn.blocks)
            {
                if (block.term == YdspIrTerm::branchIf)
                    ++counts[{ -1, 0 }];
                for (const auto& inst : block.insts)
                    ++counts[{ static_cast<int> (inst.op), inst.result >= 0 ? fn.laneCountOf (inst.result) : 0 }];
            }
            return counts;
        };
        EXPECT_EQ (shape (reference), shape (actual));
    }
}

//==============================================================================
// Host buffers that overlap: a widened stream loop reads a whole vector before
// it writes one, so it must fall back to the scalar loop unless the buffers are
// disjoint or exactly the same.

namespace
{

constexpr int vectorizerOverlapSamples = 67;
constexpr int vectorizerOverlapPadding = 16;

// One allocation holding the input at `padding` and the output `offset`
// samples after it, filled with a ramp.
std::vector<float> vectorizerOverlapStorage()
{
    std::vector<float> storage (static_cast<size_t> (vectorizerOverlapSamples + 2 * vectorizerOverlapPadding));

    for (size_t i = 0; i < storage.size(); ++i)
        storage[i] = 1.0f + 0.25f * static_cast<float> (i);

    return storage;
}

} // namespace

TEST (YdspVectorizerTests, WidenedStreamLoopMatchesScalarOrderForOverlappingBuffers)
{
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process { out = in * 0.5; }
        }
    )YDSP");

    for (const int offset : { 0, 1, 3, 9, 16, -1, -5 })
    {
        SCOPED_TRACE (offset);

        YdspCompiler compiler;
        auto graph = vectorizerCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        graph.prepare (48000.0, 128);

        auto expected = vectorizerOverlapStorage();
        {
            const auto* in = expected.data() + vectorizerOverlapPadding;
            auto* out = expected.data() + vectorizerOverlapPadding + offset;

            for (int i = 0; i < vectorizerOverlapSamples; ++i)
                out[i] = in[i] * 0.5f;
        }

        auto actual = vectorizerOverlapStorage();
        vectorizerRunBlock (graph,
                            actual.data() + vectorizerOverlapPadding,
                            actual.data() + vectorizerOverlapPadding + offset,
                            vectorizerOverlapSamples);

        EXPECT_EQ (expected, actual);
    }
}

TEST (YdspVectorizerTests, WidenedStreamLoopMatchesScalarOrderWhenOutputOverlapsOneOfTwoInputs)
{
    const auto source = String (R"YDSP(
        processor P {
            input stream a;
            input stream b;
            output stream out;
            process { out = a + b; }
        }
        graph G { input stream x; input stream w; output stream y; node p = P; connection { x -> p.a; w -> p.b; p.out -> y; } }
    )YDSP");

    for (const int offset : { 0, 1, 2, 7 })
    {
        SCOPED_TRACE (offset);

        YdspCompiler compiler;
        auto graph = vectorizerCompileGraph (source, compiler);
        ASSERT_TRUE (graph.isValid());
        graph.prepare (48000.0, 128);

        std::vector<float> first (static_cast<size_t> (vectorizerOverlapSamples));

        for (size_t i = 0; i < first.size(); ++i)
            first[i] = 0.5f * static_cast<float> (i);

        auto expected = vectorizerOverlapStorage();
        {
            const auto* in = expected.data() + vectorizerOverlapPadding;
            auto* out = expected.data() + vectorizerOverlapPadding + offset;

            for (int i = 0; i < vectorizerOverlapSamples; ++i)
                out[i] = first[static_cast<size_t> (i)] + in[i];
        }

        auto actual = vectorizerOverlapStorage();

        const auto numSamples = static_cast<size_t> (vectorizerOverlapSamples);
        std::vector<YdspInputBuffer> inputs;
        inputs.emplace_back (Span<const float> (first.data(), numSamples));
        inputs.emplace_back (Span<const float> (actual.data() + vectorizerOverlapPadding, numSamples));

        std::vector<YdspOutputBuffer> outputs;
        outputs.emplace_back (Span<float> (actual.data() + vectorizerOverlapPadding + offset, numSamples));

        ASSERT_EQ (YdspProcessResult::ok, graph.process (YdspProcessRequest { inputs, outputs, vectorizerOverlapSamples }));

        EXPECT_EQ (expected, actual);
    }
}

TEST (YdspVectorizerTests, GuardsOnlyTheStreamPairsAWidenedStoreCanReorder)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize { out[i] = in[i] * 0.5; }
                for j in 0..blockSize { out[j] = out[j] + 1.0; }
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];

    // Both loops widen, but only the first one touches two streams.
    int widenedLoops = 0;

    for (const auto& result : fn.vectorizationResults)
        widenedLoops += result.widened() ? 1 : 0;

    EXPECT_EQ (2, widenedLoops);
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::loadInputAddress));
    EXPECT_EQ (1, vectorizerCountInst (fn, YdspIrOp::loadOutputAddress));
}

TEST (YdspVectorizerTests, PeeledIterationsRoundLikeTheWidenedLanes)
{
    // Six elements are one vector plus two peeled iterations. Every element
    // runs the same recurrence on the same input, so they must stay bit-equal:
    // a peeled iteration may not round its multiply-add differently from a lane.
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.9 + in * 0.3; }
                out = z[0] - z[5];
            }
        }
    )YDSP");

    YdspCompiler compiler;
    auto graph = vectorizerCompileGraph (source, compiler);
    ASSERT_TRUE (graph.isValid());
    graph.prepare (48000.0, 64);

    std::vector<float> input (64), output (64, 1.0f);

    for (size_t i = 0; i < input.size(); ++i)
        input[i] = 0.1f + 0.37f * static_cast<float> (i % 7);

    vectorizerRunBlock (graph, input.data(), output.data(), 64);

    EXPECT_EQ (std::vector<float> (64, 0.0f), output);
}

TEST (YdspVectorizerTests, PeeledIterationsStayUnfusedWhereTheLanesCannotFuse)
{
    // A target that contracts but has no packed fused multiply-add leaves the
    // lanes as a multiply and an add, so the peeled iterations must keep theirs
    // apart too instead of being contracted into a scalar fused multiply-add.
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.9 + in * 0.3; }
                out = z[0] - z[5];
            }
        }
    )YDSP");

    for (const bool hasPackedFma : { false, true })
    {
        YdspDiagnostics diagnostics;
        YdspLexer lexer (source, diagnostics);
        auto program = YdspParser (lexer.tokenize(), diagnostics).parseProgram();
        ASSERT_NE (nullptr, program) << diagnostics.toString();
        auto analyzed = YdspSemanticAnalyzer (diagnostics).analyze (std::move (program));
        ASSERT_NE (nullptr, analyzed) << diagnostics.toString();

        YdspOptimizer optimizer (diagnostics);
        optimizer.setFastMath (true);
        optimizer.setContractionEnabled (true);
        optimizer.setTargetHasPackedFusedMultiplyAdd (hasPackedFma);
        optimizer.setVectorizationEnabled (true);

        auto ir = optimizer.build (*analyzed);
        ASSERT_NE (nullptr, ir) << diagnostics.toString();

        const auto& fn = *ir->kernels[0];
        ASSERT_TRUE (fn.vectorized);

        // One widened multiply-add and two peeled ones, or none at all.
        EXPECT_EQ (hasPackedFma ? 3 : 0, vectorizerCountInst (fn, YdspIrOp::fmaF)) << hasPackedFma;
    }
}

TEST (YdspVectorizerTests, PeeledIterationsReuseTheHoistedLoopInvariantWork)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[6];
            process {
                for i in 0..6 { z[i] = z[i] * 0.9 + in * 0.3; }
                out = z[0];
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);

    const auto& fn = *ir->kernels[0];
    ASSERT_TRUE (fn.vectorized);

    // `in * 0.3` is computed once; the other multiplies are one per peeled
    // iteration plus the widened one.
    EXPECT_EQ (4, vectorizerCountInst (fn, YdspIrOp::mulF));
}

#if ! YUP_WASM
TEST (YdspVectorizerTests, WidensALoopThatConvertsItsVariableToFloat)
{
    // `float (i + 1)` is the harmonic number of an additive bank. Widened, it
    // is a vector that counts up from the scalar conversion, which is exact
    // for a constant bound - so the widened loop gives the scalar loop's
    // samples bit for bit.
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float z[10];
            process block {
                for n in 0..blockSize {
                    for i in 0..10 { z[i] = z[i] * 0.5 + in[n] * float (i + 1); }
                    out[n] = z[0] + z[4] + z[9];
                }
            }
        }
    )YDSP");

    YdspCompileOptions widened;
    widened.fastMath = false;
    widened.emitOptimizationReport = true;

    auto scalar = widened;
    scalar.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompiler compiler;
    auto widenedGraph = vectorizerCompileGraph (source, compiler, widened);
    auto scalarGraph = vectorizerCompileGraph (source, compiler, scalar);

    ASSERT_TRUE (widenedGraph.isValid() && scalarGraph.isValid());
    ASSERT_TRUE (widenedGraph.prepare (48000.0, 64).wasOk() && scalarGraph.prepare (48000.0, 64).wasOk());

    const auto& loops = widenedGraph.getExecutionReport().getKernels()[0].loopVectorization.loops;
    ASSERT_FALSE (loops.empty());
    EXPECT_TRUE (loops[0].widened()) << loops[0].describe();

    std::vector<float> input (64), widenedOutput (64, 0.0f), scalarOutput (64, 0.0f);

    for (size_t i = 0; i < input.size(); ++i)
        input[i] = std::sin (static_cast<float> (i) * 0.37f);

    vectorizerRunBlock (widenedGraph, input.data(), widenedOutput.data(), 64);
    vectorizerRunBlock (scalarGraph, input.data(), scalarOutput.data(), 64);

    for (size_t i = 0; i < input.size(); ++i)
        EXPECT_EQ (scalarOutput[i], widenedOutput[i]) << i;
}

TEST (YdspVectorizerTests, UnrolledAccumulationIsSplitIntoPartialSumsOnlyUnderFastMath)
{
    // The taps of a FIR: 32 products added into one sum. Unrolled, each add
    // waits on the one before it; fast math may sum them in four interleaved
    // partials instead, strict math keeps the order as written.
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state float c[32], h[32];
            state int idx;
            process block {
                for n in 0..blockSize {
                    h[idx] = in[n];
                    float acc = 0.0;
                    for i in 0..32 { acc = acc + c[i] * h[(idx - i) & 31]; }
                    idx = (idx + 1) & 31;
                    out[n] = acc;
                }
            }
        }
    )YDSP");

    for (const bool fastMath : { false, true })
    {
        YdspCompileOptions options;
        options.fastMath = fastMath;
        options.emitOptimizationReport = true;

        YdspCompiler compiler;
        auto graph = vectorizerCompileGraph (source, compiler, options);
        ASSERT_TRUE (graph.isValid());

        const auto& kernel = graph.getExecutionReport().getKernels()[0];
        EXPECT_TRUE (kernel.unrolled) << fastMath;
        EXPECT_EQ (fastMath, kernel.reductionSplit) << fastMath;
    }
}
#endif

//==============================================================================
// Packed int32, int32 <-> float32 conversions, and integer comparisons/select.

TEST (YdspVectorizerTests, WidensInt32StateArrayAndConversionOpsInIr)
{
    YdspDiagnostics diagnostics;
    auto ir = vectorizerBuildIr (vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int a[10];
            state int b[10];
            process {
                for i in 0..10 {
                    let v = int (in * 10.0) + a[i];
                    let c = clamp (abs (v), 0, 255);
                    let m = select (c > 100, c & 15, (b[i] ^ 7) << 1);
                    a[i] = max (m, -b[i]);
                    b[i] = min (a[i] >> 1, 64);
                }
                out = float (a[0] + b[9]);
            }
        }
    )YDSP"), diagnostics);

    ASSERT_FALSE (diagnostics.hasErrors()) << diagnostics.toString();
    ASSERT_NE (nullptr, ir);
    ASSERT_FALSE (ir->kernels.empty());

    const auto& fn = *ir->kernels[0];
    EXPECT_TRUE (fn.vectorized);

    bool foundWidenedLoadI = false;
    bool foundWidenedStoreI = false;
    bool foundWidenedSplatI = false;
    bool foundWidenedSelectI = false;
    bool foundWidenedGtI = false;

    for (const auto& block : fn.blocks)
    {
        for (const auto& inst : block.insts)
        {
            if (inst.op == YdspIrOp::loadStateArrayI && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedLoadI = true;

            if (inst.op == YdspIrOp::storeStateArrayI && fn.laneCountOf (inst.b) == fn.vectorWidth)
                foundWidenedStoreI = true;

            if (inst.op == YdspIrOp::vsplat && fn.laneCountOf (inst.result) == fn.vectorWidth
                && static_cast<size_t> (inst.result) < fn.valueTypes.size()
                && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int32Type)
                foundWidenedSplatI = true;

            if (inst.op == YdspIrOp::selectB && fn.laneCountOf (inst.result) == fn.vectorWidth
                && static_cast<size_t> (inst.result) < fn.valueTypes.size()
                && fn.valueTypes[static_cast<size_t> (inst.result)] == YdspValueType::int32Type)
                foundWidenedSelectI = true;

            if (inst.op == YdspIrOp::gtI && fn.laneCountOf (inst.result) == fn.vectorWidth)
                foundWidenedGtI = true;
        }
    }

    EXPECT_TRUE (foundWidenedLoadI);
    EXPECT_TRUE (foundWidenedStoreI);
    EXPECT_TRUE (foundWidenedSplatI);
    EXPECT_TRUE (foundWidenedSelectI);
    EXPECT_TRUE (foundWidenedGtI);
}

TEST (YdspVectorizerTests, WidenedInt32StateBankMatchesScalarReference)
{
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            state int acc[11];
            process {
                let drive = int (in);
                for i in 0..11 {
                    let x = acc[i] + drive - 3;
                    let scaled = x * 2;
                    let clamped = clamp (abs (scaled), 1, 200);
                    let masked = ((clamped & 127) | 4) ^ 1;
                    let shifted = (masked << 2) >> 1;
                    acc[i] = select (shifted >= 20, min (shifted, 150), max (-shifted, -50));
                }
                out = float (acc[0] + acc[3] + acc[10]);
            }
        }
    )YDSP");

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompileOptions hostOptions;
    hostOptions.emitOptimizationReport = true;

    YdspCompileOptions sse2Options;
    sse2Options.targetPolicy = YdspTargetPolicy::baseline;
    sse2Options.baselineTarget = YdspNativeTarget::sse2;

    YdspCompiler compiler;
    auto scalarGraph = vectorizerCompileGraph (source, compiler, baselineOptions);
    auto hostGraph = vectorizerCompileGraph (source, compiler, hostOptions);
    auto sse2Graph = vectorizerCompileGraph (source, compiler, sse2Options);

    ASSERT_TRUE (scalarGraph.isValid() && hostGraph.isValid() && sse2Graph.isValid());

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (hostGraph.getExecutionReport().getKernels()[0].vectorized);
#endif

    scalarGraph.prepare (48000.0, 16);
    hostGraph.prepare (48000.0, 16);
    sse2Graph.prepare (48000.0, 16);

    const std::vector<float> input { 10.0f, -5.0f, 0.0f, 33.0f, -12.0f, 7.0f, 100.0f, -40.0f,
                                     1.0f, -1.0f, 15.0f, -20.0f, 4.0f, 8.0f, -9.0f, 25.0f };
    std::vector<float> scalarOut (16, 0.0f), hostOut (16, 0.0f), sse2Out (16, 0.0f);

    vectorizerRunBlock (scalarGraph, input.data(), scalarOut.data(), 16);
    vectorizerRunBlock (hostGraph, input.data(), hostOut.data(), 16);
    vectorizerRunBlock (sse2Graph, input.data(), sse2Out.data(), 16);

    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_EQ (scalarOut[i], hostOut[i]) << "host sample " << i;
        EXPECT_EQ (scalarOut[i], sse2Out[i]) << "sse2 sample " << i;
    }
}

TEST (YdspVectorizerTests, WidenedFloatIntConversionsAndSelectMatchScalarReference)
{
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    let n = int (in[i]);
                    let adjusted = select (n < 0, n - 5, n + 7);
                    let f = float (adjusted);
                    out[i] = select (n == 0, 99.0, f);
                }
            }
        }
    )YDSP");

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompileOptions hostOptions;
    hostOptions.emitOptimizationReport = true;

    YdspCompileOptions sse2Options;
    sse2Options.targetPolicy = YdspTargetPolicy::baseline;
    sse2Options.baselineTarget = YdspNativeTarget::sse2;

    YdspCompiler compiler;
    auto scalarGraph = vectorizerCompileGraph (source, compiler, baselineOptions);
    auto hostGraph = vectorizerCompileGraph (source, compiler, hostOptions);
    auto sse2Graph = vectorizerCompileGraph (source, compiler, sse2Options);

    ASSERT_TRUE (scalarGraph.isValid() && hostGraph.isValid() && sse2Graph.isValid());

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (hostGraph.getExecutionReport().getKernels()[0].vectorized);
#endif

    const std::vector<float> input {
        0.0f, -0.0f, 3.75f, -4.25f,
        2147483520.0f, 2147483648.0f, -2147483648.0f, -1e12f,
        1e12f, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(),
        42.5f, -19.9f, 0.25f, -0.25f,
        100.0f, -100.0f, 7.0f
    };

    const int numSamples = static_cast<int> (input.size());
    scalarGraph.prepare (48000.0, numSamples);
    hostGraph.prepare (48000.0, numSamples);
    sse2Graph.prepare (48000.0, numSamples);

    std::vector<float> scalarOut (input.size(), 0.0f), hostOut (input.size(), 0.0f), sse2Out (input.size(), 0.0f);

    vectorizerRunBlock (scalarGraph, input.data(), scalarOut.data(), numSamples);
    vectorizerRunBlock (hostGraph, input.data(), hostOut.data(), numSamples);
    vectorizerRunBlock (sse2Graph, input.data(), sse2Out.data(), numSamples);

    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_EQ (scalarOut[i], hostOut[i]) << "host sample " << i << " (in=" << input[i] << ")";
        EXPECT_EQ (scalarOut[i], sse2Out[i]) << "sse2 sample " << i << " (in=" << input[i] << ")";
    }
}

TEST (YdspVectorizerTests, WidenedInt32ComparesProduceExpectedValues)
{
    const auto source = vectorizerPatch (R"YDSP(
        processor P {
            input stream in;
            output stream out;
            process block {
                for i in 0..blockSize {
                    let x = int (in[i]);
                    let mask = select (x >  0,  1, 0)
                             + select (x >= 0,  2, 0)
                             + select (x <  0,  4, 0)
                             + select (x <= 0,  8, 0)
                             + select (x == 0, 16, 0)
                             + select (x != 0, 32, 0);
                    out[i] = float (mask);
                }
            }
        }
    )YDSP");

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompileOptions hostOptions;
    hostOptions.emitOptimizationReport = true;

    YdspCompileOptions sse2Options;
    sse2Options.targetPolicy = YdspTargetPolicy::baseline;
    sse2Options.baselineTarget = YdspNativeTarget::sse2;

    YdspCompiler compiler;
    auto scalarGraph = vectorizerCompileGraph (source, compiler, baselineOptions);
    auto hostGraph = vectorizerCompileGraph (source, compiler, hostOptions);
    auto sse2Graph = vectorizerCompileGraph (source, compiler, sse2Options);

    ASSERT_TRUE (scalarGraph.isValid() && hostGraph.isValid() && sse2Graph.isValid());

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (hostGraph.getExecutionReport().getKernels()[0].vectorized);
#endif

    const std::vector<float> input { 1.0f, -1.0f, 0.0f, 5.0f, -5.0f, 0.0f, 2147483648.0f, -2147483648.0f, 3.0f, -3.0f, 0.0f };
    const int numSamples = static_cast<int> (input.size());

    scalarGraph.prepare (48000.0, numSamples);
    hostGraph.prepare (48000.0, numSamples);
    sse2Graph.prepare (48000.0, numSamples);

    std::vector<float> scalarOut (input.size(), 0.0f), hostOut (input.size(), 0.0f), sse2Out (input.size(), 0.0f);

    vectorizerRunBlock (scalarGraph, input.data(), scalarOut.data(), numSamples);
    vectorizerRunBlock (hostGraph, input.data(), hostOut.data(), numSamples);
    vectorizerRunBlock (sse2Graph, input.data(), sse2Out.data(), numSamples);

    for (size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_EQ (scalarOut[i], hostOut[i]) << "host sample " << i;
        EXPECT_EQ (scalarOut[i], sse2Out[i]) << "sse2 sample " << i;
    }
}

TEST (YdspVectorizerTests, WidenedSaturatingInt32ArithmeticAndShiftsMatchScalar)
{
    const auto source = String (R"YDSP(
        processor P {
            input stream a;
            input stream b;
            output stream out;
            state int s[11];
            process block {
                for i in 0..blockSize {
                    let x = int (a[i]);
                    let y = int (b[i]);
                    let sum = x + y;
                    let diff = x - y;
                    let prod = x * y;
                    let n = -x;
                    let ab = abs (x);
                    let shl = x << y;
                    let shr = x >> y;
                    out[i] = float ((sum ^ diff ^ prod ^ n ^ ab ^ shl ^ shr) & 65535);
                }
                for j in 0..11 {
                    s[j] = s[j] + 2147483647;
                }
            }
        }
        graph G {
            input stream xa;
            input stream xb;
            output stream y;
            node p = P;
            connection { xa -> p.a; xb -> p.b; p.out -> y; }
        }
    )YDSP");

    YdspCompileOptions baselineOptions;
    baselineOptions.optimizationTier = YdspOptimizationTier::baseline;

    YdspCompileOptions hostOptions;
    hostOptions.emitOptimizationReport = true;

    YdspCompileOptions sse2Options;
    sse2Options.targetPolicy = YdspTargetPolicy::baseline;
    sse2Options.baselineTarget = YdspNativeTarget::sse2;

    YdspCompiler compiler;
    auto scalarGraph = vectorizerCompileGraph (source, compiler, baselineOptions);
    auto hostGraph = vectorizerCompileGraph (source, compiler, hostOptions);
    auto sse2Graph = vectorizerCompileGraph (source, compiler, sse2Options);

    ASSERT_TRUE (scalarGraph.isValid() && hostGraph.isValid() && sse2Graph.isValid());

#if ! YUP_WASM || defined (__wasm_simd128__)
    EXPECT_TRUE (hostGraph.getExecutionReport().getKernels()[0].vectorized);
#endif

    const std::vector<float> inA {
        2147483648.0f, -2147483648.0f, -2147483648.0f, 100000.0f,
        -100000.0f, 2147483520.0f, -7.0f, 42.0f,
        -2147483648.0f, 0.0f, 123456.0f
    };
    const std::vector<float> inB {
        1.0f, 1.0f, -1.0f, 100000.0f,
        100000.0f, 1000.0f, 33.0f, -1.0f,
        -2147483648.0f, 31.0f, 3.0f
    };

    const int numSamples = static_cast<int> (inA.size());
    scalarGraph.prepare (48000.0, numSamples);
    hostGraph.prepare (48000.0, numSamples);
    sse2Graph.prepare (48000.0, numSamples);

    std::vector<float> scalarOut (inA.size(), 0.0f), hostOut (inA.size(), 0.0f), sse2Out (inA.size(), 0.0f);

    auto runTwo = [numSamples] (YdspAudioGraph& graph, const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& out)
    {
        std::vector<YdspInputBuffer> inputs;
        inputs.emplace_back (Span<const float> (a.data(), static_cast<size_t> (numSamples)));
        inputs.emplace_back (Span<const float> (b.data(), static_cast<size_t> (numSamples)));

        std::vector<YdspOutputBuffer> outputs;
        outputs.emplace_back (Span<float> (out.data(), static_cast<size_t> (numSamples)));

        ASSERT_EQ (YdspProcessResult::ok, graph.process (YdspProcessRequest { inputs, outputs, numSamples }));
    };

    runTwo (scalarGraph, inA, inB, scalarOut);
    runTwo (hostGraph, inA, inB, hostOut);
    runTwo (sse2Graph, inA, inB, sse2Out);

    for (size_t i = 0; i < inA.size(); ++i)
    {
        EXPECT_EQ (scalarOut[i], hostOut[i]) << "host sample " << i;
        EXPECT_EQ (scalarOut[i], sse2Out[i]) << "sse2 sample " << i;
    }
}


